#include "pch.h"
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

#include "Settings.h"
#include "Yaml.h"
#include "Editor/Bindings.h"

using namespace Yaml;

namespace Inferno {
    void EditorSettings::AddRecentFile(filesystem::path path) {
        if (!filesystem::exists(path)) return;

        if (Seq::contains(RecentFiles, path))
            Seq::remove(RecentFiles, path);

        RecentFiles.push_front(path);

        while (RecentFiles.size() > MaxRecentFiles)
            RecentFiles.pop_back();
    }

    void SaveGraphicsSettings(ryml::NodeRef node, const GraphicsSettings& s) {
        node |= ryml::MAP;
        node["HighRes"] << s.HighRes;
        node["EnableBloom"] << s.EnableBloom;
        node["MsaaSamples"] << s.MsaaSamples;
        node["ForegroundFpsLimit"] << s.ForegroundFpsLimit;
        node["BackgroundFpsLimit"] << s.BackgroundFpsLimit;
        node["UseVsync"] << s.UseVsync;
        node["FilterMode"] << (int)s.FilterMode;
    }

    GraphicsSettings LoadGraphicsSettings(ryml::NodeRef node) {
        GraphicsSettings s{};
        if (node.is_seed()) return s;
        ReadValue(node["HighRes"], s.HighRes);
        ReadValue(node["EnableBloom"], s.EnableBloom);
        ReadValue(node["MsaaSamples"], s.MsaaSamples);
        if (s.MsaaSamples != 1 && s.MsaaSamples != 2 && s.MsaaSamples != 4 && s.MsaaSamples != 8)
            s.MsaaSamples = 1;

        ReadValue(node["ForegroundFpsLimit"], s.ForegroundFpsLimit);
        ReadValue(node["BackgroundFpsLimit"], s.BackgroundFpsLimit);
        ReadValue(node["UseVsync"], s.UseVsync);
        ReadValue(node["FilterMode"], (int&)s.FilterMode);
        return s;
    }

    void SaveOpenWindows(ryml::NodeRef node, const EditorSettings::OpenWindows& w) {
        node |= ryml::MAP;
        node["Lighting"] << w.Lighting;
        node["Properties"] << w.Properties;
        node["Textures"] << w.Textures;
        node["Reactor"] << w.Reactor;
        node["Diagnostics"] << w.Diagnostics;
        node["Noise"] << w.Noise;
        node["TunnelBuilder"] << w.TunnelBuilder;
        node["Sound"] << w.Sound;
        node["BriefingEditor"] << w.BriefingEditor;
        node["TextureEditor"] << w.TextureEditor;
        node["MaterialEditor"] << w.MaterialEditor;
        node["Scale"] << w.Scale;
        node["Debug"] << w.Debug;
    }

    EditorSettings::OpenWindows LoadOpenWindows(ryml::NodeRef node) {
        EditorSettings::OpenWindows w{};
        if (node.is_seed()) return w;
        ReadValue(node["Lighting"], w.Lighting);
        ReadValue(node["Properties"], w.Properties);
        ReadValue(node["Textures"], w.Textures);
        ReadValue(node["Reactor"], w.Reactor);
        ReadValue(node["Diagnostics"], w.Diagnostics);
        ReadValue(node["Noise"], w.Noise);
        ReadValue(node["TunnelBuilder"], w.TunnelBuilder);
        ReadValue(node["Sound"], w.Sound);
        ReadValue(node["BriefingEditor"], w.BriefingEditor);
        ReadValue(node["TextureEditor"], w.TextureEditor);
        ReadValue(node["MaterialEditor"], w.MaterialEditor);
        ReadValue(node["Scale"], w.Scale);
        ReadValue(node["Debug"], w.Debug);
        return w;
    }

    void SaveSelectionSettings(ryml::NodeRef node, const EditorSettings::SelectionSettings& s) {
        node |= ryml::MAP;
        node["PlanarTolerance"] << s.PlanarTolerance;
        node["StopAtWalls"] << s.StopAtWalls;
        node["UseTMap1"] << s.UseTMap1;
        node["UseTMap2"] << s.UseTMap2;
    }

    EditorSettings::SelectionSettings LoadSelectionSettings(ryml::NodeRef node) {
        EditorSettings::SelectionSettings s{};
        if (node.is_seed()) return s;
        ReadValue(node["PlanarTolerance"], s.PlanarTolerance);
        ReadValue(node["StopAtWalls"], s.StopAtWalls);
        ReadValue(node["UseTMap1"], s.UseTMap1);
        ReadValue(node["UseTMap2"], s.UseTMap2);
        return s;
    }

    void SaveLightSettings(ryml::NodeRef node, const LightSettings& s) {
        node |= ryml::MAP;
        node["Ambient"] << EncodeColor3(s.Ambient);
        node["AccurateVolumes"] << s.AccurateVolumes;
        node["Bounces"] << s.Bounces;
        node["DistanceThreshold"] << s.DistanceThreshold;
        node["EnableColor"] << s.EnableColor;
        node["EnableOcclusion"] << s.EnableOcclusion;
        node["Falloff"] << s.Falloff;
        node["MaxValue"] << s.MaxValue;
        node["Multiplier"] << s.Multiplier;
        node["Radius"] << s.Radius;
        node["Reflectance"] << s.Reflectance;
        node["Multithread"] << s.Multithread;
    }

    void SavePalette(ryml::NodeRef node, const Array<Color, PALETTE_SIZE>& palette) {
        node |= ryml::SEQ;
        for (auto& color : palette) {
            node.append_child() << EncodeColor3(color);
        }
    }

    Array<Color, PALETTE_SIZE> LoadPalette(ryml::NodeRef node) {
        Array<Color, PALETTE_SIZE> palette{};

        int i = 0;
        auto addColor = [&i, &palette](const Color& color) {
            if (i >= palette.size()) return;
            palette[i++] = color;
        };

        if (!node.valid() || node.is_seed()) {
            // Load defaults (D3 light colors)
            addColor({ .25, .3, .4 }); // bluish
            addColor({ .5, .5, .66 }); // blue lamp
            addColor({ .47, .50, .55 }); // white lamp
            addColor({ .3, .3, .3 }); // white
            addColor({ .3, .4, .4 }); // rusty teal
            addColor({ .12, .16, .16 }); // strip teal

            addColor({ .4, .2, .2 }); // reddish
            addColor({ 1.3, .3, .3 }); // super red
            addColor({ .4, .05, .05 }); // red
            addColor({ .24, .06, 0 }); // strip red
            addColor({ .5, .1, 0 }); // bright orange
            addColor({ .5, .3, .1 }); // bright orange

            addColor({ .4, .2, .05 }); // orange
            addColor({ .4, .3, .2 }); // orangish
            addColor({ .44, .32, .16 }); // bright orange
            addColor({ .2, .15, .1 }); // strip orange
            addColor({ .44, .44, .33 }); // bright yellow
            addColor({ .4, .4, .1 }); // yellow

            //addColor({ .4, .4, .3 }); // yellowish
            //addColor({ .2, .2, .15 }); // strip yellow

            addColor({ .2, .4, .3 }); // Greenish
            addColor({ .02, .3, .29 }); // teal (custom)
            addColor({ .25, .5, .15 }); // bright green
            addColor({ .05, .4, .2 }); // green
            addColor({ .16, .48, .32 }); // bright teal
            addColor({ .16, .32, .16 }); // strip green

            addColor({ .1, .24, .55 }); // bright blue
            addColor({ .05, .15, .40 }); // blue
            addColor({ .07, .14, .28 }); // strip blue
            addColor({ .12, .12, .43 }); // deep blue
            addColor({});
            addColor({});

            addColor({ 2, .6, 1.2 }); // super purple
            addColor({ .3, .05, .4 }); // purple
            addColor({ .4, .35, .45 }); // bright purple
            addColor({ .24, .24, .48 }); // purple
            addColor({ .14, .12, .20 }); // purple
            addColor({ .3, .25, .40 }); // purplish

            // Boost brightness
            for (auto& c : palette) {
                c *= 2.5f;
                c.w = 1;
            }
            return palette;
        }

        for (const auto& child : node.children()) {
            Color color;
            ReadValue(child, color);
            addColor(color);
        }

        return palette;
    }

    LightSettings LoadLightSettings(ryml::NodeRef node) {
        LightSettings settings{};
        if (node.is_seed()) return settings;

        ReadValue(node["Ambient"], settings.Ambient);
        ReadValue(node["AccurateVolumes"], settings.AccurateVolumes);
        ReadValue(node["Bounces"], settings.Bounces);
        ReadValue(node["DistanceThreshold"], settings.DistanceThreshold);
        ReadValue(node["EnableColor"], settings.EnableColor);
        ReadValue(node["EnableOcclusion"], settings.EnableOcclusion);
        ReadValue(node["Falloff"], settings.Falloff);
        ReadValue(node["MaxValue"], settings.MaxValue);
        ReadValue(node["Multiplier"], settings.Multiplier);
        ReadValue(node["Radius"], settings.Radius);
        ReadValue(node["Reflectance"], settings.Reflectance);
        ReadValue(node["Multithread"], settings.Multithread);
        return settings;
    }

    void SaveEditorBindings(ryml::NodeRef node) {
        node |= ryml::SEQ;

        for (auto& binding : Editor::Bindings::Active.GetBindings()) {
            auto child = node.append_child();
            child |= ryml::MAP;
            auto action = magic_enum::enum_name(binding.Action);
            auto key = string(magic_enum::enum_name(binding.Key));
            if (binding.Alt) key = "Alt " + key;
            if (binding.Shift) key = "Shift " + key;
            if (binding.Control) key = "Ctrl " + key;
            child[ryml::to_csubstr(action.data())] << key;
        }
    }

    void LoadEditorBindings(ryml::NodeRef node) {
        if (node.is_seed()) return;
        auto& bindings = Editor::Bindings::Active;
        bindings.Clear(); // we have some bindings to replace defaults!

        for (const auto& c : node.children()) {
            if (c.is_seed() || !c.is_map()) continue;

            auto kvp = c.child(0);
            string value, command;
            if (kvp.has_key()) command = string(kvp.key().data(), kvp.key().len);
            if (kvp.has_val()) value = string(kvp.val().data(), kvp.val().len);
            if (value.empty() || command.empty()) continue;

            Editor::EditorBinding binding{};
            if (auto commandName = magic_enum::enum_cast<Editor::EditorAction>(command))
                binding.Action = *commandName;

            auto tokens = String::Split(value, ' ');
            binding.Alt = Seq::contains(tokens, "Alt");
            binding.Shift = Seq::contains(tokens, "Shift");
            binding.Control = Seq::contains(tokens, "Ctrl");
            if (auto key = magic_enum::enum_cast<DirectX::Keyboard::Keys>(tokens.back()))
                binding.Key = *key;

            // Note that it is valid for Key to equal None to indicate that the user unbound it on purpose
            bindings.Add(binding);
        }

        // copy bindings before adding defaults so that multiple shortcuts for the same action will apply properly
        Editor::EditorBindings fileBindings = bindings;

        for (auto& defaultBinding : Editor::Bindings::Default.GetBindings()) {
            if (!fileBindings.GetBinding(defaultBinding.Action)) {
                // there's a default binding for this action and the file didn't provide one
                bindings.Add(defaultBinding);
            }
        }
    }

    void SaveBindings(ryml::NodeRef node) {
        node |= ryml::MAP;
        SaveEditorBindings(node["Editor"]);

        // todo: Game bindings
    }

    void SaveEditorSettings(ryml::NodeRef node, const EditorSettings& s) {
        node |= ryml::MAP;
        WriteSequence(node["RecentFiles"], s.RecentFiles);

        node["EnableWallMode"] << s.EnableWallMode;
        node["EnableTextureMode"] << s.EnableTextureMode;
        node["ObjectRenderDistance"] << s.ObjectRenderDistance;

        node["TranslationSnap"] << s.TranslationSnap;
        node["RotationSnap"] << s.RotationSnap;

        node["MouselookSensitivity"] << s.MouselookSensitivity;
        node["MoveSpeed"] << s.MoveSpeed;

        node["SelectionMode"] << (int)s.SelectionMode;
        node["InsertMode"] << (int)s.InsertMode;

        node["ShowObjects"] << s.ShowObjects;
        node["ShowWalls"] << s.ShowWalls;
        node["ShowTriggers"] << s.ShowTriggers;
        node["ShowFlickeringLights"] << s.ShowFlickeringLights;
        node["ShowAnimation"] << s.ShowAnimation;
        node["ShowMatcenEffects"] << s.ShowMatcenEffects;
        node["ShowPortals"] << s.ShowPortals;
        node["WireframeOpacity"] << s.WireframeOpacity;

        node["ShowWireframe"] << s.ShowWireframe;
        node["RenderMode"] << (int)s.RenderMode;
        node["GizmoSize"] << s.GizmoSize;
        node["CrosshairSize"] << s.CrosshairSize;
        node["InvertY"] << s.InvertY;
        node["InvertOrbitY"] << s.InvertOrbitY;
        node["MiddleMouseMode"] << (int)s.MiddleMouseMode;
        node["FieldOfView"] << s.FieldOfView;
        node["FontSize"] << s.FontSize;

        node["EditBothWallSides"] << s.EditBothWallSides;
        node["ReopenLastLevel"] << s.ReopenLastLevel;
        node["SelectMarkedSegment"] << s.SelectMarkedSegment;
        node["ResetUVsOnAlign"] << s.ResetUVsOnAlign;
        node["WeldTolerance"] << s.WeldTolerance;

        node["Undos"] << s.UndoLevels;
        node["AutosaveMinutes"] << s.AutosaveMinutes;
        node["CoordinateSystem"] << (int)s.CoordinateSystem;
        node["EnablePhysics"] << s.EnablePhysics;
        node["PasteSegmentObjects"] << s.PasteSegmentObjects;
        node["PasteSegmentWalls"] << s.PasteSegmentWalls;
        node["PasteSegmentSpecial"] << s.PasteSegmentSpecial;
        node["TexturePreviewSize"] << (int)s.TexturePreviewSize;
        node["ShowLevelTitle"] << s.ShowLevelTitle;

        SaveSelectionSettings(node["Selection"], s.Selection);
        SaveOpenWindows(node["Windows"], s.Windows);
        SaveLightSettings(node["Lighting"], s.Lighting);
        SavePalette(node["Palette"], s.Palette);
    }

    EditorSettings LoadEditorSettings(ryml::NodeRef node, InfernoSettings& settings) {
        EditorSettings s{};
        if (node.is_seed()) return s;

        for (const auto& c : node["RecentFiles"].children()) {
            filesystem::path path;
            ReadValue(c, path);
            if (!path.empty()) s.RecentFiles.push_back(path);
        }

        // Legacy. Read editor data paths into top level data paths
        auto dataPaths = node["DataPaths"];
        if (!dataPaths.is_seed()) {
            for (const auto& c : node["DataPaths"].children()) {
                filesystem::path path;
                ReadValue(c, path);
                if (!path.empty()) settings.DataPaths.push_back(path);
            }
        }

        ReadValue(node["EnableWallMode"], s.EnableWallMode);
        ReadValue(node["EnableTextureMode"], s.EnableTextureMode);
        ReadValue(node["ObjectRenderDistance"], s.ObjectRenderDistance);

        ReadValue(node["TranslationSnap"], s.TranslationSnap);
        ReadValue(node["RotationSnap"], s.RotationSnap);

        ReadValue(node["MouselookSensitivity"], s.MouselookSensitivity);
        ReadValue(node["MoveSpeed"], s.MoveSpeed);

        ReadValue(node["SelectionMode"], (int&)s.SelectionMode);
        ReadValue(node["InsertMode"], (int&)s.InsertMode);

        ReadValue(node["ShowObjects"], s.ShowObjects);
        ReadValue(node["ShowWalls"], s.ShowWalls);
        ReadValue(node["ShowTriggers"], s.ShowTriggers);
        ReadValue(node["ShowFlickeringLights"], s.ShowFlickeringLights);
        ReadValue(node["ShowAnimation"], s.ShowAnimation);
        ReadValue(node["ShowMatcenEffects"], s.ShowMatcenEffects);
        ReadValue(node["ShowPortals"], s.ShowPortals);
        ReadValue(node["WireframeOpacity"], s.WireframeOpacity);

        ReadValue(node["ShowWireframe"], s.ShowWireframe);
        ReadValue(node["RenderMode"], (int&)s.RenderMode);
        ReadValue(node["GizmoSize"], s.GizmoSize);
        ReadValue(node["CrosshairSize"], s.CrosshairSize);
        ReadValue(node["InvertY"], s.InvertY);
        ReadValue(node["InvertOrbitY"], s.InvertOrbitY);
        ReadValue(node["MiddleMouseMode"], (int&)s.MiddleMouseMode);
        ReadValue(node["FieldOfView"], s.FieldOfView);
        s.FieldOfView = std::clamp(s.FieldOfView, 45.0f, 130.0f);
        ReadValue(node["FontSize"], s.FontSize);
        s.FontSize = std::clamp(s.FontSize, 8, 48);

        ReadValue(node["EditBothWallSides"], s.EditBothWallSides);
        ReadValue(node["ReopenLastLevel"], s.ReopenLastLevel);
        ReadValue(node["SelectMarkedSegment"], s.SelectMarkedSegment);
        ReadValue(node["ResetUVsOnAlign"], s.ResetUVsOnAlign);
        ReadValue(node["WeldTolerance"], s.WeldTolerance);

        ReadValue(node["Undos"], s.UndoLevels);
        ReadValue(node["AutosaveMinutes"], s.AutosaveMinutes);
        ReadValue(node["CoordinateSystem"], (int&)s.CoordinateSystem);
        ReadValue(node["EnablePhysics"], s.EnablePhysics);
        ReadValue(node["PasteSegmentObjects"], s.PasteSegmentObjects);
        ReadValue(node["PasteSegmentWalls"], s.PasteSegmentWalls);
        ReadValue(node["PasteSegmentSpecial"], s.PasteSegmentSpecial);
        ReadValue(node["TexturePreviewSize"], (int&)s.TexturePreviewSize);
        ReadValue(node["ShowLevelTitle"], s.ShowLevelTitle);

        s.Palette = LoadPalette(node["Palette"]);
        s.Selection = LoadSelectionSettings(node["Selection"]);
        s.Windows = LoadOpenWindows(node["Windows"]);
        s.Lighting = LoadLightSettings(node["Lighting"]);
        return s;
    }

    void Settings::Save(const filesystem::path& path) {
        try {
            ryml::Tree doc(128, 128);
            doc.rootref() |= ryml::MAP;

            doc["Descent1Path"] << Settings::Inferno.Descent1Path.string();
            doc["Descent2Path"] << Settings::Inferno.Descent2Path.string();
            doc["MasterVolume"] << Settings::Inferno.MasterVolume;
            doc["GenerateMaps"] << Settings::Inferno.GenerateMaps;
            doc["Descent3Enhanced"] << Settings::Inferno.Descent3Enhanced;

            WriteSequence(doc["DataPaths"], Settings::Inferno.DataPaths);
            SaveEditorSettings(doc["Editor"], Settings::Editor);
            SaveGraphicsSettings(doc["Render"], Settings::Graphics);
            SaveBindings(doc["Bindings"]);

            std::ofstream file(path);
            file << doc;
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("Error saving config file:\n{}", e.what());
        }
    }

    void Settings::Load(const filesystem::path& path) {
        try {
            std::ifstream file(path);
            if (!file) return;

            std::stringstream buffer;
            buffer << file.rdbuf();
            ryml::Tree doc = ryml::parse_in_arena(ryml::to_csubstr(buffer.str()));
            ryml::NodeRef root = doc.rootref();

            if (root.is_map()) {
                ReadValue(root["Descent1Path"], Settings::Inferno.Descent1Path);
                ReadValue(root["Descent2Path"], Settings::Inferno.Descent2Path);
                ReadValue(root["MasterVolume"], Settings::Inferno.MasterVolume);
                ReadValue(root["GenerateMaps"], Settings::Inferno.GenerateMaps);
                ReadValue(root["Descent3Enhanced"], Settings::Inferno.Descent3Enhanced);

                auto dataPaths = root["DataPaths"];
                if (!dataPaths.is_seed()) {
                    for (const auto& c : dataPaths.children()) {
                        filesystem::path dataPath;
                        ReadValue(c, dataPath);
                        if (!dataPath.empty()) Settings::Inferno.DataPaths.push_back(dataPath);
                    }
                }

                Settings::Editor = LoadEditorSettings(root["Editor"], Settings::Inferno);
                Settings::Graphics = LoadGraphicsSettings(root["Render"]);
                auto bindings = root["Bindings"];
                if (!bindings.is_seed()) {
                    LoadEditorBindings(bindings["Editor"]);
                }
            }
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("Error loading config file:\n{}", e.what());
        }
    }
}
