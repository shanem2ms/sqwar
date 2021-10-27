#include "StdIncludes.h"
#include "World.h"
#include "Application.h"
#include "Engine.h"
#include <numeric>
#include "Mesh.h"
#include "gmtl/PlaneOps.h"


#define NOMINMAX


using namespace gmtl;

namespace sam
{
  
    World::World() :
        m_width(-1),
        m_height(-1),
        m_currentTool(0),
        m_prevMode(-1),
        m_mode(1)
    {

    }  

    void World::Open(const std::string& path)
    {
    }

    class Touch
    {
        Point2f m_touch;
        Point2f m_lastDragPos;
        bool m_isInit;
        bool m_isDragging;
    public:

        Vec2f m_initCamDir;

        Touch() : m_isInit(false),
            m_isDragging(false) {}

        bool IsInit() const {
            return m_isInit;
        }
        void SetInitialPos(const Point2f& mouse)
        {
            m_touch = mouse;
            m_isInit = true;
        }

        bool IsDragging() const { return m_isDragging; }

        void SetDragPos(const Point2f& newTouchPt)
        {
            Vec2f dpt = (newTouchPt - m_touch);
            if (length(dpt) > 0.04)
                m_isDragging = true;
            m_lastDragPos = newTouchPt;
        }

        const Point2f& LastDragPos() const { return m_lastDragPos; }
        const Point2f& TouchPos() const { return m_touch; }        
    };
    //https://shanetest-cos-earth.s3.us-east.cloud-object-storage.appdomain.cloud/usa10m_whqt/Q0/L0/R0/C0
    //https://shanetest-cos-earth.s3.us-east.cloud-object-storage.appdomain.cloud/world9m_whqt/Q0/L0/R0/C0
    //https://shanetest-cos-earth.s3.us-east.cloud-object-storage.appdomain.cloud/world9m_whqt/Q1/L3/R1/Q1_L3_R1_C0.png

    bool cursormode = false;
    void World::TouchDown(float x, float y, int touchId)
    {
        m_activeTouch = std::make_shared<Touch>();
        float xb = x / m_width;
        float yb = y / m_height;

        Engine& e = Engine::Inst();
        Camera::LookAt la = e.Cam().GetLookat();

        m_activeTouch->m_initCamDir = la.dir;
        m_activeTouch->SetInitialPos(Point2f(xb, yb));
    }

    constexpr float pi_over_two = 3.14159265358979323846f * 0.5f;
    void World::TouchDrag(float x, float y, int touchId)
    {
        if (m_activeTouch != nullptr)
        {
            float xb = x / m_width;
            float yb = y / m_height;

            m_activeTouch->SetDragPos(Point2f(xb, yb));

            Vec2f dragDiff = Point2f(xb, yb) - m_activeTouch->TouchPos();
            Engine& e = Engine::Inst();
            Camera::LookAt la = e.Cam().GetLookat();

            
            la.dir = m_activeTouch->m_initCamDir -
                dragDiff * 2.0f;
            e.Cam().SetLookat(la);
            
        }
    }

    void World::TouchUp(int touchId)
    {        
        m_activeTouch = nullptr;
    }

    const int LeftShift = 16;
    const int SpaceBar = 32;
    const int AButton = 'A';
    const int DButton = 'D';
    const int WButton = 'W';
    const int SButton = 'S';
    const int FButton = 'F';
    bool isPaused = false;

    int g_maxTileLod = 9;
    Vec3f g_vel;
    void World::KeyDown(int k)
    {
        float speed = 0.1f;
        switch (k)
        {
        case 'P':
            isPaused = !isPaused;
            break;
        case LeftShift:
            g_vel[1] -= speed;
            break;
        case SpaceBar:
            g_vel[1] += speed;
            break;
        case AButton:
            g_vel[0] -= speed;
            break;
        case DButton:
            g_vel[0] += speed;
            break;
        case WButton:
            g_vel[2] -= speed;
            break;
        case SButton:
            g_vel[2] += speed;
            break;
        case FButton:
            break;
        }
        if (k >= '1' && k <= '9')
        {
            g_maxTileLod = k - '0';
        }
    }

    void World::KeyUp(int k)
    {
        switch (k)
        {
        case LeftShift:
        case SpaceBar:
            g_vel[1] = 0;
            break;
        case AButton:
        case DButton:
            g_vel[0] = 0;
            break;
        case WButton:
        case SButton:
            g_vel[2] = 0;
            break;
        }
    }

    void World::Update(Engine& e, DrawContext& ctx)
    {   
        if (m_worldGroup == nullptr)
        {
            m_worldGroup = std::make_shared<SceneGroup>();
            e.Root()->AddItem(m_worldGroup);

            m_shader = e.LoadShader("vs_cubes.bin", "fs_cubes.bin");
            m_worldGroup->BeforeDraw([this](DrawContext& ctx) { ctx.m_pgm = m_shader; return true; });

            Camera::LookAt la = e.Cam().GetLookat();
            la.pos = Point3f(0, 0, -0.6f);
            e.Cam().SetLookat(la);
        }

        if (m_mode != m_prevMode)
        {
            m_worldGroup->Clear();
            m_planevis = nullptr;
            m_ptsvis = nullptr;
            if (m_mode & 1)
            {
                m_planevis = std::make_shared<PlanesVis>();
                m_worldGroup->AddItem(m_planevis);
            }
            if (m_mode & 2)
            {
                m_ptsvis = std::make_shared<PtsVis>();
                m_worldGroup->AddItem(m_ptsvis);
            }
            m_prevMode = m_mode;
        }

        {
            Camera::LookAt la = e.Cam().GetLookat();
            Vec3f right, up, forward;
            la.GetDirs(right, up, forward);
            la.pos += (g_vel[0] * right + g_vel[1] * up + g_vel[2] * forward);
            e.Cam().SetLookat(la);
        }
    }

    void World::Layout(int w, int h)
    {
        m_width = w;
        m_height = h;
    }
        

    World::~World()
    {

    }

    void World::OnDepthBuffer(const std::vector<unsigned char>& vidData, const std::vector<float>& depthData)
    {
        if (!isPaused)
        {
            if (m_planevis)
                m_planevis->SetDepthData(
                vidData.data(), vidData.size(), depthData);
            if (m_ptsvis)
                m_ptsvis->SetDepthData(
                vidData.data(), vidData.size(), depthData);
        }
    }

}
