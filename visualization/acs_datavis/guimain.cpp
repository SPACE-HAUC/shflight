// Dear ImGui: standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_glfw_opengl2/ folder**
// See imgui_impl_glfw.cpp for details.

#include "imgui/imgui.h"
#include "backend/imgui_impl_glfw.h"
#include "backend/imgui_impl_opengl2.h"
#include "implot/implot.h"
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

struct ScrollBuf
{
    int max_sz;
    int ofst;
    ImVector<ImVec2> data;
    ScrollBuf(int max_size = 600) // 600 seconds by default
    {
        max_sz = max_size;
        ofst = 0;
        data.reserve(max_sz);
    }
    void AddPoint(float x, float y)
    {
        if (data.size() < max_sz)
            data.push_back(ImVec2(x, y));
        else
        {
            data[ofst] = ImVec2(x, y);
            ofst = (ofst + 1) % max_sz;
        }
    }
    void Erase()
    {
        if (data.size() > 0)
        {
            data.shrink(0);
            ofst = 0;
        }
    }

    float Max()
    {
        float max = data[0].y;
        for (int i = 0; i < data.size(); i++)
            if (data[i].y > max)
                max = data[i].y;
        return max;
    }

    float Min()
    {
        float max = data[0].y;
        for (int i = 0; i < data.size(); i++)
            if (data[i].y < max)
                max = data[i].y;
        return max;
    }
};

int fsgn(float f)
{
    if (f < 0)
        return -1;
    return 1;
}

float find_max(float *arr, ssize_t len)
{
    float val = arr[0];
    while(len--)
        if (val < arr[len])
            val = arr[len];
    return val;
}

float find_min(float *arr, ssize_t len)
{
    float val = arr[0];
    while(len--)
        if (val > arr[len])
            val = arr[len];
    return val;
}
