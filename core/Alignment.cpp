// dllmain.cpp : Defines the entry point for the DLL application.
#include "kdtree++/kdtree.hpp"
#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/Matrix.h>
#include <gmtl/Quat.h>
#include <vector>
#include <string>
#include <sstream>
#include "DepthPts.h"
#include "PtsLib.h"

using namespace gmtl;
#define Dbg(x)

namespace sam
{
struct V3
{
    typedef float value_type;
    Point3f pos;
    value_type operator[](size_t n) const
    {
        return pos[n];
    }
};
struct ANode
{
    typedef float value_type;

    ANode(const Point3f& p, size_t i) :
        pos(p),
        idx(i) {}

    Point3f pos;
    size_t idx;

    value_type operator[](size_t n) const
    {
        return pos[n];
    }
};

struct Match
{
    Match() : distsq(-1), matchedIdx(-1) {}
    float distsq;
    size_t idx;
    long matchedIdx;
};


int MatchesT1(Point3f* pts0, size_t npts0, Point3f* pts1, size_t npts1, int* matchidxs, int maxMatches)
{
    KDTree::KDTree <3, ANode> kdtree;
    size_t ptidx = 0;
    for (Point3f* pt = pts0; pt < (pts0 + npts0); ++pt)
    {
        ANode an(*pt, ptidx++);
        kdtree.insert(an);
    }

    KDTree::KDTree <3, ANode> kdtree2;
    size_t ptidx2 = 0;
    for (Point3f* pt = pts1; pt < (pts1 + npts1); ++pt)
    {
        ANode an(*pt, ptidx2++);
        kdtree2.insert(an);
    }

    kdtree.optimize();
    kdtree2.optimize();

    size_t midx = 0;
    std::vector<Match> matches;
    matches.resize(npts0);
    for (Match& m : matches) { m.idx = midx++; }

    int skip = 1;
    if (npts1 > maxMatches)
        skip = npts1 / maxMatches;
    size_t mIdx = 0;
    for (Point3f* pt = pts1; pt < (pts1 + npts1); pt += skip)
    {
        ANode v1(*pt, 0);
        auto itFound = kdtree.find_nearest(v1);
        size_t idxfound = itFound.first->idx;
        Match& match = matches[idxfound];
        const Point3f& pt0 = pts0[idxfound];
        ANode v2(pt0, 0);
        auto itFound2 = kdtree2.find_nearest(v2);
        if (itFound2.first->idx == mIdx)
        {
            Vec3f v3f = *pt - pt0;
            float distsq = lengthSquared(v3f);
            if (match.matchedIdx < 0 ||
                distsq < match.distsq)
            {
                match.distsq = distsq;
                match.matchedIdx = mIdx;
            }
        }
        mIdx++;
    }

    int* cmatch = matchidxs;
    int nmatches = 0;
    for (Match& m : matches)
    {
        if (m.matchedIdx < 0)
            continue;
        *cmatch++ = m.idx;
        *cmatch++ = m.matchedIdx;
        nmatches++;
    }
    return nmatches;
}

int MatchesT2(Point3f* pts0, size_t npts0, Point3f* pts1, size_t npts1, float maxDistThreshold, int* matchidxs)
{
    KDTree::KDTree <3, ANode> kdtree;
    size_t ptidx = 0;
    for (Point3f* pt = pts0; pt < (pts0 + npts0); ++pt, ++ptidx)
    {
        if (!IsValid(*pt))
            continue;
        ANode an(*pt, ptidx);
        kdtree.insert(an);
    }

    kdtree.optimize();

    size_t midx = 0;
    std::vector<Match> matches;
    matches.resize(npts0);
    for (Match& m : matches) { m.idx = midx++; }


    int skip = 1;
    size_t mIdx = 0;
    for (Point3f* pt = pts1; pt < (pts1 + npts1); pt += skip, mIdx++)
    {
        if (!IsValid(*pt))
            continue;
        ANode v3(*pt, 0);
        auto itFound = kdtree.find_nearest(v3);
        Match& match = matches[itFound.first->idx];
        const Point3f& pt0 = pts0[itFound.first->idx];
        Vec3f v3f = *pt - pt0;
        float distsq = lengthSquared(v3f);
        if (distsq < maxDistThreshold &&
            (match.matchedIdx < 0 ||
            distsq < match.distsq))
        {
            match.distsq = distsq;
            match.matchedIdx = mIdx;
        }        
    }

    int* cmatch = matchidxs;
    int nmatches = 0;
    for (Match& m : matches)
    {
        if (m.matchedIdx < 0)
            continue;
        *cmatch++ = m.idx;
        *cmatch++ = m.matchedIdx;
        nmatches++;
    }
    return nmatches;
}

inline float powt(float v, int i)
{
    if (i == 1)
        return v;
    else return pow(v, i - 1);
}

float QuatEq(Quatf q, Vec3f v, Vec3f p)
{
    // Extract the vector part of the quaternion
    Vec3f u(q[0], q[1], q[2]);

    // Extract the scalar part of the quaternion
    float s = q[3];

    Vec3f c;
    // Do the math
    Vec3f result = 2.0f * dot(u, v) * u
        + (s * s - dot(u, u)) * v
        + 2.0f * s * gmtl::cross(c, u, v);

    Vec3f dvec = result - p;
    return lengthSquared(dvec);
}

Quatf DerivQuat(Quatf q, Vec3f v, Vec3f p)
{
    float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
    float vx = v[0], vy = v[1], vz = v[2];
    float px = p[0], py = p[1], pz = p[2];

    float dqx = 4 * (-pz * qz * vx + powt(qw, 2) * qx * powt(vx, 2) + powt(qx, 3) * powt(vx, 2) + qx * powt(qy, 2) * powt(vx, 2) +
        qx * powt(qz, 2) * powt(vx, 2) - pz * qw * vy + powt(qw, 2) * qx * powt(vy, 2) + powt(qx, 3) * powt(vy, 2) + qx * powt(qy, 2) * powt(vy, 2) +
        qx * powt(qz, 2) * powt(vy, 2) + pz * qx * vz + powt(qw, 2) * qx * powt(vz, 2) + powt(qx, 3) * powt(vz, 2) +
        qx * powt(qy, 2) * powt(vz, 2) + qx * powt(qz, 2) * powt(vz, 2) + py * (-qy * vx + qx * vy + qw * vz) -
        px * (qx * vx + qy * vy + qz * vz));

    float dqy = 4 * (px * qy * vx + powt(qw, 2) * qy * powt(vx, 2) + powt(qx, 2) * qy * powt(vx, 2) + powt(qy, 3) * powt(vx, 2) + qy * powt(qz, 2) * powt(vx, 2) -
        px * qx * vy + powt(qw, 2) * qy * powt(vy, 2) + powt(qx, 2) * qy * powt(vy, 2) + powt(qy, 3) * powt(vy, 2) +
        qy * powt(qz, 2) * powt(vy, 2) - px * qw * vz + powt(qw, 2) * qy * powt(vz, 2) + powt(qx, 2) * qy * powt(vz, 2) + powt(qy, 3) * powt(vz, 2) +
        qy * powt(qz, 2) * powt(vz, 2) + pz * (qw * vx - qz * vy + qy * vz) -
        py * (qx * vx + qy * vy + qz * vz));

    float dqz = 4 * (px * qz * vx + powt(qw, 2) * qz * powt(vx, 2) + powt(qx, 2) * qz * powt(vx, 2) + powt(qy, 2) * qz * powt(vx, 2) + powt(qz, 3) * powt(vx, 2) +
        px * qw * vy + powt(qw, 2) * qz * powt(vy, 2) + powt(qx, 2) * qz * powt(vy, 2) + powt(qy, 2) * qz * powt(vy, 2) +
        powt(qz, 3) * powt(vy, 2) - px * qx * vz + powt(qw, 2) * qz * powt(vz, 2) + powt(qx, 2) * qz * powt(vz, 2) + powt(qy, 2) * qz * powt(vz, 2) +
        powt(qz, 3) * powt(vz, 2) - py * (qw * vx - qz * vy + qy * vz) -
        pz * (qx * vx + qy * vy + qz * vz));

    float dqw = 4 * (-py * qz * vx + powt(qw, 3) * powt(vx, 2) + qw * powt(qx, 2) * powt(vx, 2) + qw * powt(qy, 2) * powt(vx, 2) +
        qw * powt(qz, 2) * powt(vx, 2) - py * qw * vy + powt(qw, 3) * powt(vy, 2) + qw * powt(qx, 2) * powt(vy, 2) + qw * powt(qy, 2) * powt(vy, 2) +
        qw * powt(qz, 2) * powt(vy, 2) + py * qx * vz + powt(qw, 3) * powt(vz, 2) + qw * powt(qx, 2) * powt(vz, 2) +
        qw * powt(qy, 2) * powt(vz, 2) + qw * powt(qz, 2) * powt(vz, 2) + pz * (qy * vx - qx * vy - qw * vz) -
        px * (qw * vx - qz * vy + qy * vz));

    return Quatf(dqx, dqy, dqz, dqw);
}

class PtScorer
{

    std::vector<Point3f> m_pts0;
    std::vector<Match> m_matches;
    std::vector<Point3f> m_pts1;
    std::vector<Point3f> m_tpts1;
    Matrix44f m_transform;
    KDTree::KDTree <3, ANode> m_kdtree;
    bool m_isinit;
    int m_frameIdx;

public:
    PtScorer(Vec3f* pts0, size_t npts0, Vec3f* pts1, size_t npts1, Matrix44f* pMat,
        int frameIdx) :
        m_isinit(false),
        m_frameIdx(frameIdx)
    {
        m_pts0.resize(npts0);
        memcpy(&m_pts0[0], pts0, npts0 * sizeof(Point3f));
        m_pts1.resize(npts1);
        memcpy(&m_pts1[0], pts1, npts1 * sizeof(Point3f));
        m_transform = *pMat;
        m_transform.mState = Matrix44f::XformState::AFFINE;
    }

    float GetScore(Matrix44f& testTransform)
    {
        if (!m_isinit)
            Init();
        float resultscore = 0;
        for (const Match& match : m_matches)
        {
            Point3f& pt0 = m_pts0[match.idx];
            Point3f& pt1 = m_pts1[match.matchedIdx];
            Point3f tpt1;
            xform(tpt1, testTransform, pt1);
            Vec3f v3f = tpt1 - pt0;
            resultscore += lengthSquared(v3f);
        }
        resultscore /= (float)m_matches.size();
        return log10(1.0 / resultscore);
    }

    ~PtScorer()
    {
        m_pts0.clear();
    }

private:
    void Init()
    {
        size_t ptidx = 0;
        for (Point3f& pt : m_pts0)
        {
            ANode an(pt, ptidx++);
            m_kdtree.insert(an);
        }

        m_kdtree.optimize();
        m_tpts1.resize(m_pts1.size());
        auto ittpt = m_tpts1.begin();
        for (const Point3f& pt1 : m_pts1)
        {
            xform(*ittpt, m_transform, pt1);
            ++ittpt;
        }

        size_t midx = 0;
        std::vector<Match> matches;
        matches.resize(m_pts0.size());
        for (Match& m : matches) { m.idx = midx++; }

        size_t mIdx = 0;
        for (const Point3f& tpt : m_tpts1)
        {
            ANode v3(tpt, 0);
            auto itFound = m_kdtree.find_nearest(v3);
            Match& match = matches[itFound.first->idx];
            const Point3f& pt0 = m_pts0[itFound.first->idx];
            Vec3f v3f = tpt - pt0;
            float distsq = lengthSquared(v3f);
            if (match.matchedIdx < 0 ||
                distsq < match.distsq)
            {
                match.distsq = distsq;
                match.matchedIdx = mIdx;
            }

            mIdx++;
        }

        for (const Match& match : matches)
        {
            if (match.matchedIdx >= 0)
                m_matches.push_back(match);
        }

        m_isinit = true;
    }
};
#undef max
class PTCloudAlign
{

    Matrix44f m_prevTransform;
    Matrix44f m_curTransform;
    int m_curstepsize = -1;

    std::vector<Point3f> m_pts0;
    std::vector<Point3f> m_pts1;
    float m_previousScore = std::numeric_limits<float>::max();

    KDTree::KDTree <3, ANode> m_kdtree;
    std::vector<ANode> m_allNodes;
    std::vector<Match> m_matches;

public:

    PTCloudAlign(Vec3f* ipts0, size_t npts0, Vec3f* ipts1, size_t npts1)
    {
        m_pts0.resize(npts0);
        memcpy(&m_pts0[0], ipts0, sizeof(Vec3f) * npts0);
        m_pts1.resize(npts1);
        memcpy(&m_pts1[0], ipts1, sizeof(Vec3f) * npts1);
    }

    static void GetDerivatives2(
        const std::vector<Point3f>& ptsStart,
        const std::vector<Point3f>& ptsEnd,
        Vec4f rotateVec,
        Vec3f translate,
        Vec4f& uScore,
        Vec3f& tScore)
    {
        float cos_r = cos(rotateVec[3]);
        float sin_r = sin(rotateVec[3]);
        float ux = rotateVec[0];
        float uy = rotateVec[1];
        float uz = rotateVec[2];
        float tx = translate[0];
        float ty = translate[1];
        float tz = translate[2];

        Vec4f ud(0, 0, 0, 0);
        Vec3f td(0, 0, 0);

        float invlen = 1.0 / ptsStart.size();
        for (int i = 0; i < ptsStart.size(); ++i)
        {
            float x_src = ptsEnd[i][0];
            float y_src = ptsEnd[i][1];
            float z_src = ptsEnd[i][2];
            float x_dst = ptsStart[i][0];
            float y_dst = ptsStart[i][1];
            float z_dst = ptsStart[i][2];

            float dr = (2 * ((-tx) + x_dst - ((ux * ux) * (1 - cos_r) + cos_r) * x_src - (ux * uy * (1 - cos_r) + uz * sin_r) * y_src - (ux * uz * (1 - cos_r) - uy * sin_r) * z_src) * ((-((-sin_r) + (ux * ux) * sin_r)) * x_src - (uz * cos_r + ux * uy * sin_r) * y_src - ((-uy) * cos_r + ux * uz * sin_r) * z_src) + 2 * ((-ty) - (ux * uy * (1 - cos_r) - uz * sin_r) * x_src + y_dst - ((uy * uy) * (1 - cos_r) + cos_r) * y_src - (uy * uz * (1 - cos_r) + ux * sin_r) * z_src) * ((-((-uz) * cos_r + ux * uy * sin_r)) * x_src - ((-sin_r) + (uy * uy) * sin_r) * y_src - (ux * cos_r + uy * uz * sin_r) * z_src) + 2 * ((-tz) - (ux * uz * (1 - cos_r) + uy * sin_r) * x_src - (uy * uz * (1 - cos_r) - ux * sin_r) * y_src + z_dst - ((uz * uz) * (1 - cos_r) + cos_r) * z_src) * ((-(uy * cos_r + ux * uz * sin_r)) * x_src - ((-ux) * cos_r + uy * uz * sin_r) * y_src - ((-sin_r) + (uz * uz) * sin_r) * z_src));
            float dux = (2 * ((-uz) * (1 - cos_r) * x_src + sin_r * y_src) * ((-tz) - (ux * uz * (1 - cos_r) + uy * sin_r) * x_src - (uy * uz * (1 - cos_r) - ux * sin_r) * y_src + z_dst - ((uz * uz) * (1 - cos_r) + cos_r) * z_src) + 2 * ((-uy) * (1 - cos_r) * x_src - sin_r * z_src) * ((-ty) - (ux * uy * (1 - cos_r) - uz * sin_r) * x_src + y_dst - ((uy * uy) * (1 - cos_r) + cos_r) * y_src - (uy * uz * (1 - cos_r) + ux * sin_r) * z_src) + 2 * ((-2) * ux * (1 - cos_r) * x_src - uy * (1 - cos_r) * y_src - uz * (1 - cos_r) * z_src) * ((-tx) + x_dst - ((ux * ux) * (1 - cos_r) + cos_r) * x_src - (ux * uy * (1 - cos_r) + uz * sin_r) * y_src - (ux * uz * (1 - cos_r) - uy * sin_r) * z_src));
            float duy = (2 * ((-sin_r) * x_src - uz * (1 - cos_r) * y_src) * ((-tz) - (ux * uz * (1 - cos_r) + uy * sin_r) * x_src - (uy * uz * (1 - cos_r) - ux * sin_r) * y_src + z_dst - ((uz * uz) * (1 - cos_r) + cos_r) * z_src) + 2 * ((-ux) * (1 - cos_r) * x_src - 2 * uy * (1 - cos_r) * y_src - uz * (1 - cos_r) * z_src) * ((-ty) - (ux * uy * (1 - cos_r) - uz * sin_r) * x_src + y_dst - ((uy * uy) * (1 - cos_r) + cos_r) * y_src - (uy * uz * (1 - cos_r) + ux * sin_r) * z_src) + 2 * ((-ux) * (1 - cos_r) * y_src + sin_r * z_src) * ((-tx) + x_dst - ((ux * ux) * (1 - cos_r) + cos_r) * x_src - (ux * uy * (1 - cos_r) + uz * sin_r) * y_src - (ux * uz * (1 - cos_r) - uy * sin_r) * z_src));
            float duz = (2 * ((-ux) * (1 - cos_r) * x_src - uy * (1 - cos_r) * y_src - 2 * uz * (1 - cos_r) * z_src) * ((-tz) - (ux * uz * (1 - cos_r) + uy * sin_r) * x_src - (uy * uz * (1 - cos_r) - ux * sin_r) * y_src + z_dst - ((uz * uz) * (1 - cos_r) + cos_r) * z_src) + 2 * (sin_r * x_src - uy * (1 - cos_r) * z_src) * ((-ty) - (ux * uy * (1 - cos_r) - uz * sin_r) * x_src + y_dst - ((uy * uy) * (1 - cos_r) + cos_r) * y_src - (uy * uz * (1 - cos_r) + ux * sin_r) * z_src) + 2 * ((-sin_r) * y_src - ux * (1 - cos_r) * z_src) * ((-tx) + x_dst - ((ux * ux) * (1 - cos_r) + cos_r) * x_src - (ux * uy * (1 - cos_r) + uz * sin_r) * y_src - (ux * uz * (1 - cos_r) - uy * sin_r) * z_src));
            float dtx = ((-2) * ((-tx) + x_dst - ((ux * ux) * (1 - cos_r) + cos_r) * x_src - (ux * uy * (1 - cos_r) + uz * sin_r) * y_src - (ux * uz * (1 - cos_r) - uy * sin_r) * z_src));
            float dty = ((-2) * ((-ty) - (ux * uy * (1 - cos_r) - uz * sin_r) * x_src + y_dst - ((uy * uy) * (1 - cos_r) + cos_r) * y_src - (uy * uz * (1 - cos_r) + ux * sin_r) * z_src));
            float dtz = ((-2) * ((-tz) - (ux * uz * (1 - cos_r) + uy * sin_r) * x_src - (uy * uz * (1 - cos_r) - ux * sin_r) * y_src + z_dst - ((uz * uz) * (1 - cos_r) + cos_r) * z_src));

            ud[3] += dr;
            ud[0] += dux;
            ud[1] += duy;
            ud[2] += duz;
            td[0] += dtx;
            td[1] += dty;
            td[2] += dtz;
        }

        uScore = ud * invlen;
        tScore = td * invlen;
    }

    static void GetDerivatives(
        const std::vector<Point3f>& ptsStart,
        const std::vector<Point3f>& ptsEnd,
        Vec3f rotateVec,
        Vec3f translate,
        Vec3f& uScore,
        Vec3f& tScore)
    {
        float tx = translate[0];
        float ty = translate[1];
        float tz = translate[2];

        float cosrx = (float)cos(rotateVec[0]);
        float sinrx = (float)sin(rotateVec[0]);
        float cosry = (float)cos(rotateVec[1]);
        float sinry = (float)sin(rotateVec[1]);
        float cosrz = (float)cos(rotateVec[2]);
        float sinrz = (float)sin(rotateVec[2]);
        Vec3f ud(0, 0, 0);
        Vec3f td(0, 0, 0);

        float invlen = 1.0 / ptsStart.size();

        for (int i = 0; i < ptsStart.size(); ++i)
        {
            float x_src = ptsEnd[i][0];
            float y_src = ptsEnd[i][1];
            float z_src = ptsEnd[i][2];
            float x_dst = ptsStart[i][0];
            float y_dst = ptsStart[i][1];
            float z_dst = ptsStart[i][2];

            float derivrx =
                (2 * (-tx + x_dst - cosry * x_src - sinrx * sinry * y_src - cosrx * sinry * z_src) * (-cosrx * sinry * y_src + sinrx * sinry * z_src) + 2 * (-(cosrx * cosry * cosrz - sinrx * sinrz) * y_src - (-cosry * cosrz * sinrx - cosrx * sinrz) * z_src) * (-tz + cosrz * sinry * x_src - (cosry * cosrz * sinrx + cosrx * sinrz) * y_src + z_dst - (cosrx * cosry * cosrz - sinrx * sinrz) * z_src) + 2 * (-ty - sinry * sinrz * x_src + y_dst - (cosrx * cosrz - cosry * sinrx * sinrz) * y_src - (-cosrz * sinrx - cosrx * cosry * sinrz) * z_src) * (-(-cosrz * sinrx - cosrx * cosry * sinrz) * y_src - (-cosrx * cosrz + cosry * sinrx * sinrz) * z_src));
            float derivry =
                (2 * (sinry * x_src - cosry * sinrx * y_src - cosrx * cosry * z_src) * (-tx + x_dst - cosry * x_src - sinrx * sinry * y_src - cosrx * sinry * z_src) + 2 * (-cosry * sinrz * x_src - sinrx * sinry * sinrz * y_src - cosrx * sinry * sinrz * z_src) * (-ty - sinry * sinrz * x_src + y_dst - (cosrx * cosrz - cosry * sinrx * sinrz) * y_src - (-cosrz * sinrx - cosrx * cosry * sinrz) * z_src) + 2 * (cosry * cosrz * x_src + cosrz * sinrx * sinry * y_src + cosrx * cosrz * sinry * z_src) * (-tz + cosrz * sinry * x_src - (cosry * cosrz * sinrx + cosrx * sinrz) * y_src + z_dst - (cosrx * cosry * cosrz - sinrx * sinrz) * z_src));
            float derivrz =
                (2 * (-sinry * sinrz * x_src - (cosrx * cosrz - cosry * sinrx * sinrz) * y_src - (-cosrz * sinrx - cosrx * cosry * sinrz) * z_src) * (-tz + cosrz * sinry * x_src - (cosry * cosrz * sinrx + cosrx * sinrz) * y_src + z_dst - (cosrx * cosry * cosrz - sinrx * sinrz) * z_src) + 2 * (-ty - sinry * sinrz * x_src + y_dst - (cosrx * cosrz - cosry * sinrx * sinrz) * y_src - (-cosrz * sinrx - cosrx * cosry * sinrz) * z_src) * (-cosrz * sinry * x_src - (-cosry * cosrz * sinrx - cosrx * sinrz) * y_src - (-cosrx * cosry * cosrz + sinrx * sinrz) * z_src));
            float derivtx =
                -2 * (-tx + x_dst - cosry * x_src - sinrx * sinry * y_src - cosrx * sinry * z_src);
            float derivty =
                -2 * (-ty - sinry * sinrz * x_src + y_dst - (cosrx * cosrz - cosry * sinrx * sinrz) * y_src - (-cosrz * sinrx - cosrx * cosry * sinrz) * z_src);
            float derivtz =
                -2 * (-tz + cosrz * sinry * x_src - (cosry * cosrz * sinrx + cosrx * sinrz) * y_src + z_dst - (cosrx * cosry * cosrz - sinrx * sinrz) * z_src);

            ud += Vec3f(derivrx, derivry, derivrz);
            td += Vec3f(derivtx, derivty, derivtz);
        }

        uScore = ud * invlen;
        tScore = td * invlen;
    }

    static float BestFit2(const std::vector<Point3f>& ptsStart,
        const std::vector<Point3f>& ptsEnd,
        Vec3f& translate,
        Vec4f& rotate,
        const Vec4f& dvals)
    {
        Vec3f trans(0, 0, 0);
        Vec4f rot(1, 0, 0, 0);
        float r = 0;
        float totalScore;
        for (int i = 0; i < 100; ++i)
        {
            Vec4f uScore;
            Vec3f tScore;
            GetDerivatives2(ptsStart, ptsEnd, rot, trans, uScore, tScore);
            totalScore = fabs(length(uScore) + length(tScore));
            char tmp[1024];
            sprintf(tmp, "%f  [%f %f %f] r=%f [%f %f %f]\n", totalScore, tScore[0], tScore[1], tScore[2], uScore[3], uScore[0], uScore[1], uScore[2]);
            //OutputDebugStringA(tmp);
            trans += tScore * dvals[0];
            rot[0] += uScore[0] * dvals[1];
            rot[1] += uScore[1] * dvals[1];
            rot[2] += uScore[2] * dvals[1];
            rot[3] += uScore[3] * dvals[2];

            gmtl::normalize((Vec3f&)rot);
        }

        translate = trans;
        rotate = rot;
        return totalScore;
    }

    static float BestFit(const std::vector<Point3f>& ptsStart,
        const std::vector<Point3f>& ptsEnd,
        Vec3f& translate,
        Vec3f& rotate,
        const Vec4f& dvals)
    {
        Vec3f trans(0, 0, 0);
        Vec3f rot(0, 0, 0);
        float r = 0;
        float totalScore;
        for (int i = 0; i < 100; ++i)
        {
            Vec3f uScore;
            Vec3f tScore;
            GetDerivatives(ptsStart, ptsEnd, rot, trans, uScore, tScore);
            totalScore = fabs(length(uScore) + length(tScore));
            char tmp[1024];
            sprintf(tmp, "%f  [%f %f %f] [%f %f %f]\n", totalScore, tScore[0], tScore[1], tScore[2], uScore[0], uScore[1], uScore[2]);
            Dbg(tmp);
            trans += tScore * dvals[0];
            rot[0] += uScore[0] * dvals[1];
            rot[1] += uScore[1] * dvals[2];
            rot[2] += uScore[2] * dvals[3];
        }

        translate = trans;
        rotate = rot;
        return totalScore;
    }

    int AlignStep(Matrix44f& outTransform)
    {
        if (m_curstepsize < 0)
        {
            m_matches.resize(m_pts0.size());
            size_t midx = 0;
            for (Match& m : m_matches) { m.idx = midx++; }

            int ptidx = 0;
            for (Point3f& pt : m_pts0)
            {
                ANode an(pt, ptidx);
                m_allNodes.push_back(an);
                m_kdtree.insert(an);
                ptidx++;
            }

            m_kdtree.optimize();
            m_curstepsize = 1;// m_pts0.size() / 10;
        }

        for (Match& m : m_matches)
        {
            m.matchedIdx = -1;
            m.distsq = 0;
        }

        std::vector<Point3f> pts1t;
        Matrix44f transfrm = m_curTransform;
        for (size_t idx = 0; idx < m_pts1.size(); idx += m_curstepsize)
        {
            Point3f tpt;
            xform(tpt, transfrm, m_pts1[idx]);
            pts1t.push_back(tpt);
        }

        size_t mIdx = 0;
        for (const Point3f& tpt : pts1t)
        {
            const ANode& v3 = (const ANode&)tpt;
            auto itFound = m_kdtree.find_nearest(v3);
            Match& match = m_matches[itFound.first->idx];
            const Point3f& pt0 = m_pts0[itFound.first->idx];
            Vec3f v3f = tpt - pt0;
            float distsq = lengthSquared(v3f);
            if (match.matchedIdx < 0 ||
                distsq < match.distsq)
            {
                match.distsq = distsq;
                match.matchedIdx = mIdx;
            }

            mIdx++;
        }


        std::vector<Match> matches;
        for (const Match& match : m_matches)
        {
            if (match.matchedIdx >= 0)
                matches.push_back(match);
        }

        float avgdistsq = 0;
        for (Match& match : matches)
        {
            match.distsq =
                lengthSquared(Vec3f(m_pts0[match.idx] - pts1t[match.matchedIdx]));
            avgdistsq += match.distsq;
        }

        avgdistsq /= (float)matches.size();
        avgdistsq *= 4.0;

        for (auto itMatch = matches.begin(); itMatch != matches.end();)
        {
            if (itMatch->distsq < avgdistsq)
                itMatch = matches.erase(itMatch);
            else
                itMatch++;
        }

        std::vector<Point3f> vecs0;
        std::vector<Point3f> vecs1;
        for (Match& m : matches)
        {
            vecs0.push_back(m_pts0[m.idx]);
            vecs1.push_back(pts1t[m.matchedIdx]);
        }

        Vec3f translate2;
        Vec3f rotate2;
        BestFit(vecs0, vecs1, translate2, rotate2,
            Vec4f(-0.5f, -0.005f, -0.010f, -0.005f));

        AxisAnglef aaY(rotate2[1], Vec3f(0, 1, 0));
        AxisAnglef aaZ(rotate2[2], Vec3f(0, 0, 1));
        Quatf qx = make<Quatf, AxisAnglef>(AxisAnglef(rotate2[0], Vec3f(1, 0, 0)));
        Quatf qy = make<Quatf, AxisAnglef>(AxisAnglef(rotate2[1], Vec3f(0, 1, 0)));
        Quatf qz = make<Quatf, AxisAnglef>(AxisAnglef(rotate2[2], Vec3f(1, 0, 0)));
        Quatf q = qx * qy * qz;
        Matrix44f rotmat = makeRot<Matrix44f, Quatf>(q);
        Matrix44f trnsmat = makeTrans<Matrix44f, Vec3f>(translate2);
        Matrix44f transform = rotmat * trnsmat;


        float resultscore = 0;
        float oldscore = 0;
        for (int idx = 0; idx < vecs0.size(); idx++)
        {
            Point3f tpt1;
            xform(tpt1, transform, vecs1[idx]);
            resultscore += lengthSquared(Vec3f(vecs0[idx] - tpt1));
            oldscore += lengthSquared(Vec3f(vecs0[idx] - vecs1[idx]));
        }
        resultscore /= (float)(vecs0.size() * vecs0.size());
        oldscore /= (float)(vecs0.size() * vecs0.size());

        int retstg;
        if (resultscore > m_previousScore)
        {
            int nsteps = m_pts0.size() / m_curstepsize;
            m_curstepsize /= 2;
            m_prevTransform = m_curTransform;
            retstg = 1;
            if (nsteps > 100)
                retstg = 2;
        }
        else
        {
            m_curTransform = m_curTransform * transform;
            m_previousScore = resultscore;
            retstg = 0;
        }

        outTransform = m_curTransform;
        return retstg;
    }
};

int GetNearest(Point3f* pts0, size_t ptCount0, Point3f* pts1, size_t ptCount1, int* outMatches)
{
    KDTree::KDTree <3, ANode> kdtree;
    size_t ptidx = 0;

    for (Point3f* pt = pts0; pt != (pts0 + ptCount0); ++pt)
    {
        ANode an(*pt, ptidx++);
        kdtree.insert(an);
    }

    kdtree.optimize();

    size_t midx = 0;
    std::vector<Match> matches;
    matches.resize(ptCount0);
    for (Match& m : matches) { m.idx = midx++; }

    size_t mIdx = 0;
    for (Point3f* pt1 = pts1; pt1 != (pts1 + ptCount1); ++pt1)
    {
        ANode v3(*pt1, 0);
        auto itFound = kdtree.find_nearest(v3);
        Match& match = matches[itFound.first->idx];

        const Point3f& pt0 = pts0[itFound.first->idx];
        Vec3f v3f = *pt1 - pt0;
        float distsq = lengthSquared(v3f);
        if (match.matchedIdx < 0 ||
            distsq < match.distsq)
        {
            match.distsq = distsq;
            match.matchedIdx = mIdx;
        }

        mIdx++;
    }

    int nMatches = 0;
    for (const Match& match : matches)
    {
        if (match.matchedIdx >= 0)
        {
            *outMatches++ = match.idx;
            *outMatches++ = match.matchedIdx;
            nMatches++;
        }
    }

    return nMatches;
}

PtScorer* CreatePtScorer(float* m_pts0, size_t ptCount0, float* m_pts1, size_t ptCount1, float* pmatrix,
    int frameIdx)
{
    return new PtScorer((Vec3f*)m_pts0, ptCount0 / 3, (Vec3f*)m_pts1, ptCount1 / 3, (Matrix44f*)pmatrix, frameIdx);
}

float GetScore(PtScorer* pthis, float* pmatrix)
{
    return pthis->GetScore((Matrix44f&)*pmatrix);
}

void FreePtScorer(PtScorer* pthis)
{
    delete pthis;
}

PTCloudAlign* CreatePtCloudAlign(Vec3f *pts0, size_t ptCount0, Vec3f* pts1, size_t ptCount1)
{
    return new PTCloudAlign(pts0, ptCount0, pts1, ptCount1);
}

int AlignStep(PTCloudAlign* pthis, float* matrix)
{
    return pthis->AlignStep((Matrix44f&)*matrix);
}

void FreePtCloudAlign(PTCloudAlign* pthis)
{
    delete pthis;
}

int FindMatches(float* pts0, size_t ptCount0, float* pts1, size_t ptCount1,
    float maxDistThreshold,
    int* matches)
{
    return MatchesT2((Point3f*)pts0, ptCount0, (Point3f*)pts1, ptCount1, maxDistThreshold, matches);
}

inline float randfl()
{
    const float invRand = 1.0f / RAND_MAX;
    return rand() * invRand;
}

const int cnt = 20;
const int dcnt = cnt * 2;
const float rscale = 10.0f / cnt;;
const float vscale = 0.1f / cnt;;
std::vector<int> scores;

static inline Vec3f RotX(float a, Vec3f v)
{
    float cosA = (float)cos(a);
    float sinA = (float)sin(a);
    return Vec3f(v[0], v[1] * cosA - v[2] * sinA, v[1] * sinA + v[2] * cosA);
}

static Vec3f RotY(float b, Vec3f v)
{
    float cosB = (float)cos(b);
    float sinB = (float)sin(b);
    return Vec3f(v[0] * cosB - v[2] * sinB, v[1], v[0] * sinB + v[2] * cosB);
}

static Vec3f RotZ(float c, Vec3f v)
{
    float cosC = (float)cos(c);
    float sinC = (float)sin(c);
    return Vec3f(v[0] * cosC - v[1] * sinC, v[0] * sinC + v[1] * cosC, v[2]);
}


void CalcNormals(Point3f* pts0, Vec3f* nrm, int dw, int dh)
{
    for (int y = 0; y < (dh - 1); ++y)
    {
        for (int x = 0; x < (dw - 1); ++x)
        {
            if (!IsValid(pts0[(y)*dw + (x)]))
                continue;
            Point3f* borders[4] = { &pts0[(y + 1) * dw + (x + 1)],
                &pts0[(y + 1) * dw + (x)],
                &pts0[(y)*dw + (x + 1)],
                &pts0[(y)*dw + (x)] };

            Point3f* vpts[3];
            int vidx = 0;
            for (int idx = 0; idx < 4; ++idx)
            {
                if (IsValid(*borders[idx]))
                    vpts[vidx++] = borders[idx];
                if (vidx >= 3)
                    break;
            }

            Vec3f& n = nrm[(y)*dw + (x)];
            if (vidx < 3)
            {
                continue;
            }
            Vec3f i = *vpts[0] - *vpts[1];
            Vec3f j = *vpts[2] - *vpts[1];
            normalize(i);
            normalize(j);
            cross(n, i, j);
            normalize(n);
        }
        nrm[(y)*dw + (dw - 1)] = Vec3f(0, 0, 1);
    }
    for (int x = 0; x < dw; ++x)
    {
        nrm[(dh - 1) * dw + x] = Vec3f(0, 0, 1);
    }

}


void CalcDepthPts(const Matrix44f& camMatrix, float* depthVals, int width, int height, std::vector<Point3f>& outPts)
{
    float lastGoodDepth = -1;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depthVal = depthVals[y * width + x];
            if (isnan(depthVal))
                outPts.push_back(Point3f(NAN, NAN, NAN));
            else
            {

                float z = 1 / depthVal;
                Vec4f modelPos = camMatrix * Vec4f(x, y, z, 1);
                modelPos /= modelPos[3];
                outPts.push_back(Point3f(modelPos[0], modelPos[1], modelPos[2]));
            }
        }
    }
}

bool BestFitLod(Point3f* pts0, Vec3f *nrm0, size_t ptCount0, Point3f* pts1, size_t ptCount1,
    int dw, int dh,
    float maxDistThreshold,
    Vec3f* outTranslate,
    Vec3f* outRotate)
{
    bool success = true;

    std::vector<int> matches;
    matches.resize(ptCount0 + ptCount1);
    int nmatches = MatchesT2(pts0, ptCount0, pts1, ptCount1, maxDistThreshold, matches.data());

    std::vector<Point3f> pvec0;
    std::vector<Point3f> pnrm0;
    std::vector<Point3f> pvec1;

    for (int idx = 0; idx < nmatches; idx++)
    {
        pvec0.push_back(pts0[matches[idx * 2]]);
        pnrm0.push_back(nrm0[matches[idx * 2]]);
        pvec1.push_back(pts1[matches[idx * 2 + 1]]);
    }

    /*
    {
        float totalDist2 = 0;
        for (size_t idx = 0; idx < pvec0.size(); ++idx)
        {
            float dp = dot((pvec1[idx] - pvec0[idx]), pnrm0[idx]);
            totalDist2 += dp * dp;
        }

        char tmp[1024];
        sprintf_s(tmp, "O=%f\n", totalDist2);
        OutputDebugStringA(tmp);
    }
    */
    Vec3f bestRot;
    Vec3f bestOffset;
    float prevErr = 100000;

    float div = 1.0f / pvec0.size();
    {
        Vec3f r(0, 0, 0);
        Vec3f t(0, 0, 0);
        float err = 0;
        for (int tIdx = 0; tIdx < 100; ++tIdx)
        {
            Vec3f dr(0, 0, 0);
            Vec3f dt(0, 0, 0);
            float rx = r[0];
            float ry = r[1];
            float rz = r[2];
            float tx = t[0];
            float ty = t[1];
            float tz = t[2];

            float cosrx = cos(rx);
            float cosry = cos(ry);
            float cosrz = cos(rz);
            float sinrx = sin(rx);
            float sinry = sin(ry);
            float sinrz = sin(rz);
            for (size_t idx = 0; idx < pvec0.size(); ++idx)
            {
                float px = pvec0[idx][0];
                float py = pvec0[idx][1];
                float pz = pvec0[idx][2];
                float qx = pvec1[idx][0];
                float qy = pvec1[idx][1];
                float qz = pvec1[idx][2];
                float nx = pnrm0[idx][0];
                float ny = pnrm0[idx][1];
                float nz = pnrm0[idx][2];

                float dtx = 2 * (-nx * cosry * cosrz - cosrx * (nz * cosrz * sinry + ny * sinrz) + sinrx * (ny * cosrz * sinry - nz * sinrz)) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));
                float dty = 2 * (-nz * cosrz * sinrx + (nx * cosry - ny * sinrx * sinry) * sinrz + cosrx * (-ny * cosrz + nz * sinry * sinrz)) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));
                float dtz = 2 * (-nz * cosrx * cosry + ny * cosry * sinrx + nx * sinry) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));

                float drx = 2 * (ny * (sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + nz * (-cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz)))) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));
                float dry = 2 * (-nz * cosrx * (-(qz + tz) * sinry + cosry * ((qx + tx) * cosrz - (qy + ty) * sinrz)) + ny * sinrx * (-(qz + tz) * sinry + cosry * ((qx + tx) * cosrz - (qy + ty) * sinrz)) + nx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));
                float drz = 2 * (nx * cosry * ((qy + ty) * cosrz + (qx + tx) * sinrz) - sinrx * (cosrz * (nz * (qx + tx) + ny * (qy + ty) * sinry) + (-nz * (qy + ty) + ny * (qx + tx) * sinry) * sinrz) + cosrx * (cosrz * (-ny * (qx + tx) + nz * (qy + ty) * sinry) + (ny * (qy + ty) + nz * (qx + tx) * sinry) * sinrz)) * (nx * (px + (qz + tz) * sinry + cosry * (-(qx + tx) * cosrz + (qy + ty) * sinrz)) + nz * (pz - sinrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) - cosrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))) + ny * (py - cosrx * ((qy + ty) * cosrz + (qx + tx) * sinrz) + sinrx * ((qz + tz) * cosry + sinry * ((qx + tx) * cosrz - (qy + ty) * sinrz))));

                dr += Vec3f(drx, dry, drz);
                dt += Vec3f(dtx, dty, dtz);
            }

            const float srMult = 0.05f;
            const float stMult = 0.5f;
            dr *= div;
            r -= dr * srMult;
            dt *= div;
            t -= dt * stMult;

            err = lengthSquared(dr) + lengthSquared(dt);
            prevErr = err;
        }

        if (err > 0.01f)
        {
            char tmp[1024];
            sprintf(tmp, "err=%f\n", err);
            Dbg(tmp);
            success = false;
        }
        bestRot = r;
        bestOffset = t;
    }

    /*
{
        float totalDist2 = 0;
        for (size_t idx = 0; idx < pvec0.size(); ++idx)
        {
            pvec1[idx] = RotZ(bestRot[2], RotY(bestRot[1], RotX(bestRot[0], pvec1[idx])));
            float dp = dot(pvec1[idx] - pvec0[idx], pnrm0[idx]);
            totalDist2 += dp * dp;
        }
        char tmp[1024];
        sprintf_s(tmp, "B=%f\n", totalDist2);
        OutputDebugStringA(tmp);
    }
    */

    *outTranslate = bestOffset;
    //*outRotate = Vec3f(0, 0, 0);
    *outRotate = bestRot;
    return success;
}


void BestFit(float* _pts0, float* _nrm0, size_t ptCount0, float* _pts1, size_t ptCount1,
    int dw, int dh,
    float maxDistThreshold,
    Matrix44f* outTransform)
{
    Point3f* pts0 = (Point3f*)_pts0;
    Point3f* pts1 = (Point3f*)_pts1;
    Vec3f* nrm0 = (Vec3f*)_nrm0;

    Vec3f translate(0, 0, 0);
    Vec3f rotate(0, 0, 0);

    BestFitLod(pts0, nrm0, ptCount0, pts1, ptCount1, dw, dh, maxDistThreshold, &translate, &rotate);

    gmtl::EulerAngleXYZf euler(rotate[0], -rotate[1], rotate[2]);
    gmtl::Quatf quat = make<gmtl::Quatf>(euler);
    Matrix44f rotmat = makeRot<Matrix44f, Quatf>(quat);
    Matrix44f trnsmat = makeTrans<Matrix44f, Vec3f>(translate);
    *outTransform  = (rotmat * trnsmat);
}

bool BestFitAll(float* _vals0, float* _vals1, int width, int height,
    float* cameraVals,
    float maxDistThreshold,
    Matrix44f* outTransform)
{

    Vec4f cameraCalibrationVals;
    Vec2f cameraCalibrationDims;
    memcpy(&cameraCalibrationVals, cameraVals, sizeof(float) * 4);
    memcpy(&cameraCalibrationDims, cameraVals + 4, sizeof(float) * 2);

    int dw = width;
    int dh = height;

    int totalFloats = 0;
    int nLods = 0;
    while (dw >= 32)
    {
        dw /= 2;
        dh /= 2;

        totalFloats += (dw * dh);
        nLods++;
    }


    float* vals[2] = { _vals0, _vals1 };
    std::vector<float> dlods[8][2];
    for (int vIdx = 0; vIdx < 2; ++vIdx)
    {
        std::vector<float> depthLods;
        depthLods.resize(totalFloats);
        DepthBuildLods(vals[vIdx], depthLods.data(), width, height);

        dw = width;
        dh = height;

        float* ptr = depthLods.data();
        size_t offset = 0;
        for (int i = 0; i < nLods; ++i)
        {
            dw /= 2;
            dh /= 2;

            dlods[i][vIdx].resize(dw * dh);
            memcpy(dlods[i][vIdx].data(), ptr + offset, dw * dh * sizeof(float));
            offset += dw * dh;
        }
    }

    bool success = true;
    Matrix44f alignTransform;
    for (int curLod = nLods; curLod >= 0; --curLod)
    {
        float* vals0 = nullptr, * vals1 = nullptr;
        if (curLod > 0)
        {
            vals0 = dlods[curLod - 1][0].data();
            vals1 = dlods[curLod - 1][1].data();
        }
        else
        {
            vals0 = vals[0];
            vals1 = vals[1];
        }

        int dw = width >> curLod;
        int dh = height >> curLod;

        Matrix44f camMatrix =
            CameraMat(cameraCalibrationVals, cameraCalibrationDims, dw, dh);

        std::vector<Point3f> pts0;
        CalcDepthPts(camMatrix, vals0, dw, dh, pts0);

        std::vector<Point3f> pts1;
        CalcDepthPts(camMatrix, vals1, dw, dh, pts1);

        for (auto itPt = pts1.begin(); itPt != pts1.end(); ++itPt)
        {
            gmtl::xform(*itPt, alignTransform, *itPt);
        }

        Vec3f translate(0, 0, 0);
        Vec3f rotate(0, 0, 0);
        std::vector<Vec3f> normals;
        normals.resize(pts0.size());
        CalcNormals(pts0.data(), normals.data(), dw, dh);
        success &= BestFitLod(pts0.data(), normals.data(), pts0.size(), pts1.data(), pts1.size(), dw, dh, 
            maxDistThreshold, &translate, &rotate);

        gmtl::EulerAngleXYZf euler(rotate[0], -rotate[1], rotate[2]);
        gmtl::Quatf quat = make<gmtl::Quatf>(euler);
        Matrix44f rotmat = makeRot<Matrix44f, Quatf>(quat);
        Matrix44f trnsmat = makeTrans<Matrix44f, Vec3f>(translate);
        alignTransform *= (rotmat * trnsmat);
        if (curLod == 2)
            break;
    }

    *outTransform = alignTransform;
    return success;
}


void CalcScores()
{
    auto itmax = std::max_element(scores.begin(), scores.end());
    size_t ival = itmax - scores.begin();
    size_t ridx = ival / dcnt;
    size_t vidx = ival % dcnt;
    float r = ((int)ridx - cnt) * rscale;
    float v = ((int)vidx - cnt) * vscale;
}

Matrix44f CameraMat(const Vec4f& cameraCalibrationVals,
    const Vec2f& cameraCalibrationDims,
    int dw, int dh)
{
    float ratioX = cameraCalibrationDims[0] / dw;
    float ratioY = cameraCalibrationDims[1] / dh;
    Vec4f cMat = cameraCalibrationVals;
    float xScl = ratioX / cMat[0];
    float yScl = ratioY / cMat[1];
    float xOff = cMat[2] / ratioX + (30 * dw / 640);
    float yOff = cMat[3] / ratioY;
    Matrix44f proj;
    proj.set(
        xScl, 0, 0, -xOff * xScl,
        0, yScl, 0, -yOff * yScl,
        0, 0, 0, 1,
        0, 0, 1, 0);

    const float PI = 3.14159265358979323846f;
    Matrix44f r1 = gmtl::makeRot<Matrix44f, AxisAnglef>(AxisAnglef(PI / 2.0f, 0, 0, 1));
    Matrix44f r2 = gmtl::makeRot<Matrix44f, AxisAnglef>(AxisAnglef(PI, 0, 1, 0));
    Matrix44f cm = r1 * r2 * proj;

    //std::ostringstream str;
    //str << proj << std::endl << r1 << std::endl << r2 << std::endl << cm << std::endl;
    //OutputDebugStringA(str.str().c_str());
    return cm;
}

}