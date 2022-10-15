#include "pch.h"
#include <list>
#include "DirectX.h"
#include "SoundSystem.h"
#include "FileSystem.h"
#include "Resources.h"
#include "Game.h"
#include "logging.h"
#include "Graphics/Render.h"
#include "Physics.h"
#include <vendor/WAVFileReader.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace std::chrono;

namespace Inferno::Sound {
    // Scales game coordinates to audio coordinates.
    // The engine claims to be unitless but doppler, falloff, and reverb are noticeably different using smaller values.
    constexpr float AUDIO_SCALE = 1;
    constexpr float MAX_SFX_VOLUME = 0.75; // should come from settings
    constexpr float MERGE_WINDOW = 1 / 10.0f; // Merge the same sound being played by a source within a window

    struct Sound3DInstance : public Sound3D {
        float Muffle = 1, TargetMuffle = 1;
        bool Started = false;
        Ptr<SoundEffectInstance> Instance;
        AudioEmitter Emitter; // Stores position
        double StartTime = 0;

        void UpdateEmitter(const Vector3& listener, float dt) {
            auto obj = Game::Level.TryGetObject(Source);
            if (obj && obj->IsAlive() && AttachToSource) {
                // Move the emitter to the object location if attached
                auto pos = obj->GetPosition(Game::LerpAmount);
                if (AttachOffset != Vector3::Zero) {
                    auto rot = obj->GetRotation(Game::LerpAmount);
                    pos += Vector3::Transform(AttachOffset, rot);
                }

                Emitter.SetPosition(pos * AUDIO_SCALE);
                Segment = obj->Segment;
            }
            else {
                // object is dead. Should the sound stop?
            }

            assert(Radius > 0);
            auto emitterPos = Emitter.Position / AUDIO_SCALE;
            auto delta = listener - emitterPos;
            Vector3 dir;
            delta.Normalize(dir);
            auto dist = delta.Length();

            auto ratio = std::min(dist / Radius, 1.0f);
            // 1 / (0.97 + 3x)^2 - 0.065 inverse square that crosses at 0,1 and 1,0
            //auto volume = 1 / std::powf(0.97 + 3*ratio, 2) - 0.065f;

            TargetMuffle = 1; // don't hit test very close sounds

            if (dist < Radius) { // only hit test if sound is actually within range
                if (Looped && !Instance->GetState() == SoundState::PLAYING) {
                    //fmt::print("Starting looped sound\n");
                    Instance->Play(true);
                }

                if (Occlusion) {
                    constexpr float MUFFLE_MAX = 0.95f;
                    constexpr float MUFFLE_MIN = 0.25f;

                    if (dist > 10) { // don't hit test nearby sounds
                        Ray ray(emitterPos, dir);
                        LevelHit hit;
                        if (IntersectLevel(Game::Level, ray, Segment, dist, hit)) {
                            auto hitDist = (listener - hit.Point).Length();
                            // we hit a wall, muffle it based on the distance from the source
                            // a sound coming immediately around the corner shouldn't get muffled much
                            TargetMuffle = std::clamp(1 - hitDist / 60, MUFFLE_MIN, MUFFLE_MAX);
                        }
                    }
                }
            }
            else {
                // stop looped sounds when going out of range
                if (Looped && Instance->GetState() == SoundState::PLAYING) {
                    //fmt::print("Stopping out of range looped sound\n");
                    Instance->Stop();
                }
            }

            auto diff = TargetMuffle - Muffle;
            auto sign = Sign(diff);
            Muffle += std::min(abs(diff), dt * 3) * sign; // Take 1/3 a second to reach muffle target

            //auto falloff = std::powf(1 - ratio, 3); // cubic falloff
            auto falloff = 1 - ratio; // linear falloff
            //auto falloff = 1 - (ratio * ratio); // square falloff
            //Instance->SetVolume(Volume * falloff * Muffle * MAX_SFX_VOLUME);

            Debug::Emitters.push_back(Emitter.Position / AUDIO_SCALE);
        }
    };

    namespace {
        // https://github.com/microsoft/DirectXTK/wiki/AudioEngine
        Ptr<AudioEngine> Engine;
        List<Ptr<SoundEffect>> SoundsD1, SoundsD2;
        Dictionary<string, Ptr<SoundEffect>> SoundsD3;

        std::atomic<bool> Alive = false;
        std::thread WorkerThread;
        std::list<Sound3DInstance> SoundInstances;
        std::mutex ResetMutex, SoundInstancesMutex;

        AudioListener Listener;

        constexpr X3DAUDIO_CONE c_listenerCone = {
            X3DAUDIO_PI * 5.0f / 6.0f, X3DAUDIO_PI * 11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f
        };

        constexpr X3DAUDIO_CONE c_emitterCone = {
            0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f
        };

        constexpr X3DAUDIO_DISTANCE_CURVE_POINT c_emitter_LFE_CurvePoints[3] = {
            { 0.0f, 0.1f }, { 0.5f, 0.5f}, { 0.5f, 0.5f }
        };

        constexpr X3DAUDIO_DISTANCE_CURVE c_emitter_LFE_Curve = {
            (X3DAUDIO_DISTANCE_CURVE_POINT*)&c_emitter_LFE_CurvePoints[0], 3
        };

        constexpr X3DAUDIO_DISTANCE_CURVE_POINT c_emitter_Reverb_CurvePoints[3] = {
            { 0.0f, 0.5f}, { 0.75f, 1.0f }, { 1.0f, 0.65f }
        };
        constexpr X3DAUDIO_DISTANCE_CURVE c_emitter_Reverb_Curve = {
            (X3DAUDIO_DISTANCE_CURVE_POINT*)&c_emitter_Reverb_CurvePoints[0], 3
        };
    }

    void SoundWorker(float volume, milliseconds pollRate) {
        SPDLOG_INFO("Starting audio mixer thread");

        auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (!SUCCEEDED(result))
            SPDLOG_WARN("CoInitializeEx did not succeed");

        try {
            auto devices = AudioEngine::GetRendererDetails();
            wstring info = L"Available sound devices:\n";
            for (auto& device : devices)
                info += fmt::format(L"{}\n", device.description/*, device.deviceId*/);

            SPDLOG_INFO(info);

            auto flags = AudioEngine_EnvironmentalReverb | AudioEngine_ReverbUseFilters | AudioEngine_UseMasteringLimiter;
#ifdef _DEBUG
            flags |= AudioEngine_Debug;
#endif
            Engine = MakePtr<AudioEngine>(flags, nullptr/*, devices[0].deviceId.c_str()*/);
            Engine->SetDefaultSampleRate(22050); // Change based on D1/D2
            SoundsD1.resize(255);
            SoundsD2.resize(255);
            Alive = true;
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("Unable to start sound engine: {}", e.what());
            return;
        }

        Engine->SetMasterVolume(volume);

        while (Alive) {
            Debug::Emitters.clear();

            if (Engine->Update()) {
                try {
                    auto dt = pollRate.count() / 1000.0f;
                    //Listener.Update(Render::Camera.Position * AUDIO_SCALE, Render::Camera.Up, dt);
                    Listener.SetOrientation(Render::Camera.GetForward(), Render::Camera.Up);
                    Listener.Position = Render::Camera.Position * AUDIO_SCALE;
                    //Listener.Position = {};
                    //Listener.OrientTop = {};
                    //Listener.OrientTop.y = sin(Game::ElapsedTime * 3.14f);
                    //Listener.OrientTop.x = -cos(Game::ElapsedTime * 3.14f);
                    //Listener.Velocity = {};

                    std::scoped_lock lock(SoundInstancesMutex);
                    auto sound = SoundInstances.begin();
                    while (sound != SoundInstances.end()) {
                        auto state = sound->Instance->GetState();

                        if (!sound->Looped) {
                            if (state == SoundState::STOPPED && sound->Started) {
                                //SPDLOG_INFO("Removing object sound instance");
                                SoundInstances.erase(sound++);
                                continue;
                            }

                            if (state == SoundState::STOPPED && !sound->Started) {
                                // New sound
                                sound->Instance->Play();
                                sound->Started = true;
                            }
                        }
                        //else {

                        //}

                        sound->UpdateEmitter(Render::Camera.Position, dt);
                        // Hack to force sounds caused by the player to be exactly on top of the listener.
                        // Objects and the camera are slightly out of sync due to update timing and threading
                        if (Game::State == GameState::Game && sound->FromPlayer)
                            sound->Emitter.Position = Listener.Position;

                        sound->Instance->Apply3D(Listener, sound->Emitter, false);
                        //DWORD flags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
                        //    | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
                        //    | X3DAUDIO_CALCULATE_REVERB;

                        //{
                        //    float matrix[XAUDIO2_MAX_AUDIO_CHANNELS * 8] = {};
                        //    X3DAUDIO_DSP_SETTINGS dspSettings{};
                        //    dspSettings.SrcChannelCount = 1;
                        //    dspSettings.DstChannelCount = 2;
                        //    dspSettings.pMatrixCoefficients = matrix;

                        //    X3DAUDIO_EMITTER emitter;
                        //    memcpy(&emitter, &sound->Emitter, sizeof(X3DAUDIO_EMITTER));
                        //    //emitter.OrientTop = {};
                        //    //emitter.OrientFront = {};
                        //    emitter.OrientTop.y = emitter.OrientFront.z = 1.0f;

                        //    X3DAUDIO_LISTENER listener;
                        //    memcpy(&listener, &Listener, sizeof(X3DAUDIO_LISTENER));
                        //    //listener.OrientTop = {};
                        //    listener.OrientFront = {};
                        //    //listener.OrientTop.y = -1.0f;
                        //    listener.OrientFront.z = 1.0f;
                        //    //Vector3 top = listener.OrientTop;
                        //    //top.Normalize();
                        //    //listener.OrientTop = top;

                        //    //if (listener.OrientTop.y > 0.8f) 
                        //      //listener.OrientTop = { 0.00001f, 0.99f, 0 };

                        //    auto handle = Engine->Get3DHandle();
                        //    X3DAudioCalculate(handle, &listener, &emitter, flags, &dspSettings);
                        //    SPDLOG_INFO("XY: {:.2f} {:.2f}  LR: {:.2f}, {:.2f}", listener.OrientTop.x, listener.OrientTop.y, matrix[0], matrix[1]);
                        //}
                        sound++;
                    }
                }
                catch (const std::exception& e) {
                    SPDLOG_ERROR("Error in audio worker: {}", e.what());
                }

                std::this_thread::sleep_for(pollRate);
            }
            else {
                // https://github.com/microsoft/DirectXTK/wiki/AudioEngine
                if (!Engine->IsAudioDevicePresent()) {
                }

                if (Engine->IsCriticalError()) {
                    SPDLOG_WARN("Attempting to reset audio engine");
                    Engine->Reset();
                }

                std::this_thread::sleep_for(1000ms);
            }
        }
        SPDLOG_INFO("Stopping audio mixer thread");
        CoUninitialize();
    }

    // Creates a mono PCM sound effect
    SoundEffect CreateSoundEffect(AudioEngine& engine, span<ubyte> raw, uint32 frequency = 22050, float trimStart = 0) {
        // create a buffer and store wfx at the beginning.
        int trim = int(frequency * trimStart);
        auto wavData = MakePtr<uint8[]>(raw.size() + sizeof(WAVEFORMATEX) - trim);
        auto startAudio = wavData.get() + sizeof(WAVEFORMATEX);
        memcpy(startAudio, raw.data() + trim, raw.size() - trim);

        auto wfx = (WAVEFORMATEX*)wavData.get();
        wfx->wFormatTag = WAVE_FORMAT_PCM;
        wfx->nChannels = 1;
        wfx->nSamplesPerSec = frequency;
        wfx->nAvgBytesPerSec = frequency;
        wfx->nBlockAlign = 1;
        wfx->wBitsPerSample = 8;
        wfx->cbSize = 0;

        // Pass the ownership of the buffer to the sound effect
        return SoundEffect(&engine, wavData, wfx, startAudio, raw.size() - trim);
    }

    SoundEffect CreateSoundEffectWav(AudioEngine& engine, span<ubyte> raw) {
        DirectX::WAVData result{};
        DirectX::LoadWAVAudioInMemoryEx(raw.data(), raw.size(), result);

        // create a buffer and store wfx at the beginning.
        auto wavData = MakePtr<uint8[]>(result.audioBytes + sizeof(WAVEFORMATEX));
        auto pWavData = wavData.get();
        auto startAudio = pWavData + sizeof(WAVEFORMATEX);
        memcpy(pWavData, result.wfx, sizeof(WAVEFORMATEX));
        memcpy(pWavData + sizeof(WAVEFORMATEX), result.startAudio, result.audioBytes);

        // Pass the ownership of the buffer to the sound effect
        return SoundEffect(&engine, wavData, (WAVEFORMATEX*)wavData.get(), startAudio, result.audioBytes);
    }

    void Shutdown() {
        if (!Alive) return;
        Alive = false;
        Engine->Suspend();
        WorkerThread.join();
    }

    void Init(HWND, float volume, milliseconds pollRate) {
        // HWND is not used, but indicates the sound system requires a window
        WorkerThread = std::thread(SoundWorker, volume, pollRate);

        //DWORD channelMask{};
        //Engine->GetMasterVoice()->GetChannelMask(&channelMask);
        //auto hresult = X3DAudioInitialize(channelMask, 20, Engine->Get3DHandle());

        //XAUDIO2_VOICE_DETAILS details{};
        //Engine->GetMasterVoice()->GetVoiceDetails(&details);

        //DSPMatrix.resize(details.InputChannels);
        //DSPSettings.SrcChannelCount = 1;
        //DSPSettings.DstChannelCount = DSPMatrix.size();
        //DSPSettings.pMatrixCoefficients = DSPMatrix.data();

        Listener.pCone = (X3DAUDIO_CONE*)&c_listenerCone;

        //X3DAudioCalculate(instance, listener, emitter, flags, &dsp);
    }

    void SetReverb(Reverb reverb) {
        Engine->SetReverb((AUDIO_ENGINE_REVERB)reverb);
    }

    SoundEffect* LoadSoundD1(int id) {
        if (!Seq::inRange(SoundsD1, id)) return nullptr;
        if (SoundsD1[id]) return SoundsD1[int(id)].get();

        std::scoped_lock lock(ResetMutex);
        int frequency = 11025;
        float trimStart = 0;
        if (id == 47)
            trimStart = 0.05f; // Trim the first 50ms from the door close sound due to a crackle

        auto data = Resources::SoundsD1.Read(id);
        if (data.empty()) return nullptr;
        return (SoundsD1[int(id)] = MakePtr<SoundEffect>(CreateSoundEffect(*Engine, data, frequency))).get();
    }

    SoundEffect* LoadSoundD2(int id) {
        if (!Seq::inRange(SoundsD2, id)) return nullptr;
        if (SoundsD2[id]) return SoundsD2[int(id)].get();

        std::scoped_lock lock(ResetMutex);
        int frequency = 22050;

        // The Class 1 driller sound was not resampled for D2 and should be a lower frequency
        if (id == 127)
            frequency = 11025;

        auto data = Resources::SoundsD2.Read(id);
        if (data.empty()) return nullptr;
        return (SoundsD2[int(id)] = MakePtr<SoundEffect>(CreateSoundEffect(*Engine, data, frequency))).get();
    }

    SoundEffect* LoadSoundD3(string fileName) {
        if (fileName.empty()) return nullptr;
        if (SoundsD3[fileName]) return SoundsD3[fileName].get();

        std::scoped_lock lock(ResetMutex);

        if (auto data = Resources::Descent3Hog.ReadEntry(fileName)) {
            return (SoundsD3[fileName] = MakePtr<SoundEffect>(CreateSoundEffectWav(*Engine, *data))).get();
        }
        else {
            return nullptr;
        }
    }

    SoundEffect* LoadSound(const SoundResource& resource) {
        if (!Alive) return nullptr;

        SoundEffect* sound = LoadSoundD3(resource.D3);
        if (!sound) sound = LoadSoundD1(resource.D1);
        if (!sound) sound = LoadSoundD2(resource.D2);
        return sound;
    }

    void Play(const SoundResource& resource, float volume, float pan, float pitch) {
        auto sound = LoadSound(resource);
        if (!sound) return;
        sound->Play(volume, pitch, pan);
    }

    // Specify LFE level distance curve such that it rolls off much sooner than
    // all non-LFE channels, making use of the subwoofer more dramatic.
    static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
    static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

    static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
    static const X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

    void Play(const Sound3D& sound) {
        auto sfx = LoadSound(sound.Resource);
        if (!sfx) return;

        auto position = sound.Position * AUDIO_SCALE;

        {
            std::scoped_lock lock(SoundInstancesMutex);

            // Check if any emitters are already playing this sound from this source
            for (auto& instance : SoundInstances) {
                if (instance.Source == sound.Source &&
                    instance.Resource.GetID() == sound.Resource.GetID() &&
                    instance.StartTime + MERGE_WINDOW > Game::ElapsedTime &&
                    !instance.Looped) {

                    if (instance.AttachToSource && sound.AttachToSource)
                        instance.AttachOffset = (instance.AttachOffset + sound.AttachOffset) / 2;

                    instance.Emitter.Position = (position + instance.Emitter.Position) / 2;
                    // only use a portion of the duplicate sound to increase volume
                    instance.Volume = std::max(instance.Volume, sound.Volume) * 1.15f;
                    fmt::print("Merged sound effect {}\n", sound.Resource.GetID());
                    return; // Don't play sounds within the merge window
                }
            }

            auto& s = SoundInstances.emplace_back(sound);
            s.Instance = sfx->CreateInstance(SoundEffectInstance_Use3D | SoundEffectInstance_ReverbUseFilters);
            s.Instance->SetVolume(sound.Volume);
            s.Instance->SetPitch(sound.Pitch);
            auto addr = (void*)&s.Instance;

            //s.Emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&c_emitter_LFE_Curve;
            //s.Emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&c_emitter_Reverb_Curve;
            s.Emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
            s.Emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
            s.Emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
            s.Emitter.CurveDistanceScaler = sound.Radius;
            s.Emitter.Position = position;
            s.Emitter.DopplerScaler = 1.0f;
            s.Emitter.InnerRadius = sound.Radius / 6;
            s.Emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
            s.Emitter.pCone = (X3DAUDIO_CONE*)&c_emitterCone;

            s.StartTime = Game::ElapsedTime;
        }
    }

    void Reset() {
        if (!Engine) return;
        std::scoped_lock lock(ResetMutex);
        SPDLOG_INFO("Clearing audio cache");
        //SoundsD1.clear(); // unknown if effects must be stopped before releasing
        Stop3DSounds();
        SoundInstances.clear();
        Engine->TrimVoicePool();

        //for (auto& sound : Sounds)
        //    sound.release(); // unknown if effects must be stopped before releasing
    }

    void PrintStatistics() {
        if (!Engine) return;
        auto stats = Engine->GetStatistics();

        SPDLOG_INFO("Audio stats:\nPlaying: {} / {}\nInstances: {}\nVoices {} / {} / {} / {}\n{} audio bytes",
                    stats.playingOneShots, stats.playingInstances,
                    stats.allocatedInstances,
                    stats.allocatedVoices, stats.allocatedVoices3d,
                    stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
                    stats.audioBytes);
    }

    void Pause() { Engine->Suspend(); }
    void Resume() { Engine->Resume(); }

    float GetVolume() { return Alive ? Engine->GetMasterVolume() : 0; }
    void SetVolume(float volume) { if (Alive) Engine->SetMasterVolume(volume); }

    void Stop3DSounds() {
        if (!Alive) return;

        std::scoped_lock lock(SoundInstancesMutex);
        for (auto& sound : SoundInstances) {
            sound.Instance->Stop();
        }
    }

    void Stop2DSounds() {
        //for (auto& effect : SoundEffects) {

        //}
    }
}