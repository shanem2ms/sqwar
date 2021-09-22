#pragma once

#include "SceneItem.h"

namespace sam
{
    struct DrawContext;


    class Square : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;

    public:
        int m_state;
        Square() : m_state(0) {}
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}