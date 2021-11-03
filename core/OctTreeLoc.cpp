// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <algorithm>
#include "Pt.h"
#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/VecOps.h>
#include <gmtl/Matrix.h>
#include <vector>
#include "OctTreeLoc.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

inline Vec3f mulvec(const Vec3f& v1, const Vec3f& v2)
{
    return Vec3f(v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2]);
}

AABoxf OctTreeLoc::GetBounds(const AABoxf& bounds) const
{
    Vec3f extents =
        (bounds.getMax() - bounds.getMin()) / (float)((uint32)1 << m_level);

    
    Vec3f minPt(m_x, m_y, m_z);
    Vec3f maxPt(m_x + 1, m_y + 1, m_z + 1);
    
    return AABoxf(bounds.getMin() + mulvec(minPt, extents),
        bounds.getMin() + mulvec(maxPt, extents));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OctTreeLoc OctTreeLoc::GetTileAtLoc(const Vec3f& pt, uint32 level, const AABoxf& bounds)
{
    Vec3f extents =
        (bounds.getMax() - bounds.getMin()) / (float)((uint32)1 << level);
    extents[0] = 1.0f / extents[0];
    extents[1] = 1.0f / extents[1];
    extents[2] = 1.0f / extents[2];

    Vec3f xyz = mulvec(pt - bounds.getMin(), extents);
    return OctTreeLoc(level, xyz[0], xyz[1], xyz[2]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OctTreeLoc OctTreeLoc::GetChild(size_t idx) const
{
    int xInc = idx & 1;
    int yInc = (idx >> 1) & 1;
    int zInc = (idx >> 2) & 1;
    return OctTreeLoc(m_level + 1, (m_x << 1) + xInc,
        (m_y << 1) + yInc,
        (m_z << 1) + zInc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OctTreeLoc OctTreeLoc::ParentAtLevel(uint32 level) const
{
    uint32 levelShift = (m_level - level);
    return OctTreeLoc(level, m_x >> levelShift,
        m_y >> levelShift,
        m_z >> levelShift);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool OctTreeLoc::IsValid() const
{
    if (m_level == sInvalidLOD)
        return false;
    uint32 maxTileIdx = 1 << m_level;
    if (m_x >= maxTileIdx ||
        m_y >= maxTileIdx ||
        m_z >= maxTileIdx)
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OctTreeLoc OctTreeLoc::GlobalToLoc(const OctTreeLoc& parentTile) const
{
    // Check if global location LOD is too low.
    int levelDelta = m_level - parentTile.m_level;
    if (levelDelta < 0)
    {
        return OctTreeLoc(sInvalidLOD, 0, 0, 0);
    }

    // Check if the location in the LOD is outside the root tile(s).
    uint32 globalRootX = m_x >> levelDelta;
    uint32 globalRootY = m_y >> levelDelta;
    uint32 globalRootZ = m_z >> levelDelta;

    if (globalRootX != parentTile.m_x || globalRootY != parentTile.m_y ||
        globalRootZ != parentTile.m_z)
    {
        return OctTreeLoc(sInvalidLOD, 0, 0, 0);
    }

    uint32 startXAtCurrentLevel = parentTile.m_x << levelDelta;
    uint32 startYAtCurrentLevel = parentTile.m_y << levelDelta;
    uint32 startZAtCurrentLevel = parentTile.m_z << levelDelta;

    return OctTreeLoc(
        levelDelta,
        m_x - startXAtCurrentLevel,
        m_y - startYAtCurrentLevel,
        m_z - startZAtCurrentLevel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

size_t OctTreeLoc::IndexOfChild(const OctTreeLoc& child) const
{
    if (child.m_level <= m_level)
        return (size_t)(-1);

    OctTreeLoc childLoc = child;

    if (child.m_level > (m_level + 1))
        childLoc = childLoc.ParentAtLevel(m_level + 1);

    uint32 localX = childLoc.m_x - (m_x << 1);
    uint32 localY = childLoc.m_y - (m_y << 1);
    uint32 localZ = childLoc.m_z - (m_z << 1);

    if (localX > 1 || localY > 1 || localZ > 1)
        return (size_t)(-1);

    return (localZ << 2) | (localY << 1) | localX;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool OctTreeLoc::IsChildOf(const OctTreeLoc& parent) const
{
    int levelDelta = m_level - parent.m_level;
    if (levelDelta < 0)
    {
        return false;
    }

    // Check if the location in the LOD is outside the root tile(s).
    uint32 globalRootX = m_x >> levelDelta;
    uint32 globalRootY = m_y >> levelDelta;
    uint32 globalRootZ = m_z >> levelDelta;

    if (globalRootX != parent.m_x || globalRootY != parent.m_y ||
        globalRootZ != parent.m_z)
    {
        return false;
    }

    return true;
}
