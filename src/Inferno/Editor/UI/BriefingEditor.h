#pragma once

#include "WindowBase.h"
#include "Mission.h"
#include "WindowsDialogs.h"
#include "Game.h"
#include "Briefing.h"

namespace Inferno::Editor {
    class BriefingEditor : public WindowBase {
        int _txbIndex = 0;
        string _buffer;
        static constexpr int BUFFER_SIZE = 2048 * 10;
    public:
        BriefingEditor() : WindowBase("Briefing Editor", &Settings::Windows.BriefingEditor) {
            _buffer.resize(BUFFER_SIZE);
        }

    protected:
        void OnUpdate() override {

            ImGui::BeginChild("pages", { 200, 0 }, true);
            for (auto& entry : Game::Mission->Entries) {
                if (!entry.IsBriefing() || !entry.Index) continue;
                if (ImGui::Selectable(entry.Name.c_str(), _txbIndex == *entry.Index, ImGuiSelectableFlags_AllowDoubleClick)) {
                    _txbIndex = *entry.Index;
                    if (ImGui::IsMouseDoubleClicked(0))
                        OpenBriefing(entry);
                }
            }

            ImGui::EndChild();

            ImGui::SameLine();

            {
                ImGui::BeginGroup();

                ImGui::BeginChild("editor", { 0, 0 }, true);
                ImGui::InputTextMultiline("##editor", _buffer.data(), BUFFER_SIZE, { -1, -1 }, ImGuiInputTextFlags_AllowTabInput);
                ImGui::EndChild();

                ImGui::EndGroup();
            }


            /*ImGui::DragFloat3("Strength", &_strength.x, 1, 0, 100, "%.2f");
            ImGui::HelpMarker("Maximum amount of movement on each axis");

            ImGui::DragFloat("Scale", &_scale, 1.0f, 1, 1000, "%.2f");
            ImGui::HelpMarker("A higher scale creates larger waves\nwith less localized noise");

            ImGui::DragInt("Seed", &_seed);
            ImGui::Checkbox("Random Seed", &_randomSeed);

            if (ImGui::Button("Apply", { 100, 0 })) {
                Commands::ApplyNoise(_scale, _strength, _seed);
                if (_randomSeed) _seed = rand();
            }*/
        }

        void OpenBriefing(HogEntry& entry) {
           auto data = Game::Mission->ReadEntry(entry);
           auto briefing = Briefing::Read(data);
           _buffer = briefing.Raw;
        }
    };
}