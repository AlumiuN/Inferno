#pragma once

#include "Input.h"
#include "Types.h"
#include "Utility.h"

namespace Inferno {
    // Bindable in-game actions
    enum class GameAction {
        None,
        SlideLeft,
        SlideRight,
        SlideUp,
        SlideDown,
        Forward,
        Reverse,
        RollLeft,
        RollRight,
        PitchUp,
        PitchDown,
        YawLeft,
        YawRight,
        Afterburner,

        FirePrimary,
        FireSecondary,

        FireOnceEventIndex, // Actions past this index are only fired on button down

        FireFlare,
        DropBomb,

        CyclePrimary,
        CycleSecondary,
        CycleBomb,

        Automap,
        Headlight,
        Converter,
        Count
    };

    struct GameCommand {
        GameAction Id;
        std::function<void()> Action;
    };

    struct GameBinding {
        GameAction Action = GameAction::None;
        Input::Keys Key = Input::Keys::None;
        Input::MouseButtons Mouse = Input::MouseButtons::None;
        // Gamepad / Joystick?

        string GetShortcutLabel() const;

        //bool operator==(const GameBinding& rhs) const {
        //    return Key == rhs.Key || Mouse == rhs.Mouse;
        //}
    };


    class GameBindings {
        List<GameBinding> _bindings;
        Array<bool, (uint)GameAction::Count> _state;

    public:
        GameBindings() { Reset(); }

        // Adds a new binding and unbinds any existing actions using the same shortcut
        void Bind(GameBinding binding);
        span<GameBinding> GetBindings() { return _bindings; }

        void UnbindExisting(const GameBinding& binding) {
            for (auto& b : _bindings) {
                if (b.Key == binding.Key)
                    b.Key = Input::Keys::None;

                if (b.Mouse == binding.Mouse)
                    b.Mouse = Input::MouseButtons::None;
            }
        }

        void Update() {
            // Clear state
            ranges::fill(_state, false);

            // If any bindings are down for the action, set it as true
            for (auto& binding : _bindings) {
                if (binding.Action > GameAction::FireOnceEventIndex) {
                    if (Input::IsKeyPressed(binding.Key) || Input::IsMouseButtonPressed(binding.Mouse))
                        _state[(uint)binding.Action] = true;
                }
                else {
                    if (Input::IsKeyDown(binding.Key) || Input::IsMouseButtonDown(binding.Mouse))
                        _state[(uint)binding.Action] = true;
                }
            }
        }

        bool Pressed(GameAction action) const {
            return _state[(uint)action];
        }

        void Reset();

        //bool IsReservedKey(Input::Keys);
    };

    namespace Game {
        inline GameBindings Bindings;
    }
}
