#pragma once

#include <DirectXTK12/Keyboard.h>

namespace Inferno::Input {
    using Keys = DirectX::Keyboard::Keys;

    enum class MouseButtons : uint8_t {
        None,
        Left,
        Right,
        Middle,
        X1,
        X2,
        WheelUp,
        WheelDown
    };

    inline DirectX::SimpleMath::Vector2 MouseDelta;
    inline DirectX::SimpleMath::Vector2 MousePosition;
    inline DirectX::SimpleMath::Vector2 DragStart; // Mouse drag start position in screen coordinates

    int GetWheelDelta();
    //inline bool ScrolledUp() { return GetWheelDelta() > 0; }
    //inline bool ScrolledDown() { return GetWheelDelta() < 0; }

    inline bool ControlDown;
    inline bool ShiftDown;
    inline bool AltDown;
    inline bool HasFocus = true; // Window has focus

    void Update();
    void Initialize(HWND);

    // Returns true while a key is held down
    bool IsKeyDown(Keys);

    // Returns true when a key is first pressed
    bool IsKeyPressed(Keys);

    // Returns true when a key is first released
    bool IsKeyReleased(Keys);

    // Returns true while a key is held down
    bool IsMouseButtonDown(MouseButtons);

    // Returns true when a key is first pressed
    bool IsMouseButtonPressed(MouseButtons);

    // Returns true when a key is first released
    bool IsMouseButtonReleased(MouseButtons);

    void ResetState();

    void NextFrame();

    enum class SelectionState {
        None,
        Preselect, // Mouse button pressed
        BeginDrag, // Fires after preselect and the cursor moves
        Dragging, // Mouse is moving with button down
        ReleasedDrag, // Mouse button released after dragging
        Released // Button released. Does not fire if dragging
    };

    inline SelectionState DragState, LeftDragState, RightDragState;

    enum class MouseMode {
        Normal,
        Mouselook,
        Orbit
    };

    MouseMode GetMouseMode();
    void SetMouseMode(MouseMode);

    void ProcessMessage(UINT message, WPARAM, LPARAM);

    string KeyToString(Keys key);

    enum class EventType {
        KeyPress,
        KeyRelease,
        MouseBtnPress,
        MouseBtnRelease,
        MouseWheel,
        Reset
    };

    void QueueEvent(EventType type, WPARAM keyCode = 0, int64_t flags = 0);
}
