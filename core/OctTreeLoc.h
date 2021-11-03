////////////////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT © 2019 by WSI Corporation
////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file OctTreeLoc.h
/// Interface for the CycloidalNoise class.
/// \author Shane Morrison
#pragma once

#include <gmtl/Vec.h>
#include <gmtl/AABox.h>
using namespace gmtl;

class OctTreeLoc
{
public:

    static const uint32 sInvalidLOD = (uint32)(-1);
    OctTreeLoc() : m_level(sInvalidLOD) {}

    OctTreeLoc(uint32 level, uint32 x, uint32 y, uint32 z) :
        m_level(level),
        m_x(x),
        m_y(y),
        m_z(z) {}

    uint32 m_level;
    uint32 m_x;
    uint32 m_y;
    uint32 m_z;


    AABoxf GetBounds(const AABoxf& bounds) const;
    static OctTreeLoc GetTileAtLoc(const Vec3f& pt, uint32 level, const AABoxf& bounds);

    OctTreeLoc GetChild(size_t idx) const;
    OctTreeLoc ParentAtLevel(uint32 level) const;
    OctTreeLoc GlobalToLoc(const OctTreeLoc& localRoot) const;
    size_t IndexOfChild(const OctTreeLoc& child) const;
    bool IsValid() const;
    bool IsChildOf(const OctTreeLoc& parent) const;

    /// Equality comparison.
    bool operator==(const OctTreeLoc& rhs) const;

    /// Inequality comparison.
    bool operator!=(const OctTreeLoc& rhs) const
    {
        return !(*this == rhs);
    }

    /// Less Than comparison.
    bool operator<(const OctTreeLoc& rhs) const;

    /// Greater Than comparison.
    bool operator>(const OctTreeLoc& rhs) const;

    /// Less Than or Equal comparison.
    bool operator<=(const OctTreeLoc& rhs) const
    {
        return !(*this > rhs);
    }

    /// Greater Than or Equal comparison.
    bool operator>=(const OctTreeLoc& rhs) const
    {
        return !(*this < rhs);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Inlines
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool OctTreeLoc::operator==(const OctTreeLoc& rhs) const
{
    return (m_level == rhs.m_level &&
        m_x == rhs.m_x &&
        m_y == rhs.m_y &&
        m_z == rhs.m_z);
}

inline bool OctTreeLoc::operator<(const OctTreeLoc& rhs) const
{
    if (m_level != rhs.m_level)
        return m_level < rhs.m_level;
    if (m_x != rhs.m_x)
        return m_x < rhs.m_x;
    if (m_y != rhs.m_y)
        return m_y < rhs.m_y;
    if (m_z != rhs.m_z)
        return m_z < rhs.m_z;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool OctTreeLoc::operator>(const OctTreeLoc& rhs) const
{
    if (m_level != rhs.m_level)
        return m_level > rhs.m_level;
    if (m_x != rhs.m_x)
        return m_x > rhs.m_x;
    if (m_y != rhs.m_y)
        return m_y > rhs.m_y;
    if (m_z != rhs.m_z)
        return m_z > rhs.m_z;
    return false;
}

struct OctLocator
{
    OctTreeLoc loc;
    uint64 offset;
};
#pragma once
