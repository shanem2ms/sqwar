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

    static const int boardSize = 32;
    struct BoardState
    {
        BoardState() 
        {
            memset(s, 0, sizeof(s));
        }
        char s[boardSize * boardSize];

        int Score(int playerIdx)
        {
            int curScore = 0;
            int pscore = playerIdx + 1;
            int oscore = (1 - playerIdx) + 1;
            for (int idx = 0; idx < boardSize * boardSize; ++idx)
            {
                if (s[idx] == pscore) curScore++;
                if (s[idx] == oscore) curScore--;
            }
            return curScore;
        }
    };
    
    class World
    {
    private:

        int m_width;
        int m_height;

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
        static void FindOptimalSquare(const BoardState& state, int playerIdx, int &sqx, int &sqy);
    };

}
