#include "StdIncludes.h"
#include "World.h"
#include "Application.h"
#include "Engine.h"
#include <numeric>
#include "Mesh.h"
#include "gmtl/PlaneOps.h"
#include "PtsVis.h"
#include "PlanesVis.h"
#include "FaceVis.h"
#include "DepthPts.h"


#define NOMINMAX


using namespace gmtl;

namespace sam
{
  
    World::World() :
        m_width(-1),
        m_height(-1),
        m_currentTool(0),
        m_prevMode(-1),
        m_mode(6),
        m_wsHeadCenter(),
        m_prevDepthTimestamp(std::numeric_limits<double>::max())
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
            m_facevis = nullptr;
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
            if (m_mode & 4)
            {
                m_facevis = std::make_shared<FaceVis>();
                m_worldGroup->AddItem(m_facevis);
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

    void World::OnDepthBuffer(DepthData& depth)
    {
        if (depth.props.timestamp < m_prevDepthTimestamp)
            identity(m_alignedMtx);
        if (!isPaused)
        {
            std::vector<float> filtereddata;
            float faceDepth = -m_wsHeadCenter[2];
            filtereddata = depth.depthData;
            float d = 0.25f;
            float min = faceDepth - d;
            float max = faceDepth + d;
            int px = 0, py = 0;
                
            Vec2f fc(m_faceCenterXY);
            fc[0] = -fc[0];
            fc[1] = -fc[1];
            Vec2f v1((fc + Vec2f(1, 1)) * 0.5f);
            Vec2f centerPixel(v1[0] * depth.props.depthWidth, v1[1] * depth.props.depthHeight);
            float radiusSq = 150 * 150;
                
            for (auto itdata = filtereddata.begin() + 16; itdata !=
                filtereddata.end(); ++itdata)
            {
                float& val = *itdata;
                    
                float distSq = (px - centerPixel[0]) * (px - centerPixel[0]) +
                    (py - centerPixel[1]) * (py - centerPixel[1]);

                px++;
                if (px == depth.props.depthWidth)
                {
                    px = 0;
                    py++;
                }
                if (std::isnan(val) || std::isinf(val))
                    continue;
                if (distSq > radiusSq || val < min || val > max)
                    val = nanf("");
            }
            int dw = depth.props.depthWidth;
            int dh = depth.props.depthHeight;
            int size = 0;
            std::vector<int> lodOffsets;
            std::vector<int> lodSizes;
            lodOffsets.push_back(0);
            while (dw >= 16)
            {
                dw >>= 1;
                dh >>= 1;
                size += dw * dh;
                lodOffsets.push_back(size);
                lodSizes.push_back(dw * dh);
            }
            std::vector<float> lods(size);
            DepthBuildLods(filtereddata.data()+16, lods.data(), depth.props.depthWidth, depth.props.depthHeight);
            
            int lod = 0;
            
            if (lod > 0)
            {
                std::vector<float> lores(lodSizes[lod - 1] + 16);
                memcpy(lores.data(), filtereddata.data(), sizeof(float) * 16);
                memcpy(lores.data() + 16, lods.data() + lodOffsets[lod - 1], lodSizes[lod - 1] * sizeof(float));
                GetDepthPointsWithColor(lores, depth.vidData.data(), depth.props.vidWidth, depth.props.vidHeight, depth.pts, depth.props.depthWidth >> lod, depth.props.depthHeight >> lod, 10000.0f);
            }
            else
                GetDepthPointsWithColor(depth.depthData, depth.vidData.data(), depth.props.vidWidth, depth.props.vidHeight, depth.pts, depth.props.depthWidth >> lod, depth.props.depthHeight >> lod, 10000.0f);


            if (false && m_wsHeadCenter[2] != 0)
            {
                for (Vec4f& pt : depth.pts)
                {
                    for (int idx = 0; idx < 3; ++idx)
                        pt[idx] -= m_wsHeadCenter[idx];
                }
                std::vector<Vec3f> alignPts1;
                alignPts1.reserve(depth.pts.size() / 4);
                for (const Vec4f& pt : depth.pts)
                {
                    if (!isinf(pt[0]))
                        alignPts1.push_back(Vec3f(pt[0], pt[1], pt[2]));
                }
                if (m_prevPts.size() > 0)
                {
                    PTCloudAlign* pAlign = CreatePtCloudAlign(m_prevPts.data(), m_prevPts.size(), alignPts1.data(), alignPts1.size());
                    Matrix44f mat;
                    mat.mState = Matrix44f::AFFINE;
                    identity(mat);
                    int nsteps = 0;
                    while (AlignStep(pAlign, mat.mData) < 2)
                    {
                        nsteps++;
                    }

                    FreePtCloudAlign(pAlign);
                    m_alignedMtx *= mat;
                }
                std::swap(m_prevPts, alignPts1);
            }

            depth.alignMtx = m_alignedMtx;
            if (m_planevis)
                m_planevis->SetDepthData(depth);
            if (m_ptsvis)
                m_ptsvis->SetDepthData(depth);
        }

        m_prevDepthTimestamp = depth.props.timestamp;
    }

    void World::OnFaceData(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t> indices)
    {
        if (!isPaused)
        {
            if (m_wsHeadCenter[2] == 0)
            {
                Matrix44f wm, vm, pm;
                wm.mState = Matrix44f::AFFINE;
                vm.mState = Matrix44f::AFFINE;
                pm.mState = Matrix44f::FULL;
                memcpy(wm.mData, props.wMatf, sizeof(props.wMatf));
                memcpy(vm.mData, props.viewMatf, sizeof(props.viewMatf));
                memcpy(pm.mData, props.projMatf, sizeof(props.projMatf));
                wm = vm * wm;
                Vec4f pos;
                xform(pos, wm, Vec4f(0, 0, -0.025, 1));
                m_wsHeadCenter = Point3f(pos[0], pos[1], pos[2]);
                wm = pm * wm;
                xform(pos, wm, Vec4f(0, 0, 0, 1));
                pos /= pos[3];
                m_faceCenterXY = Vec2f(pos[0], pos[1]);
            }
            if (m_facevis)
                m_facevis->OnFaceData(props, vertices, indices);
        }
    }

}
