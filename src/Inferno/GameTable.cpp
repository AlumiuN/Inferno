﻿#include "pch.h"
#include <fstream>

#include "HamFile.h"
#include "Yaml.h"
#include "Graphics/Render.Particles.h"

namespace Inferno {
    template <class T>
    bool ReadArray(ryml::NodeRef node, span<T> values) {
        if (!node.valid() || node.is_seed()) return false;

        if (node.has_children()) {
            // Array of values
            int i = 0;

            for (const auto& child : node.children()) {
                if (i >= values.size()) break;
                Yaml::ReadValue(child, values[i++]);
            }
        }
        else if (node.has_val()) {
            // Single value
            T value{};
            Yaml::ReadValue(node, value);
            for (auto& v : values)
                v = value;
        }
        return true;
    }

    //void ReadVectorArray(ryml::NodeRef node, span<Vector3> values) {
    //    if (!node.valid() || node.is_seed()) return;

    //    if (node.has_children()) {
    //        // Array of values
    //        int i = 0;

    //        for (const auto& child : node.children()) {
    //            if (i >= values.size()) break;
    //            Yaml::ReadValue(child, values[i++]);
    //        }
    //    }
    //    else if (node.has_val()) {
    //        // Single value
    //        Vector3 value;
    //        Yaml::ReadValue(node, value);
    //        for (auto& v : values)
    //            v = value;
    //    }
    //}

    template <class T>
    void ReadRange(ryml::NodeRef node, NumericRange<T>& values) {
        if (!node.valid() || node.is_seed()) return;

        if (node.has_children()) {
            // Array of values
            int i = 0;
            T children[2] = {};

            for (const auto& child : node.children()) {
                if (i >= 2) break;
                Yaml::ReadValue(child, children[i++]);
            }

            if (i == 1) values = { children[0], children[0] };
            if (i == 2) values = { children[0], children[1] };
        }
        else if (node.has_val()) {
            // Single value
            T value{};
            Yaml::ReadValue(node, value);
            values = { value, value };
        }
    }

    void ReadWeaponInfo(ryml::NodeRef node, HamFile& ham, int& id) {
        Yaml::ReadValue(node["id"], id);
        if (!Seq::inRange(ham.Weapons, id)) return;

        auto& weapon = ham.Weapons[id];
#define READ_PROP(name) Yaml::ReadValue(node[#name], weapon.##name)
        Yaml::ReadValue(node["RenderType"], (int&)weapon.RenderType);
        READ_PROP(Thrust);

        READ_PROP(Drag);
        READ_PROP(Mass);
        READ_PROP(AmmoUsage);
        READ_PROP(EnergyUsage);
        READ_PROP(ModelSizeRatio);
        READ_PROP(WallHitSound);
        READ_PROP(WallHitVClip);
        READ_PROP(FireDelay);
        READ_PROP(Lifetime);
        READ_PROP(FireCount);

        READ_PROP(BlobSize);
        Yaml::ReadValue(node["BlobBitmap"], (int&)weapon.BlobBitmap);

        READ_PROP(ImpactSize);
        READ_PROP(SplashRadius);
        READ_PROP(TrailSize);

        READ_PROP(FlashSize);
        Yaml::ReadValue(node["FlashVClip"], (int&)weapon.FlashVClip);
        Yaml::ReadValue(node["FlashSound"], (int&)weapon.FlashSound);

        READ_PROP(FlashStrength);
#undef READ_PROP
        Yaml::ReadValue(node["Model"], (int&)weapon.Model);

        ReadArray<float>(node["Damage"], weapon.Damage);
        ReadArray<float>(node["Speed"], weapon.Speed);

#define READ_PROP_EXT(name) Yaml::ReadValue(node[#name], weapon.Extended.##name)
        READ_PROP_EXT(FlashColor);
        READ_PROP_EXT(Name);
        READ_PROP_EXT(Behavior);
        READ_PROP_EXT(Glow);
        READ_PROP_EXT(ModelName);
        READ_PROP_EXT(ModelScale);
        READ_PROP_EXT(Size);
        READ_PROP_EXT(Chargable);
        READ_PROP_EXT(Spread);

        READ_PROP_EXT(Decal);
        READ_PROP_EXT(DecalRadius);

        READ_PROP_EXT(ExplosionSize);
        READ_PROP_EXT(ExplosionSound);
        READ_PROP_EXT(ExplosionTexture);
        READ_PROP_EXT(ExplosionTime);

        READ_PROP_EXT(RotationalVelocity);
        READ_PROP_EXT(Bounces);
        READ_PROP_EXT(Sticky);

        READ_PROP_EXT(LightRadius);
        READ_PROP_EXT(LightColor);
        Yaml::ReadValue(node["LightMode"], (int&)weapon.Extended.LightMode);
        READ_PROP_EXT(ExplosionColor);
        READ_PROP_EXT(InheritParentVelocity);
#undef READ_PROP_EXT
    }

    void ReadPowerupInfo(ryml::NodeRef node, HamFile& ham, int& id) {
        Yaml::ReadValue(node["id"], id);
        if (!Seq::inRange(ham.Powerups, id)) return;

        auto& powerup = ham.Powerups[id];
        Yaml::ReadValue(node["LightRadius"], powerup.LightRadius);
        Yaml::ReadValue(node["LightColor"], powerup.LightColor);
        Yaml::ReadValue(node["LightMode"], (int&)powerup.LightMode);
        Yaml::ReadValue(node["Glow"], powerup.Glow);
    }

    Option<string> ReadEffectName(ryml::NodeRef node) {
        string name;
        Yaml::ReadValue(node["Name"], name);
        if (name.empty()) {
            SPDLOG_WARN("Found effect with no name!");
            return {};
        }

        return name;
    }

    void ReadBeamInfo(ryml::NodeRef node, Dictionary<string, Render::BeamInfo>& beams) {
        Render::BeamInfo info{};

#define READ_PROP(name) Yaml::ReadValue(node[#name], info.##name)
        ReadRange(node["Radius"], info.Radius);
        ReadRange(node["Width"], info.Width);
        READ_PROP(Color);
        READ_PROP(Texture);
        READ_PROP(Frequency);
        READ_PROP(StrikeTime);
        READ_PROP(Amplitude);
        READ_PROP(Life);
        READ_PROP(Scale);
        READ_PROP(FadeInOutTime);
#undef READ_PROP

        bool fadeEnd = false, randomEnd = false, fadeStart = false;
        bool randomObjStart = false, randomObjEnd = false;
        Yaml::ReadValue(node["FadeEnd"], fadeEnd);
        Yaml::ReadValue(node["FadeStart"], fadeStart);
        Yaml::ReadValue(node["RandomEnd"], randomEnd);
        Yaml::ReadValue(node["RandomObjStart"], randomObjStart);
        Yaml::ReadValue(node["RandomObjEnd"], randomObjEnd);
        SetFlag(info.Flags, Render::BeamFlag::FadeEnd, fadeEnd);
        SetFlag(info.Flags, Render::BeamFlag::FadeStart, fadeStart);
        SetFlag(info.Flags, Render::BeamFlag::RandomEnd, randomEnd);
        SetFlag(info.Flags, Render::BeamFlag::RandomObjStart, randomObjStart);
        SetFlag(info.Flags, Render::BeamFlag::RandomObjEnd, randomObjEnd);

        if (auto name = ReadEffectName(node))
            beams[*name] = info;
    }

    void ReadSparkInfo(ryml::NodeRef node, Dictionary<string, Render::SparkEmitter>& sparks) {
        Render::SparkEmitter info;

#define READ_PROP(name) Yaml::ReadValue(node[#name], info.##name)
        READ_PROP(Color);
        READ_PROP(Restitution);
        READ_PROP(Texture);
        READ_PROP(Width);
        READ_PROP(FadeTime);
        READ_PROP(Drag);
        READ_PROP(VelocitySmear);
        READ_PROP(Duration);
        READ_PROP(SpawnRadius);
        READ_PROP(UseWorldGravity);
        READ_PROP(UsePointGravity);
        READ_PROP(PointGravityStrength);
        READ_PROP(PointGravityVelocity);
        READ_PROP(PointGravityOffset);
        READ_PROP(Offset);
        READ_PROP(FadeSize);
        ReadRange(node["SparkDuration"], info.SparkDuration);
        ReadRange(node["Velocity"], info.Velocity);
        ReadRange(node["Count"], info.Count);
#undef READ_PROP

        if (auto name = ReadEffectName(node))
            sparks[*name] = info;
    }

    void ReadExplosions(ryml::NodeRef node, Dictionary<string, Render::ExplosionInfo>& explosions) {
        Render::ExplosionInfo info;
        Yaml::ReadValue(node["Instances"], info.Instances);
        Yaml::ReadValue(node["FadeTime"], info.FadeTime);
        ReadRange(node["Radius"], info.Radius);
        ReadRange(node["Delay"], info.Delay);
        Yaml::ReadValue(node["Clip"], (int&)info.Clip);
        Yaml::ReadValue(node["Sound"], (int&)info.Sound);
        Yaml::ReadValue(node["Volume"], info.Volume);

        if (auto name = ReadEffectName(node))
            explosions[*name] = info;
    }

    void ReadTracers(ryml::NodeRef node, Dictionary<string, Render::TracerInfo>& tracers) {
        Render::TracerInfo info;
        Yaml::ReadValue(node["Length"], info.Length);
        Yaml::ReadValue(node["Width"], info.Width);
        Yaml::ReadValue(node["Texture"], info.Texture);
        Yaml::ReadValue(node["BlobTexture"], info.BlobTexture);
        Yaml::ReadValue(node["Color"], info.Color);
        Yaml::ReadValue(node["FadeSpeed"], info.FadeSpeed);
        Yaml::ReadValue(node["Duration"], info.Duration);

        if (auto name = ReadEffectName(node))
            tracers[*name] = info;
    }

    void ReadRobotInfo(ryml::NodeRef node, HamFile& ham, int& id) {
        Yaml::ReadValue(node["id"], id);
        if (!Seq::inRange(ham.Robots, id)) return;

        auto& robot = ham.Robots[id];
        ReadArray<Vector3>(node["GunPoints"], robot.GunPoints);
        ReadArray<ubyte>(node["GunSubmodels"], robot.GunSubmodels);

#define READ_TAG(name) Yaml::ReadValue(node[#name], (int&)robot.##name)
#define READ_PROP(name) Yaml::ReadValue(node[#name], robot.##name)
        READ_TAG(Model);
        READ_TAG(ExplosionClip1);
        READ_TAG(ExplosionClip2);
        Yaml::ReadValue(node["WeaponType"], (int&)robot.WeaponType);
        Yaml::ReadValue(node["WeaponType2"], (int&)robot.WeaponType2);
        READ_PROP(Guns);

        // todo: contains data
        READ_PROP(ContainsChance);

        READ_PROP(Kamikaze);
        READ_PROP(Score);
        READ_PROP(Badass);
        READ_PROP(EnergyDrain);
        READ_PROP(Lighting);
        READ_PROP(HitPoints);
        READ_PROP(Mass);
        READ_PROP(Drag);

        READ_TAG(Cloaking);
        READ_TAG(Attack);

        READ_TAG(ExplosionSound1);
        READ_TAG(ExplosionSound2);
        READ_TAG(SeeSound);
        READ_TAG(AttackSound);
        READ_TAG(ClawSound);
        READ_TAG(TauntSound);
        READ_TAG(DeathrollSound);

        READ_PROP(IsThief);
        READ_PROP(Pursues);
        READ_PROP(LightCast);
        READ_PROP(DeathRoll);
        READ_PROP(Flags);
        READ_PROP(Glow);
        READ_PROP(Behavior);
        READ_PROP(Aim);
        READ_PROP(Multishot);

        Array<float, 5> fov{}, fireDelay{}, fireDelay2{}, turnTime{}, speed{}, circleDistance{};
        Array<int16, 5> shots{}, evasion{};

        bool hasFov = ReadArray<float>(node["FOV"], fov);
        for (auto& f : fov) {
            f *= DegToRad; // Convert FOV to radians (0 to 2PI)
            f = (f - DirectX::XM_PI) / DirectX::XM_PI; // [-PI, PI] to [-1, 1]
            f = std::clamp(f, -1.0f, 1.0f);
        }

        bool hasFireDelay = ReadArray<float>(node["FireDelay"], fireDelay);
        bool hasFireDelay2 = ReadArray<float>(node["FireDelay2"], fireDelay2);
        bool hasTurnTime = ReadArray<float>(node["TurnTime"], turnTime);
        bool hasSpeed = ReadArray<float>(node["Speed"], speed);
        bool hasCircleDist = ReadArray<float>(node["CircleDistance"], circleDistance);
        bool hasShots = ReadArray<int16>(node["Shots"], shots);
        bool hasEvasion = ReadArray<int16>(node["Evasion"], evasion);

        for (int i = 0; i < 5; i++) {
            auto& diff = robot.Difficulty[i];
            if (hasCircleDist) diff.CircleDistance = circleDistance[i];
            if (hasFireDelay) diff.FireDelay = fireDelay[i];
            if (hasFireDelay2) diff.FireDelay2 = fireDelay2[i];
            if (hasEvasion) diff.EvadeSpeed = (uint8)evasion[i];
            if (hasShots) diff.ShotCount = (uint8)shots[i];
            if (hasSpeed) diff.Speed = speed[i];
            if (hasTurnTime) diff.TurnTime = turnTime[i];
            if (hasFov) diff.FieldOfView = fov[i];
        }

#undef READ_PROP
#undef READ_TAG
    }

    void LoadGameTable(filesystem::path path, HamFile& ham) {
        try {
            std::ifstream file(path);
            if (!file) {
                SPDLOG_ERROR(L"Unable to open game table `{}`", path.c_str());
                return;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            ryml::Tree doc = ryml::parse_in_arena(ryml::to_csubstr(buffer.str()));
            ryml::NodeRef root = doc.rootref();

            if (!root.is_map()) {
                SPDLOG_WARN(L"Game table `{}` is empty", path.c_str());
                return;
            }

            Render::EffectLibrary = {}; // Reset effect library

            if (auto weapons = root["Weapons"]; !weapons.is_seed()) {
                for (const auto& weapon : weapons.children()) {
                    int id = -1;
                    try {
                        ReadWeaponInfo(weapon, ham, id);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading weapon {}\n{}", id, e.what());
                    }
                }
            }

            if (auto robots = root["Robots"]; !robots.is_seed()) {
                for (const auto& robot : robots.children()) {
                    int id = -1;
                    try {
                        ReadRobotInfo(robot, ham, id);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading robot {}\n{}", id, e.what());
                    }
                }
            }

            if (auto powerups = root["Powerups"]; !powerups.is_seed()) {
                for (const auto& powerup : powerups.children()) {
                    int id = -1;
                    try {
                        ReadPowerupInfo(powerup, ham, id);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading powerup {}\n{}", id, e.what());
                    }
                }
            }

            auto effects = root["Effects"];

            if (auto beams = effects["Beams"]; !beams.is_seed()) {
                for (const auto& beam : beams.children()) {
                    try {
                        ReadBeamInfo(beam, Render::EffectLibrary.Beams);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading beam info", e.what());
                    }
                }
                SPDLOG_INFO("Loaded {} beams", Render::EffectLibrary.Beams.size());
            }


            if (auto sparks = effects["Sparks"]; !sparks.is_seed()) {
                for (const auto& beam : sparks.children()) {
                    try {
                        ReadSparkInfo(beam, Render::EffectLibrary.Sparks);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading spark info", e.what());
                    }
                }
                SPDLOG_INFO("Loaded {} sparks", Render::EffectLibrary.Sparks.size());
            }

            if (auto explosions = effects["Explosions"]; !explosions.is_seed()) {
                for (const auto& beam : explosions.children()) {
                    try {
                        ReadExplosions(beam, Render::EffectLibrary.Explosions);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading explosion info", e.what());
                    }
                }
                SPDLOG_INFO("Loaded {} explosions", Render::EffectLibrary.Explosions.size());
            }

            if (auto tracers = effects["Tracers"]; !tracers.is_seed()) {
                for (const auto& beam : tracers.children()) {
                    try {
                        ReadTracers(beam, Render::EffectLibrary.Tracers);
                    }
                    catch (const std::exception& e) {
                        SPDLOG_WARN("Error reading tracer info", e.what());
                    }
                }
                SPDLOG_INFO("Loaded {} tracers", Render::EffectLibrary.Tracers.size());
            }
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("Error loading game table:\n{}", e.what());
        }
    }
}
