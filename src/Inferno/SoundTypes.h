#pragma once
#include "Types.h"

namespace Inferno {
    constexpr ObjRef GLOBAL_SOUND_SOURCE = { ObjID(9999), ObjSig(9999) }; // Assign the source to this value to have it culled against all others
    constexpr float DEFAULT_SOUND_RADIUS = 250;
    enum class SoundUID : unsigned int { None = 0 }; // ID used to cancel a playing sound

    // Handle to a sound resource
    struct SoundResource {
        SoundResource() = default;
        SoundResource(SoundID);
        SoundResource(string);

        int D1 = -1; // Index to PIG data
        int D2 = -1; // Index to S22 data
        string D3; // D3 file name or system path

        // Priority is D3, D1, D2
        bool operator== (const SoundResource& rhs) const {
            if (!D3.empty() && !rhs.D3.empty()) return D3 == rhs.D3;
            if (D1 != -1 && rhs.D1 != -1) return D1 == rhs.D1;
            return D2 == rhs.D2;
        }

        float GetDuration() const;
    };

    struct Sound2D {
        SoundResource Resource;
        float Volume = 1;
        float Pitch = 0;
    };

    struct Sound3D {
        Sound3D(SoundResource resource, ObjRef source) : Resource(std::move(resource)), Source(source) {}
        Sound3D(SoundResource resource, const Vector3& pos, SegID seg) : Resource(std::move(resource)), Position(pos), Segment(seg) {}

        SoundResource Resource;
        Vector3 Position; // Position the sound comes from
        SegID Segment = SegID::None; // Segment the sound starts in, needed for occlusion
        SideID Side = SideID::None; // Side, used for turning of forcefields
        ObjRef Source = GLOBAL_SOUND_SOURCE; // Source to attach the sound to
        float Volume = 1;
        float Pitch = 0; // -1 to 1;
        bool Occlusion = true; // Occludes level geometry when determining volume
        float Radius = DEFAULT_SOUND_RADIUS; // Determines max range and falloff
        bool AttachToSource = false; // The sound moves with the Source object
        Vector3 AttachOffset; // The offset from the Source when attached
        bool FromPlayer = false; // For the player's firing sounds, afterburner, etc
        bool Merge = true; // Merge with other sounds played in a similar timeframe
        SoundUID ID = SoundUID::None;
        bool Looped = false;
        uint32 LoopCount = 0;
        uint32 LoopStart = 0;
        uint32 LoopEnd = 0;
    };
}