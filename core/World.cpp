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
        m_playerTurn(0), 
        m_boardDirty(false)
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
        Camera::Fly la = e.Cam().GetFly();

        Matrix44f vproj = e.Cam().ProjectionMatrix()* e.Cam().ViewMatrix();
        invert(vproj);
        Vec4f res;
        xform(res, vproj, Vec4f(xb * 2 - 1, -yb * 2 + 1, 0.5f, 1));
        res /= res[3];
        int xsq = (int)(res[0] + (float)(boardSize / 2));
        int ysq = (int)(res[1] + (float)(boardSize / 2));
        if (m_playerTurn == 1)
        {
            if (PlayerTakeSquare(m_board, m_playerTurn, ysq, xsq))
            {
                m_boardDirty = true;
                m_playerTurn = 1 - m_playerTurn;
            }
        }

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
        }
    }

    void World::TouchUp(int touchId)
    {        
        m_activeTouch = nullptr;
    }

    bool World::PlayerTakeSquare(BoardState& state, int playerIdx, int sqx, int sqy)
    {
        int opponent = 1 - playerIdx;
        if (state.S(sqx, sqy, playerIdx) == 0)
        {
            state.SA(sqx, sqy, playerIdx);
            return true;
        }
        else if (state.S(sqx, sqy, playerIdx) == playerIdx + 1)
        {
            if ((sqx > 0 && state.S(sqx - 1, sqy, playerIdx) == playerIdx + 1) ||
                (sqx < (boardSize - 1) && state.S(sqx + 1, sqy, playerIdx) == playerIdx + 1))
            {
                int rmm = sqx;
                for (; rmm < boardSize &&
                    state.S(rmm, sqy, playerIdx) == playerIdx + 1; ++rmm);
                int lmm = sqx;
                for (; lmm >= 0 &&
                    state.S(lmm, sqy, playerIdx) == playerIdx + 1; --lmm);

                for (int mm = sqy; mm < boardSize; ++mm)
                {
                    bool foundopp = false;
                    for (int x = lmm + 1; x < rmm && !foundopp; ++x)
                    {
                        foundopp |= state.S(x, mm, playerIdx) == opponent + 1;
                    }
                    if (foundopp)
                        break;
                    for (int x = lmm + 1; x < rmm && !foundopp; ++x)
                    {
                        state.SA(x, mm, playerIdx);
                    }
                }
                for (int mm = sqy; mm >= 0; --mm)
                {
                    bool foundopp = false;
                    for (int x = lmm + 1; x < rmm && !foundopp; ++x)
                    {
                        foundopp |= state.S(x, mm, playerIdx) == opponent + 1;
                    }
                    if (foundopp)
                        break;
                    for (int x = lmm + 1; x < rmm && !foundopp; ++x)
                    {
                        state.SA(x, mm, playerIdx);
                    }
                }

            }
            else
            {
                for (int mm = sqx; mm < boardSize && 
                    state.S(mm, sqy, playerIdx) != opponent + 1; ++mm)
                {
                    state.SA(mm, sqy, playerIdx);
                }
                for (int mm = sqx; mm >= 0 && state.S(mm, sqy, playerIdx) != opponent + 1; --mm)
                {
                    state.SA(mm, sqy, playerIdx);
                }
            }
           
            return true;
        }

        return false;
    }

    int World::FindOptimalSquare(const BoardState &state, int playerIdx, int& sqx, int& sqy, int level)
    {
        int bestScore = boardSize * boardSize;
        if (playerIdx == 0)
            bestScore = -bestScore;
        int bestx = 0;
        int besty = 0;
        for (int x = 0; x < boardSize; ++x)
        {
            for (int y = 0; y < boardSize; ++y)
            {
                if (state.S(x, y, playerIdx) == ((1 - playerIdx) + 1))
                    continue;

                BoardState copy = state;
                if (!World::PlayerTakeSquare(copy, playerIdx, x, y))
                    continue;

                int score;
                if (level < 2)
                {
                    int ox = 0, oy = 0;
                    score = FindOptimalSquare(copy, 1 - playerIdx, ox, oy, level + 1);
                }
                else
                    score = copy.Score();

                if ((playerIdx == 0 && score > bestScore) ||
                    (playerIdx == 1 && score < bestScore))
                {
                    bestScore = score;
                    bestx = x;
                    besty = y;
                }
            }
        }

        sqx = bestx;
        sqy = besty;
        return bestScore;
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
    void World::KeyDown(int k)
    {
        float speed = 0.1f;
        switch (k)
        {
        case 'P':
            isPaused = !isPaused;
            break;
        case LeftShift:
            break;
        case SpaceBar:
            break;
        case AButton:
            break;
        case DButton:
            break;
        case WButton:
            break;
        case SButton:
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
            break;
        case AButton:
        case DButton:
            break;
        case WButton:
        case SButton:
            break;
        }
    }

    void World::Update(Engine& e, DrawContext& ctx)
    {   
        if (m_worldGroup == nullptr)
        {
            m_worldGroup = std::make_shared<SceneGroup>();
            e.Root()->AddItem(m_worldGroup);
            const int sq = boardSize;
            m_squares.resize(sq * sq);
            for (int x = 0; x < sq; x++)
            {
                for (int y = 0; y < sq; y++)
                {
                    auto square = std::make_shared<Square>();
                    float scale = 1;
                    square->SetOffset(Point3f((x - sq / 2 + 0.5f) * scale, (y - sq / 2 + 0.5f) * scale, 0));
                    square->SetScale(Vec3f(scale * 0.37f, scale * 0.37f, scale * 0.37f));
                    m_squares[y * sq + x] = square;
                    m_worldGroup->AddItem(square);
                }
            }

            m_shader = e.LoadShader("vs_cubes.bin", "fs_cubes.bin");
            m_worldGroup->BeforeDraw([this](DrawContext& ctx) { ctx.m_pgm = m_shader; return true; });
        }

        if (m_playerTurn == 0)
        {
            int sqx = 0, sqy = 0;
            FindOptimalSquare(m_board, m_playerTurn, sqx, sqy, 0);
            PlayerTakeSquare(m_board, m_playerTurn, sqx, sqy);
            m_playerTurn = 1 - m_playerTurn;
            m_boardDirty = true;
        }
        if (m_boardDirty)
        {
            for (int idx = 0; idx < boardSize * boardSize; ++idx)
                m_squares[idx]->m_state = m_board.Raw(idx);
            m_boardDirty = false;
        }
        e.Cam().SetOrthoMatrix(boardSize / 2);
        Camera::Fly la = e.Cam().GetFly();
    }

    void World::Layout(int w, int h)
    {
        m_width = w;
        m_height = h;
    }
        

    World::~World()
    {

    }

}
