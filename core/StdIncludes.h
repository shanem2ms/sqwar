#pragma once
#include <string>
#include <vector>
#include <map>

#include <bgfx/bgfx.h>

#ifdef SAM_COROUTINES
#include <coroutine>

#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/when_all.hpp>

extern cppcoro::static_thread_pool g_threadPool;


namespace cppcoro
{
    template <typename Func>
    task<> dispatch(Func func) {
        co_await g_threadPool.schedule();
        co_await func();
    }
}

namespace co = cppcoro; 
#endif
constexpr float pi = 3.14159265358979323846f;

template <class T> class bgfxh
{
    T t;
public:
    bgfxh() :
        t(BGFX_INVALID_HANDLE)
    {

    }

    operator T() const
    {
        return t;
    }

    T &operator = (const T& rhs)
    {
        free();
        t = rhs;
        return t;
    }

    void free()
    {
        if (bgfx::isValid(t))
        {
            bgfx::destroy(t);
            t = BGFX_INVALID_HANDLE;
        }
    }
    ~bgfxh()
    {
        free();
    }

private:
    bgfxh(const bgfxh& rhs);
};

typedef unsigned char byte;



