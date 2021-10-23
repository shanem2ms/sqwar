#include "StdIncludes.h"
#include "Mesh.h"

bgfx::VertexLayout PosTexcoordVertex::ms_layout;
bgfx::VertexLayout PosTexcoordNrmVertex::ms_layout;
bgfx::VertexLayout VoxelVertex::ms_layout;

bool Cube::isInit = false;
bgfx::VertexBufferHandle Cube::vbh;
bgfx::IndexBufferHandle Cube::ibh;


bool Quad::isInit = false;
bgfx::VertexBufferHandle Quad::vbh;
bgfx::IndexBufferHandle Quad::ibh;

template <int N> void Grid<N>::init()
{
    if (isInit)
        return;
    PosTexcoordVertex::init();
    const int size = 16;
    vertices.resize(size * size);
    for (int x = 0; x < size; ++x)
    {
        for (int y = 0; y < size; ++y)
        {
            PosTexcoordVertex& v = vertices[y * size + x];
            v.m_x = ((float)(x) / (float)(size - 1)) * 2 - 1;
            v.m_z = ((float)(y) / (float)(size - 1)) * 2 - 1;
            v.m_y = 0;
            v.m_u = ((float)(x) / (float)(size - 1));
            v.m_v = ((float)(y) / (float)(size - 1));

            if (x < size - 1 &&
                y < size - 1)
            {
                int idx = y * size + x;
                indices.push_back(idx + 1);
                indices.push_back(idx + size);
                indices.push_back(idx);

                indices.push_back(idx + size + 1);
                indices.push_back(idx + size);
                indices.push_back(idx + 1);
            }
        }
    }

    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosTexcoordVertex)),
        PosTexcoordVertex::ms_layout
    );

    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t))
    );

    isInit = true;
}

template <int N> bool Grid<N>::isInit = false;
template <int N> bgfx::VertexBufferHandle Grid<N>::vbh;
template <int N> bgfx::IndexBufferHandle Grid<N>::ibh;
template <int N> std::vector<PosTexcoordVertex> Grid<N>::vertices;
template <int N> std::vector<uint16_t> Grid<N>::indices;
template class Grid<16>;



void CubeList::Create(const std::vector<Vec3f>& pts, float cubeSize)
{
    PosTexcoordNrmVertex::init();


    static PosTexcoordNrmVertex s_cubeVertices[] =
    {
        {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f, 0, 0, 1},
        { 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, 0, 0, 1},
        {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0, 0, 1},
        { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f, 0, 0, 1},

        {-1.0f,  1.0f, -1.0f, 0.0f,  1.0f, 0, 0, -1},
        { 1.0f,  1.0f, -1.0f, 1.0f,  1.0f, 0, 0, -1},
        {-1.0f, -1.0f, -1.0f, 0.0f,  0.0f, 0, 0, -1},
        { 1.0f, -1.0f, -1.0f, 1.0f,  0.0f, 0, 0, -1},

        {-1.0f,  1.0f,  1.0f, 0.0f,  1.0f, -1, 0, 0 },
        {-1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1, 0, 0 },
        {-1.0f, -1.0f,  1.0f, 0.0f,  0.0f, -1, 0, 0 },
        {-1.0f, -1.0f, -1.0f, 1.0f,  0.0f, -1, 0, 0 },

        { 1.0f,  1.0f,  1.0f, 0.0f,  1.0f, 1, 0, 0 },
        { 1.0f, -1.0f,  1.0f, 1.0f,  1.0f, 1, 0, 0},
        { 1.0f,  1.0f, -1.0f, 0.0f,  0.0f, 1, 0, 0},
        { 1.0f, -1.0f, -1.0f, 1.0f,  0.0f, 1, 0, 0},

        {-1.0f,  1.0f,  1.0f, 0.0f,  1.0f, 0, 1, 0},
        { 1.0f,  1.0f,  1.0f, 1.0f,  1.0f, 0, 1, 0},
        {-1.0f,  1.0f, -1.0f, 0.0f,  0.0f, 0, 1, 0},
        { 1.0f,  1.0f, -1.0f, 1.0f,  0.0f, 0, 1, 0},

        {-1.0f, -1.0f,  1.0f, 0.0f,  1.0f, 0, -1, 0},
        {-1.0f, -1.0f, -1.0f, 1.0f,  1.0f, 0, -1, 0},
        { 1.0f, -1.0f,  1.0f, 0.0f,  0.0f, 0, -1, 0},
        { 1.0f, -1.0f, -1.0f, 1.0f,  0.0f, 0, -1, 0},
    };

    static const uint32_t s_cubeIndices[] =
    {
         0,  1,  2, // 0
         1,  3,  2,

         4,  6,  5, // 2
         5,  6,  7,

         8, 10,  9, // 4
         9, 10, 11,

        12, 14, 13, // 6
        14, 15, 13,

        16, 18, 17, // 8
        18, 19, 17,

        20, 22, 21, // 10
        21, 22, 23,
    };

    vertices.reserve(pts.size() * 24);
    indices.reserve(pts.size() * 36);
    for (const Vec3f& pt : pts)
    {
        uint32_t offset = vertices.size();
        for (uint32_t i : s_cubeIndices)
        {
            indices.push_back(i + offset);
        }
        for (const PosTexcoordNrmVertex& cubevtx : s_cubeVertices)
        {
            PosTexcoordNrmVertex vtx = cubevtx;
            vtx.m_x = vtx.m_x * cubeSize + pt[0];
            vtx.m_y = vtx.m_y * cubeSize + pt[1];
            vtx.m_z = vtx.m_z * cubeSize + pt[2];
            vertices.push_back(vtx);
        }
    }

    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosTexcoordNrmVertex))
        , PosTexcoordNrmVertex::ms_layout
    );

    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices.data(), indices.size() * sizeof(uint32_t)),
        BGFX_BUFFER_INDEX32
    );    
}



void VoxCube::Create(const std::vector<Vec4f>& pts)
{
    VoxelVertex::init();
    verticesSize = pts.size();
    pvertices = new VoxelVertex[verticesSize];
    VoxelVertex* pdata = pvertices;
    for (const Vec4f& pt : pts)
    {
        VoxelVertex vtx = { pt[0], pt[1], pt[2], pt[3] };
        *pdata++ = vtx;
    }

    memsize = verticesSize * sizeof(VoxelVertex);
}


void VoxCube::Use()
{
    if (!bgfx::isValid(vbh))
    {
        //vbh = bgfx::createynamicVertexBuffer(verticesSize, VoxelVertex::ms_layout, BGFX_BUFFER_COMPUTE_WRITE);
        vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(pvertices, verticesSize * sizeof(VoxelVertex), VoxCube::ReleaseFn)
            , VoxelVertex::ms_layout
        );

    }
}

VoxCube::~VoxCube()
{
}

void VoxCube::ReleaseFn(void* ptr, void* pThis)
{
    delete[]ptr;

}
