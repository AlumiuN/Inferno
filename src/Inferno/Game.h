#pragma once
#include "Level.h"
#include "HogFile.h"
#include "Mission.h"
#include "Game.Player.h"

namespace Inferno {
    enum class GameState { Game, Editor, Paused };
}

namespace Inferno::Game {
    constexpr float TICK_RATE = 1.0f / 64; // 64 ticks per second (homing missiles use 32 ticks per second)

    inline int Difficulty = 0; // 0 to 4 for trainee to insane

    inline GameState State = GameState::Editor;
    inline Vector3 Gravity = { 0, -10, 0 };

    // The loaded level. Only one level can be active at a time.
    inline Inferno::Level Level;

    // The loaded mission. Not always present.
    inline Option<HogFile> Mission;

    // Only single player for now
    inline struct Player Player = {};

    // is the game level loading?
    inline std::atomic IsLoading = false;

    void LoadLevel(Inferno::Level&&);

    void LoadMission(const filesystem::path& file);

    inline void UnloadMission() {
        Mission = {};
    }

    // Tries to read the mission file (msn / mn2) for the loaded mission
    Option<MissionInfo> TryReadMissionInfo();

    inline double Time = 0; // Elapsed game time in seconds. Stops when paused.
    inline float DeltaTime = 0; // Elapsed game time since last update. 0 when paused.
    inline float LerpAmount = 1; // How much to lerp between the previous and next object states

    void FireWeapon(ObjID objId, int gun, WeaponID id, bool showFlash = true, const Vector2& spread = Vector2::Zero);

    // Detonates a weapon with a splash radius
    void ExplodeWeapon(Object&);

    void Update(float dt);
    void ToggleEditorMode();

    // Finds the nearest object ID to an object
    Tuple<ObjID, float> FindNearestObject(const Object&);
    void UpdateWeapon(Object&, float dt);

    inline bool ShowDebugOverlay = false;

    inline bool SecretLevelDestroyed = false;

    // Schedules an object to be added at end of update
    void AddObject(const Object&);

    /*inline bool ObjShouldThink(const Object& obj) {
        return obj.NextThinkTime <= Time && obj.NextThinkTime != -1;
    }*/

    // Returns true if the provided time has come to pass
    inline bool TimeHasElapsed(float time) {
        return time <= Time && time != -1;
    }

    using GunIndex = int;
    using WeaponBehavior = std::function<void(Inferno::Player&, GunIndex, WeaponID)>;
    WeaponBehavior& GetWeaponBehavior(const string& name);

    constexpr float DOOR_WAIT_TIME = 5; // How long a door stays open
    constexpr float MINE_ARM_TIME = 2.0f; // How long before player can shoot or be hit by their own mines

    // 255 marks where weapons aren't considered for autoselection

    constexpr Array<uint8, 11> DefaultPrimaryPriority{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 255 };
    constexpr Array<uint8, 11> DefaultSecondaryPriority{ 9, 8, 4, 3, 1, 5, 0, 255, 7, 6, 2 };

    inline Array<uint8, 11> PrimaryPriority = DefaultPrimaryPriority;
    inline Array<uint8, 11> SecondaryPriority = DefaultSecondaryPriority;
}
