#pragma once

#include "SceneItem.h"

namespace sam
{
    struct DrawContext;


    class Square : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;
        bgfx::UniformHandle m_texture;
        bgfxh<bgfx::TextureHandle> m_tex;
    public:
        Square() {}
        void SetDepthData(const std::vector<unsigned char>& data);
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}