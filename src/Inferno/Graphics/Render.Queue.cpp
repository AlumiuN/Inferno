#include "pch.h"
#include "Render.Queue.h"

#include "Game.h"
#include "Game.Wall.h"
#include "Mesh.h"
#include "Render.Debug.h"
#include "Render.Editor.h"
#include "Render.Particles.h"
#include "Render.h"
#include "ScopedTimer.h"

namespace Inferno::Render {
    bool ShouldDrawObject(const Object& obj) {
        if (!obj.IsAlive()) return false;
        bool gameModeHidden = obj.Type == ObjectType::Player || obj.Type == ObjectType::Coop;
        if (Game::State != GameState::Editor && gameModeHidden) return false;
        DirectX::BoundingSphere bounds(obj.GetPosition(Game::LerpAmount), obj.Radius);
        return CameraFrustum.Contains(bounds);
    }

    void RenderQueue::Update(Level& level, span<LevelMesh> levelMeshes, span<LevelMesh> wallMeshes) {
        _transparentQueue.clear();
        _opaqueQueue.clear();
        if (!_meshBuffer) return;

        if (Settings::Editor.RenderMode != RenderMode::None) {
            // Queue commands for level meshes
            for (auto& mesh : levelMeshes)
                _opaqueQueue.push_back({ &mesh, 0 });

            if (Game::State == GameState::Editor || level.Objects.empty()) {
                for (auto& mesh : wallMeshes) {
                    float depth = Vector3::DistanceSquared(Camera.Position, mesh.Chunk->Center);
                    _transparentQueue.push_back({ &mesh, depth });
                }

                if (Settings::Editor.ShowObjects) {
                    for (auto& obj : level.Objects) {
                        if (!ShouldDrawObject(obj)) continue;
                        QueueEditorObject(obj, Game::LerpAmount);
                    }
                }

                QueueParticles();
                QueueDebris();
                Seq::sortBy(_transparentQueue, [](const RenderCommand & l, const RenderCommand & r) {
                    return l.Depth < r.Depth; // front to back, because the draw call flips it
                });
            }
            else {
                TraverseLevel(level.Objects[0].Segment, level, wallMeshes);
                // todo: remove calls after merging into traverse level
                QueueParticles();
                QueueDebris();
            }
        }
    }

    void RenderQueue::QueueEditorObject(Object& obj, float lerp) {
        auto position = obj.GetPosition(lerp);

        DirectX::BoundingSphere bounds(position, obj.Radius);
        if (!CameraFrustum.Contains(bounds))
            return;

        float depth = GetRenderDepth(position);
        const float maxDistSquared = Settings::Editor.ObjectRenderDistance * Settings::Editor.ObjectRenderDistance;

        if (depth > maxDistSquared && Game::State == GameState::Editor) {
            DrawObjectOutline(obj);
        }
        else if (obj.Render.Type == RenderType::Model && obj.Render.Model.ID != ModelID::None) {
            _opaqueQueue.push_back({ &obj, depth });

            auto& mesh = _meshBuffer->GetHandle(obj.Render.Model.ID);
            if (mesh.HasTransparentTexture)
                _transparentQueue.push_back({ &obj, depth });
        }
        else {
            _transparentQueue.push_back({ &obj, depth });
        }
    }

    void RenderQueue::TraverseLevel(SegID startId, Level& level, span<LevelMesh> wallMeshes) {
        ScopedTimer levelTimer(&Render::Metrics::QueueLevel);

        _visited.clear();
        _search.push(startId);

        struct ObjDepth { Object* Obj = nullptr; float Depth = 0; };
        List<ObjDepth> objects;

        while (!_search.empty()) {
            auto id = _search.front();
            _search.pop();

            // must check if visited because multiple segs can connect to the same seg before being it is visited
            if (_visited.contains(id)) continue;
            _visited.insert(id);
            auto* seg = &level.GetSegment(id);

            struct SegDepth { SegID Seg = SegID::None; float Depth = 0; };
            Array<SegDepth, 6> children{};

            // Find open sides
            for (auto& sideId : SideIDs) {
                if (!WallIsTransparent(level, { id, sideId }))
                    continue; // Can't see through wall

                if (id != startId) { // always add nearby segments
                    auto vec = seg->Sides[(int)sideId].Center - Camera.Position;
                    vec.Normalize();
                    if (vec.Dot(seg->Sides[(int)sideId].AverageNormal) >= 0)
                        continue; // Cull backfaces
                }

                auto cid = seg->GetConnection(sideId);
                auto cseg = level.TryGetSegment(cid);
                if (cseg && !_visited.contains(cid) /*&& CameraFrustum.Contains(cseg->Center)*/) {
                    children[(int)sideId] = {
                        .Seg = cid,
                        .Depth = Vector3::DistanceSquared(cseg->Center, Camera.Position)
                    };
                }
            }

            // Sort connected segments by depth
            Seq::sortBy(children, [](const SegDepth& a, const SegDepth& b) {
                if (a.Seg == SegID::None) return false;
            if (b.Seg == SegID::None) return true;
            return a.Depth < b.Depth;
            });

            for (auto& c : children) {
                if (c.Seg != SegID::None)
                    _search.push(c.Seg);
            }

            objects.clear();

            // queue objects in segment
            for (auto oid : seg->Objects) {
                if (auto obj = level.TryGetObject(oid)) {
                    if (!ShouldDrawObject(*obj)) continue;

                    DirectX::BoundingSphere bounds(obj->Position, obj->Radius);
                    if (CameraFrustum.Contains(bounds))
                        objects.push_back({ obj, GetRenderDepth(obj->Position) });
                }
            }

            // Sort objects in segment by depth
            Seq::sortBy(objects, [](const ObjDepth& a, const ObjDepth& b) {
                return a.Depth < b.Depth;
            });

            // Queue objects in seg
            for (auto& obj : objects) {
                if (obj.Obj->Render.Type == RenderType::Model &&
                    obj.Obj->Render.Model.ID != ModelID::None) {
                    _opaqueQueue.push_back({ obj.Obj, 0 });

                    auto& mesh = _meshBuffer->GetHandle(obj.Obj->Render.Model.ID);
                    if (mesh.HasTransparentTexture)
                        _transparentQueue.push_back({ obj.Obj, obj.Depth });
                }
                else {
                    _transparentQueue.push_back({ obj.Obj, obj.Depth });
                }
            }

            // queue visible walls
            for (auto& mesh : wallMeshes) { // this does not scale well
                if (mesh.Chunk->Tag.Segment == id)
                    _transparentQueue.push_back({ &mesh, 0 });
            }
        }

        Stats::VisitedSegments = (uint16)_visited.size();
    }
}
