#pragma once

#include "imgui_local.h"
#include "Game.h"
#include "Graphics/Render.h"

namespace Inferno {
    // Performance overlay
    inline void DrawDebugOverlay(const ImVec2& pos, const ImVec2& pivot) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0, 0, 0, 0.5f });

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("Debug Overlay", nullptr, flags)) {
            static Array<float, 90> values = {};
            static int values_offset = 0;
            static double refresh_time = 0.0;
            static int usedValues = 0;

            if (refresh_time == 0.0)
                refresh_time = Game::ElapsedTime;

            while (refresh_time < Game::ElapsedTime) {
                values[values_offset] = Render::FrameTime;
                values_offset = (values_offset + 1) % values.size();
                refresh_time += 1.0f / 60.0f;
                usedValues = std::min((int)values.size(), usedValues + 1);
            }

            {
                float average = 0.0f;
                for (int n = 0; n < usedValues; n++)
                    average += values[n];

                if (usedValues > 0) average /= (float)usedValues;
                auto overlay = fmt::format("FPS {:.1f} ({:.2f} ms)  Calls: {:d}", 1 / average, average * 1000, Render::DrawCalls);
                ImGui::PlotLines("##FrameTime", values.data(), (int)values.size(), values_offset, overlay.c_str(), 0, 1 / 20.0f, ImVec2(0, 120.0f));
            }
        }
        ImGui::End();

        ImGui::PopStyleColor();
    }

    // Player ship info, rooms, AI, etc
    inline void DrawGameDebugOverlay(const ImVec2& pos, const ImVec2& pivot) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::PushStyleColor(ImGuiCol_Border, { 0, 0, 0, 0 });

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("Game Debug Overlay", nullptr, flags)) {
            if (auto player = Game::Level.TryGetObject(ObjID(0))) {
                ImGui::Text("Segment: %d", player->Segment);
                ImGui::Text("Room type: Normal");
                ImGui::Text("Ship vel: %.2f, %.2f, %.2f", Debug::ShipVelocity.x, Debug::ShipVelocity.y, Debug::ShipVelocity.z);

            }
        }
        ImGui::End();

        ImGui::PopStyleColor();
    }
}
