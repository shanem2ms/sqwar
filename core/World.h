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

    static const int boardSize = 16;
    class BoardState
    {
    private:
        char s[boardSize * boardSize];
        int score;
    public:
        BoardState() :
            score(0)
        {
            memset(s, 0, sizeof(s));
        }

        inline char Raw(int idx) const
        {
            return s[idx];
        }

        inline void SA(int x, int y, int playerIdx)
        {
            if (playerIdx == 0)
            {
                s[y * boardSize + x] = 1;
                score++;
            }
            if (playerIdx == 1)
            {
                s[x * boardSize + y] = 2;
                score--;
            }
        }

        inline char S(int x, int y, int playerIdx) const
        {
            return playerIdx == 0 ? s[y * boardSize + x] :
                s[x * boardSize + y];
        }

        int Score()
        {
            return score;
        }
    };
    
    class World
    {
    private:

        int m_width;
        int m_height;
        bool m_boardDirty;

        std::shared_ptr<SceneGroup> m_worldGroup;
        std::shared_ptr<Touch> m_activeTouch;
        int m_currentTool;
        bgfx::ProgramHandle m_shader;        
        std::vector<std::shared_ptr<Square>> m_squares;
        int m_playerTurn;
        BoardState m_board;
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
        static bool PlayerTakeSquare(BoardState& state, int playerIdx, int sqx, int sqy);
        static int FindOptimalSquare(const BoardState& state, int playerIdx, int &sqx, int &sqy, int level);
    };

}
