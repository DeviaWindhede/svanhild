#pragma once

namespace RenderConstants
{
    static constexpr unsigned int FrameCount = 2;

    inline UINT GetFrameGroupCount(size_t aSize, size_t aThreadCount)
    {
        return static_cast<UINT>((aSize + aThreadCount - 1) / aThreadCount);
    }
}

