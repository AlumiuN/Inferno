#pragma once
#include "Level.h"
#include "Editor.Selection.h"

namespace Inferno::Editor {

    struct BezierCurve2 {
        Array<Vector3, 4> Points{};
    };

    struct PathNode {
        Matrix Rotation;
        Vector3 Position; // absolute and unrotated vertices
        Array<Vector3, 4> Vertices;
        Vector3 Axis; // axis of rotation from last node to this node
        float Angle; // rotation angle around z axis
    };

    struct TunnelNode {
        Vector3 Point;
        Vector3 Normal;
        Vector3 Up;
        Array<Vector3, 4> Vertices;
        Matrix Rotation;
    };

    struct TunnelPath {
        TunnelNode Start, End;
        List<PathNode> Nodes;
    };

    constexpr float MinTunnelLength = 10;
    constexpr float MaxTunnelLength = 200;

    struct TunnelParams {
        PointTag Start;
        PointTag End;
        int Steps = 5;
        float StartLength = 40;
        float EndLength = 40;
        bool Twist = true;

        void ClampInputs() {
            Steps = std::clamp(Steps, 1, 100);
            StartLength = std::clamp(StartLength, MinTunnelLength, MaxTunnelLength);
            EndLength = std::clamp(EndLength, MinTunnelLength, MaxTunnelLength);
        }
    };

    void CreateTunnel(Level& level, TunnelParams& params);
    void ClearTunnel();
    void CreateTunnelSegments(Level& level, const TunnelPath& path, const TunnelParams&);

    inline List<Vector3> TunnelBuilderPath;
    inline List<Vector3> TunnelBuilderPoints;
    inline List<Vector3> DebugTunnelPoints;
    inline List<Vector3> DebugTunnelLines;
    inline TunnelPath DebugTunnel;

    inline BezierCurve2 TunnelBuilderHandles; // For preview
    inline PointTag TunnelStart, TunnelEnd;
}