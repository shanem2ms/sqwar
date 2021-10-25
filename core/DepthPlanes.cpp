// dllmain.cpp : Defines the entry point for the DLL application.
#include "StdIncludes.h"
#include "DepthPts.h"
#include <gmtl/AABox.h>
#include <gmtl/AABoxOps.h>
#include <gmtl/Plane.h>
#include <gmtl/PlaneOps.h>
#include <gmtl/Frustum.h>
#include <functional>

using namespace gmtl;

enum class Side
{
    Top = 0,
    Left = 1,
    Right = 2,
    Bottom = 3
};

struct Rect
{
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int _x, int _y, int _w, int _h) :
        x(_x), y(_y), w(_w), h(_h) {}

    int x;
    int y;
    int w;
    int h;

    unsigned long long GetUniqueId()
    {
        return ((unsigned long long)(x) << 30) |
            ((unsigned long long)(y) << 20) |
            ((unsigned long long)(w) << 10) |
            ((unsigned long long)(h));
    }
    int Right() { return x + w; }
    int Bottom() { return y + h; }
};

struct Buffer
{
    Point3f* depthPths;
    int width;
    int height;
};

struct Quad
{
    Point3f pt[4];
};

struct Result
{
    Result() : visitCnt(0),
        shouldRemove(false)
    {

    }
    Rect r;
    Quad q;
    float maxDistFound;

    Vec3f normal;
    Point3f pt0;

    bool isPicked = false;

    std::vector<std::pair<Result*, Side>> neighbors;
    int visitCnt;
    bool shouldRemove;
};

typedef std::shared_ptr<Result> ResultPtr;

float g_mindist = 0.05f;
float g_splitThreshold = 0.015f;
float g_MinDPVal = 0.9f;
float g_maxCoverage = 20.0f;
unsigned long long lastPickedId = 0;

void SetPlaneConstants(float minDist, float splitThreshold, float
    minDPVal, float maxCoverage)
{
    g_mindist = minDist;
    g_splitThreshold = splitThreshold;
    g_MinDPVal = minDPVal;
    g_maxCoverage = maxCoverage;
}

inline bool IsValid(const Vec3f &v)
{
    return !isinf(v[0]) && (v[0] != 0 || v[1] != 0 || v[2] != 0);
}

struct Tile
{
    Rect m_rect;
    Buffer& m_buffer;

    Tile* m_tiles;

    Tile(Buffer& buffer, const Rect& rect) :
        m_buffer(buffer),
        m_rect(rect),
        m_tiles(nullptr)
    {

    }

    inline Point3f& GetPt(int x, int y)
    {
        int ry = std::min(m_buffer.height - 1, y + m_rect.y);
        int rx = std::min(m_buffer.width - 1, x + m_rect.x);
        return m_buffer.depthPths[ry * m_buffer.width + rx];
    }

    void Process(std::vector<ResultPtr>& quads, int level)
    {
        const Point3f* ptl = nullptr, * ptr = nullptr,
            * pbl = nullptr, * pbr = nullptr;
        int height = m_rect.h;
        int width = m_rect.w;
        int found = 0;

        //if (m_rect.GetUniqueId() == lastPickedId)
        //    OutputDebugStringA("pick");

        for (int y = 0; y <= height && ptl == nullptr; ++y)
        {
            for (int x = 0; x <= width && ptl == nullptr; ++x)
            {
                Point3f& pt = GetPt(x, y);
                if (IsValid(pt))
                {
                    found++;  ptl = &pt;
                }
            }
        }

        for (int y = 0; y <= height && ptr == nullptr; ++y)
        {
            for (int x = width; x >= 0 && ptr == nullptr; --x)
            {
                Point3f& pt = GetPt(x, y);
                if (IsValid(pt))
                {
                    ptr = &pt;
                    found++;
                }
            }
        }

        for (int y = height; y >= 0 && pbl == nullptr; --y)
        {
            for (int x = 0; x <= width && pbl == nullptr; ++x)
            {
                Point3f& pt = GetPt(x, y);
                if (IsValid(pt))
                {
                    pbl = &pt;
                    found++;
                }
            }
        }

        for (int y = height; y >= 0 && pbr == nullptr; --y)
        {
            for (int x = width; x >= 0 && pbr == nullptr; --x)
            {
                Point3f& pt = GetPt(x, y);
                if (IsValid(pt))
                {
                    pbr = &pt;
                    found++;
                }
            }
        }

        if (found < 4)
            return;

        Vec3f vec1 = *pbr - *ptr;
        Vec3f vec2 = *ptr - *ptl;
        Vec3f vec3 = *pbl - *ptl;
        Vec3f nrm1;
        cross(nrm1, vec1, vec2);
        if (length(nrm1) == 0)
        {
            cross(nrm1, vec1, vec3);
            if (length(nrm1) == 0)
                cross(nrm1, vec2, vec3);
        }

        if (length(nrm1) == 0)
        {
#if 0
            char output[1024];
            sprintf_s(output, "Tile [%d, %d] [%d, %d]\n", m_rect.x, m_rect.y, m_rect.w, m_rect.h);
            OutputDebugStringA(output);
#endif
            return;
        }

        normalize(nrm1);

        Point3f planePt = *ptl;
        bool split = false;
        float maxDistFound = 0;
        float numPts = 0;
        for (int y = 0; y <= height && !split; ++y)
        {
            for (int x = 0; x <= width && !split; ++x)
            {
                Point3f& pt = GetPt(x, y);
                if (IsValid(pt))
                {                    
                    float dp = fabs(dot(pt - planePt, nrm1));
                    if (dp > g_mindist)
                    {
                        split = true;
                    }
                    maxDistFound += dp;
                    numPts++;
                }
            }
        }

        maxDistFound /= numPts;
        if (maxDistFound > g_splitThreshold)
            split = true;

        if (split)
        {
            if (m_rect.w > m_rect.h)
            {
                m_tiles = new Tile[2]{
                     Tile(m_buffer, Rect(m_rect.x, m_rect.y, m_rect.w - m_rect.w / 2, m_rect.h)),
                     Tile(m_buffer, Rect(m_rect.x + m_rect.w / 2, m_rect.y, m_rect.w / 2, m_rect.h)) };
            }
            else
            {
                m_tiles = new Tile[2]{
                     Tile(m_buffer, Rect(m_rect.x, m_rect.y, m_rect.w, m_rect.h / 2)),
                     Tile(m_buffer, Rect(m_rect.x, m_rect.y + m_rect.h / 2, m_rect.w, m_rect.h / 2)) };
            }

            for (int tIdx = 0; tIdx < 2; ++tIdx)
            {
                m_tiles[tIdx].Process(quads, level + 1);
            }
        }
        else
        {
            Result r;
            r.normal = nrm1;
            r.pt0 = planePt;
            r.r = m_rect;
            r.maxDistFound = maxDistFound;
            r.q.pt[0] = *ptl;
            r.q.pt[1] = *ptr;
            r.q.pt[2] = *pbl;
            r.q.pt[3] = *pbr;

            quads.push_back(std::make_shared<Result>(r));
        }
    }
};

enum class BreakType
{
    Start,
    End
};
struct TileBreak
{
    int xyVal;
    BreakType start;
    Side edge;
    Result* pTile;

    bool operator < (const TileBreak& rhs) const
    {
        if (xyVal != rhs.xyVal)
            return xyVal < rhs.xyVal;
        else if (start != rhs.start)
            return start < rhs.start;
        else
            return edge < rhs.edge;
    }
};


void PopulateNeighbors(std::vector<ResultPtr>& resultTiles)
{
    std::map<int, std::vector<ResultPtr>> xTilesLeft;
    std::map<int, std::vector<ResultPtr>> xTilesRight;
    std::map<int, std::vector<ResultPtr>> yTilesTop;
    std::map<int, std::vector<ResultPtr>> yTilesBottom;

    for (const ResultPtr& result : resultTiles)
    {
        {
            auto itRes = xTilesLeft.find(result->r.x);
            if (itRes == xTilesLeft.end())
            {
                itRes = xTilesLeft.insert(std::make_pair(result->r.x,
                    std::vector<ResultPtr>())).first;
            }
            itRes->second.push_back(result);
        }

        {
            auto itRes = xTilesRight.find(result->r.Right());
            if (itRes == xTilesRight.end())
            {
                itRes = xTilesRight.insert(std::make_pair(result->r.Right(),
                    std::vector<ResultPtr>())).first;
            }
            itRes->second.push_back(result);
        }

        {
            auto itRes = yTilesTop.find(result->r.y);
            if (itRes == yTilesTop.end())
            {
                itRes = yTilesTop.insert(std::make_pair(result->r.y,
                    std::vector<ResultPtr>())).first;
            }
            itRes->second.push_back(result);
        }

        {
            auto itRes = yTilesBottom.find(result->r.Bottom());
            if (itRes == yTilesBottom.end())
            {
                itRes = yTilesBottom.insert(std::make_pair(result->r.Bottom(),
                    std::vector<ResultPtr>())).first;
            }
            itRes->second.push_back(result);
        }
    }

    for (auto itXBucket : xTilesLeft)
    {
        std::vector<ResultPtr>& xvecs = itXBucket.second;
        std::sort(xvecs.begin(), xvecs.end(), [](const ResultPtr& a,
            const ResultPtr& b)
            {
                return a->r.y < b->r.y;
            });
    }
    for (auto itXBucket : xTilesRight)
    {
        std::vector<ResultPtr>& xvecs = itXBucket.second;
        std::sort(xvecs.begin(), xvecs.end(), [](const ResultPtr& a,
            const ResultPtr& b)
            {
                return a->r.y < b->r.y;
            });
    }
    for (auto itYBucket : yTilesTop)
    {
        std::vector<ResultPtr>& yvecs = itYBucket.second;
        std::sort(yvecs.begin(), yvecs.end(), [](const ResultPtr& a,
            const ResultPtr& b)
            {
                return a->r.x < b->r.x;
            });
    }

    for (auto itYBucket : yTilesBottom)
    {
        std::vector<ResultPtr>& yvecs = itYBucket.second;
        std::sort(yvecs.begin(), yvecs.end(), [](const ResultPtr& a,
            const ResultPtr& b)
            {
                return a->r.x < b->r.x;
            });
    }

    for (auto itLeftBucket : xTilesLeft)
    {
        auto itRightBucket = xTilesRight.find(itLeftBucket.first);
        if (itRightBucket != xTilesRight.end())
        {
            std::vector<ResultPtr>& leftvec = itLeftBucket.second;
            std::vector<ResultPtr>& rightvec = itRightBucket->second;
            std::vector<TileBreak> tbreaks;
            for (ResultPtr& ptr : leftvec)
            {
                TileBreak tb1;
                tb1.edge = Side::Right;
                tb1.xyVal = ptr->r.y;
                tb1.pTile = ptr.get();
                tb1.start = BreakType::Start;
                tbreaks.push_back(tb1);
                tb1.xyVal = ptr->r.Bottom();
                tb1.pTile = ptr.get();
                tb1.start = BreakType::End;
                tbreaks.push_back(tb1);
            }
            for (ResultPtr& ptr : rightvec)
            {
                TileBreak tb1;
                tb1.edge = Side::Left;
                tb1.xyVal = ptr->r.y;
                tb1.pTile = ptr.get();
                tb1.start = BreakType::Start;
                tbreaks.push_back(tb1);
                tb1.xyVal = ptr->r.Bottom();
                tb1.pTile = ptr.get();
                tb1.start = BreakType::End;
                tbreaks.push_back(tb1);
            }

            std::sort(tbreaks.begin(), tbreaks.end());
            Result* curresult[4] = { nullptr, nullptr };
            for (TileBreak& tb : tbreaks)
            {
                if (tb.start == BreakType::Start)
                {
                    int edge = (int)tb.edge;
                    curresult[edge] = tb.pTile;
                    if (curresult[3 - edge] != nullptr)
                    {
                        curresult[3 - edge]->neighbors.push_back(
                            std::make_pair(tb.pTile, (Side)(3 - edge)));
                        tb.pTile->neighbors.push_back(
                            std::make_pair(curresult[3 - edge], tb.edge));
                    }
                }
                else
                    curresult[(int)tb.edge] = nullptr;
            }
        }
    }


    for (auto itTopBucket : yTilesTop)
    {
        auto itBottomBucket = yTilesBottom.find(itTopBucket.first);
        if (itBottomBucket != yTilesBottom.end())
        {
            std::vector<ResultPtr>& topvec = itTopBucket.second;
            std::vector<ResultPtr>& bottomvec = itBottomBucket->second;
            std::vector<TileBreak> tbreaks;
            for (ResultPtr& ptr : topvec)
            {
                TileBreak tb1;
                tb1.edge = Side::Bottom;
                tb1.xyVal = ptr->r.x;
                tb1.pTile = ptr.get();
                tb1.start = BreakType::Start;
                tbreaks.push_back(tb1);
                tb1.xyVal = ptr->r.Right();
                tb1.pTile = ptr.get();
                tb1.start = BreakType::End;
                tbreaks.push_back(tb1);
            }
            for (ResultPtr& ptr : bottomvec)
            {
                TileBreak tb1;
                tb1.edge = Side::Top;
                tb1.xyVal = ptr->r.x;
                tb1.pTile = ptr.get();
                tb1.start = BreakType::Start;
                tbreaks.push_back(tb1);
                tb1.xyVal = ptr->r.Right();
                tb1.pTile = ptr.get();
                tb1.start = BreakType::End;
                tbreaks.push_back(tb1);
            }

            std::sort(tbreaks.begin(), tbreaks.end());
            Result* curresult[4] = { nullptr, nullptr };
            for (TileBreak& tb : tbreaks)
            {
                if (tb.start == BreakType::Start)
                {
                    int edge = (int)tb.edge;
                    curresult[edge] = tb.pTile;
                    if (curresult[3 - edge] != nullptr)
                    {
                        curresult[3 - edge]->neighbors.push_back(
                            std::make_pair(tb.pTile, (Side)(3 - edge)));
                        tb.pTile->neighbors.push_back(
                            std::make_pair(curresult[3 - edge], tb.edge));
                    }
                }
                else
                    curresult[(int)tb.edge] = nullptr;
            }
        }
    }
}

void FindConnected(Result* pThis, int visitIdx, std::vector<Result*>& outTiles)
{
    if (pThis->visitCnt == visitIdx)
        return;

    outTiles.push_back(pThis);
    pThis->visitCnt = visitIdx;

    for (auto itNeighbors : pThis->neighbors)
    {
        Result* other = itNeighbors.first;

        float dotNrm = dot(pThis->normal, other->normal);
        if ((dotNrm < -g_MinDPVal || dotNrm > g_MinDPVal) &&
            dot((pThis->pt0 - other->pt0), pThis->normal) < g_mindist)
        {
            FindConnected(itNeighbors.first, visitIdx, outTiles);
        }
    }
}

void DepthMakePlanes(float* vals, Point3f* outVertices, Point3f* outTexCoords, int maxCount, int* outCount,
    int depthWidth, int depthHeight)
{
    Buffer b;
    b.depthPths = (Point3f*)vals;
    b.width = depthWidth;
    b.height = depthHeight;

    Rect top(0, 0, b.width, b.height);

    Tile t(b, top);
    std::vector<ResultPtr> resultTiles;
    t.Process(resultTiles, 0);

    float fullDiagonal = sqrt(depthWidth * depthWidth + depthHeight * depthHeight);
    for (auto itRes = resultTiles.begin(); itRes !=
        resultTiles.end();)
    {
        ResultPtr& res = *itRes;
        bool yorz = res->normal[1] > res->normal[2];
        Vec3f xdir;
        cross(xdir, res->normal, yorz ? Vec3f(0, 0, 1) : Vec3f(0, 1, 0));
        Vec3f ydir;
        cross(ydir, xdir, res->normal);
        const Point3f* pts = res->q.pt;
        Vec3f ptpln[4];
        for (int i = 0; i < 4; ++i)
        {
            ptpln[i] = Vec3f(dot(pts[i] - res->pt0, xdir),
                dot(pts[i] - res->pt0, ydir), 0);
        }

        float longestDiag = 0;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = i + 1; j < 4; ++j)
            {
                longestDiag =
                    std::max(longestDiag, lengthSquared(Vec3f(pts[i] - pts[j])));
            }
        }

        longestDiag = sqrt(longestDiag);

        float rectDiag = sqrt(res->r.w * res->r.w + res->r.h * res->r.h) / fullDiagonal;
        float coverage = fabs(longestDiag / rectDiag);
        if (coverage > g_maxCoverage)
        {
            //char output[1024];
            //sprintf_s(output, "Area %f ( %f [%d, %d]) \n", coverage, longestDiag, res->r.w, res->r.h);
            //OutputDebugStringA(output);
            //res->shouldRemove = true;                
            itRes = resultTiles.erase(itRes);
        }
        else
            ++itRes;

    }
    PopulateNeighbors(resultTiles);

    const int visitIdx = 1;
    std::vector<std::vector<Result*>> outTiles;
    for (ResultPtr& res : resultTiles)
    {
        if (res->visitCnt == visitIdx)
            continue;
        outTiles.push_back(std::vector<Result*>());
        FindConnected(res.get(), 1, outTiles.back());
    }

    size_t vIdx = 0;
    for (auto& itVec : outTiles)
    {
        Point3f rgb((float)std::rand() / RAND_MAX,
            (float)std::rand() / RAND_MAX,
            (float)std::rand() / RAND_MAX);

        for (auto result : itVec)
        {
            Quad& q = result->q;
            outVertices[vIdx] = q.pt[0];
            outVertices[vIdx + 1] = q.pt[1];
            outVertices[vIdx + 2] = q.pt[2];
            outVertices[vIdx + 3] = q.pt[1];
            outVertices[vIdx + 4] = q.pt[3];
            outVertices[vIdx + 5] = q.pt[2];
            for (size_t idx = 0; idx < 6; ++idx)
            {
                outTexCoords[vIdx + idx] = result->isPicked ? Point3f(1, 1, 1) :
                    rgb;
            }

            vIdx += 6;

        }
    }
    *outCount = vIdx;
}
