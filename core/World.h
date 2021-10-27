#pragma once
#include <map>
#include <set>
#include "SceneItem.h"
#include "PtsVis.h"
#include "PlanesVis.h"

class SimplexNoise;
namespace sam
{

    struct DrawContext;
    class Engine;
    class Touch;
    class TargetCube;

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
        int m_mode;
        int m_prevMode;
    public:


        void OnDepthBuffer(const std::vector<unsigned char>& vidData, const std::vector<float>& depthData);

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
