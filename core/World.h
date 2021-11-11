#pragma once
#include <map>
#include <set>
#include "SceneItem.h"

class SimplexNoise;
namespace sam
{

    struct DrawContext;
    class Engine;
    class Touch;
    class TargetCube;
    struct DepthDataProps;
    struct FaceDataProps;
    class FaceVis;
    class PtsVis;
    class PlanesVis;
    class DepthData;

    class World
    {
    private:

        int m_width;
        int m_height;

        std::shared_ptr<SceneGroup> m_worldGroup;
        std::shared_ptr<Touch> m_activeTouch;
        int m_currentTool;
        bgfx::ProgramHandle m_shader;     
        std::shared_ptr<PlanesVis> m_planevis;
        std::shared_ptr<PtsVis> m_ptsvis;
        std::shared_ptr<FaceVis> m_facevis;
        Point3f m_wsHeadCenter;
        Vec2f m_faceCenterXY;
        int m_mode;
        int m_prevMode;
        std::vector<Vec3f> m_prevPts;
        Matrix44f m_alignedMtx;
        double m_prevDepthTimestamp;
    public:


        void OnDepthBuffer(DepthData& depth);
        void OnFaceData(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t> indices);
        void Layout(int w, int h);
        World();
        ~World();
        int GetMode() const { return m_mode; }
        void SetMode(int mode) 
        { m_mode = mode; }
        void Update(Engine& engine, DrawContext& ctx);
        void TouchDown(float x, float y, int touchId);
        void TouchDrag(float x, float y, int touchId);
        void TouchUp(int touchId);
        void KeyDown(int k);
        void KeyUp(int k);
        void Open(const std::string &path);
    };

}
