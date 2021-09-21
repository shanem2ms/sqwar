#pragma once
#include <map>
#include <set>
#include "SceneItem.h"
#include "Square.h"

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
        gmtl::Point3f m_camVel;
        float m_tiltVel;
        bool m_flymode;

        float m_gravityVel;

        std::shared_ptr<SceneGroup> m_worldGroup;
        std::shared_ptr<Touch> m_activeTouch;
        int m_currentTool;
        bgfx::ProgramHandle m_shader;        
        std::vector<std::shared_ptr<Square>> m_squares;
    public:

        void Layout(int w, int h);
        World();
        ~World();
        void Update(Engine& engine, DrawContext& ctx);
        void TouchDown(float x, float y, int touchId);
        void TouchDrag(float x, float y, int touchId);
        void TouchUp(int touchId);
        void KeyDown(int k);
        void KeyUp(int k);
        void Open(const std::string &path);
    };

}
