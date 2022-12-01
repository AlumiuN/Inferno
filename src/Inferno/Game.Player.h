#pragma once

#include "Player.h"
#include "Resources.h"

namespace Inferno {
    enum class FireState { None, Press, Hold, Release };

    constexpr float MAX_ENERGY = 200;
    constexpr float MAX_SHIELDS = 200;
    constexpr float OMEGA_CHARGE_COST = 1.0f / 8; // charge cost to fire one shot of omega
    constexpr float OMEGA_RECHARGE_TIME = 2; // time to fully recharge omega
    constexpr float OMEGA_RECHARGE_ENERGY = 4; // energy to fully recharge omega
    constexpr float OMEGA_RECHARGE_DELAY = 1.0f / 4; // how long before recharging starts

    // Extracted player state that was scattered across methods or globals as static variables
    struct Player : PlayerData {
        const float RearmTime = 1.0f; // Time to swap between weapons and being able to fire

        PrimaryWeaponIndex Primary = PrimaryWeaponIndex::Laser;
        SecondaryWeaponIndex Secondary = SecondaryWeaponIndex::Concussion;
        bool PrimaryWasSuper[10]{};
        bool SecondaryWasSuper[10]{};

        float PrimarySwapTime = 0; // Primary weapon is changing. Used to fade monitor contents.
        float SecondarySwapTime = 0; // Secondary weapon is changing. Used to fade monitor contents.
        float WeaponCharge = 0; // How long weapon has been charging (held down)
        float OmegaCharge = 1; // How much charge the omega has stored
        float OmegaRechargeDelay = 0; // Delay before Omega starts recharging after firing
        float FlareDelay = 0;
        float PrimaryDelay = 0;
        float SecondaryDelay = 0;
        float AfterburnerCharge = 1; // 0 to 1
        bool HasSpew = false; // has dropped items on death
        bool SpawnInvuln = false; // temporary invuln when spawning
        bool LavafallHissPlaying = false; // checks if a lavafall (or waterfall) sound is already playing
        bool SpreadfireToggle = false; // horizontal / vertical
        uint8 HelixOrientation = 0; // increments in 22.5 degrees
        float FusionNextSoundDelay = 0;
        uint8 FiringIndex = 0, MissileFiringIndex = 0;

        FireState PrimaryState{}, SecondaryState{};
        float RefuelSoundTime = 0;
        bool AfterburnerActive = false;
        int BombIndex = 0; // 0 is proxy, 1 is smart mine
        double LastPrimaryFireTime = 0;

        void GiveWeapon(PrimaryWeaponIndex weapon) {
            PrimaryWeapons |= (1 << (uint16)weapon);
            if (weapon == PrimaryWeaponIndex::Vulcan || weapon == PrimaryWeaponIndex::Gauss)
                PrimaryAmmo[(int)PrimaryWeaponIndex::Vulcan] += 2500;
        }

        void GiveWeapon(SecondaryWeaponIndex weapon) {
            SecondaryWeapons |= (1 << (uint16)weapon);
            SecondaryAmmo[(uint16)weapon]++;
        }

        bool HasWeapon(PrimaryWeaponIndex weapon) const {
            return PrimaryWeapons & (1 << (uint16)weapon);
        }

        SecondaryWeaponIndex GetActiveBomb() const;

        void AddEnergy(float energy) {
            Energy += energy;
            Energy = std::clamp(Energy, 0.0f, MAX_ENERGY);
        }

        void CycleBombs();

        void DropBomb();

        //bool HasWeapon(SecondaryWeaponIndex weapon) const {
        //    return SecondaryWeapons & (1 << (uint16)weapon);
        //}

        void GivePowerup(PowerupFlag powerup) {
            SetFlag(Powerups, powerup);
        }

        bool HasPowerup(PowerupFlag powerup) const {
            return HasFlag(Powerups, powerup);
        }

        void RemovePowerup(PowerupFlag powerup) {
            ClearFlag(Powerups, powerup);
        }

        void TouchPowerup(Object& obj);
        void TouchObject(Object& obj);

        // Gives energy and returns true if able to pick up a powerup
        bool PickUpEnergy();
        bool PickUpPrimary(PrimaryWeaponIndex);
        bool PickUpSecondary(SecondaryWeaponIndex, uint16 count = 1);
        // Returns the amount of ammo picked up
        int PickUpAmmo(PrimaryWeaponIndex, uint16 amount);

        bool CanFirePrimary(PrimaryWeaponIndex index) const {
            if (!HasWeapon(index)) return false;

            auto& weapon = Resources::GetWeapon(GetPrimaryWeaponID(index));
            bool canFire = true;

            if (index == PrimaryWeaponIndex::Vulcan ||
                index == PrimaryWeaponIndex::Gauss)
                canFire &= weapon.AmmoUsage <= PrimaryAmmo[(int)PrimaryWeaponIndex::Vulcan];

            if (index == PrimaryWeaponIndex::Omega)
                canFire &= Energy > 1 || OmegaCharge > OMEGA_CHARGE_COST;  // it's annoying to switch to omega with no energy

            canFire &= weapon.EnergyUsage <= Energy;
            return canFire;
        }

        bool CanFireSecondary(SecondaryWeaponIndex index) const {
            auto& weapon = Resources::GetWeapon(GetSecondaryWeaponID(index));

            return
                weapon.AmmoUsage <= SecondaryAmmo[(int)index] &&
                weapon.EnergyUsage <= Energy;
        }

        // returns the forward thrust multiplier
        float UpdateAfterburner(float dt, bool active);

        void SelectPrimary(PrimaryWeaponIndex index);
        void SelectSecondary(SecondaryWeaponIndex index);

        void Update(float dt);

        void FireFlare();
        void FirePrimary();
        void HoldPrimary();
        void ReleasePrimary();

        void FireSecondary();

        bool CanOpenDoor(const Wall& wall) const;

        void AutoselectPrimary();
        void AutoselectSecondary();
        void GiveExtraLife(uint8 lives = 1);

    private:
        SoundUID _afterburnerSoundSig = 0;
        SoundUID _fusionChargeSound = 0;
        float _prevAfterburnerCharge = 0;
        float _nextFlareFireTime = 0;

        WeaponID GetPrimaryWeaponID(PrimaryWeaponIndex index) const {
            if (index == PrimaryWeaponIndex::Laser) {
                if (LaserLevel < 4) return WeaponID{ (int)WeaponID::Laser1 + LaserLevel };
                if (LaserLevel == 4) return WeaponID::Laser5;
                if (LaserLevel == 5) return WeaponID::Laser6;
            }

            return PrimaryToWeaponID[(int)index];
        }

        static WeaponID GetSecondaryWeaponID(SecondaryWeaponIndex index) {
            return SecondaryToWeaponID[(int)index];
        }
    };
}