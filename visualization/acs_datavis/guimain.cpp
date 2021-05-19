// Dear ImGui: standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_glfw_opengl2/ folder**
// See imgui_impl_glfw.cpp for details.

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

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
    ImVector<ImVec4> data;
    ScrollBuf(int max_size = 3000) // 600 seconds by default
    {
        max_sz = max_size;
        ofst = 0;
        data.reserve(max_sz);
    }
    void AddPoint(float t, float x, float y, float z)
    {
        if (data.size() < max_sz)
            data.push_back(ImVec4(x, y, z, t));
        else
        {
            data[ofst] = ImVec4(x, y, z, t);
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

    float MaxX()
    {
        float max = data[0].x;
        for (int i = 0; i < data.size(); i++)
            if (data[i].x > max)
                max = data[i].x;
        return max;
    }

    float MinX()
    {
        float max = data[0].x;
        for (int i = 0; i < data.size(); i++)
            if (data[i].x < max)
                max = data[i].x;
        return max;
    }

    float MaxY()
    {
        float max = data[0].y;
        for (int i = 0; i < data.size(); i++)
            if (data[i].y > max)
                max = data[i].y;
        return max;
    }

    float MinY()
    {
        float max = data[0].y;
        for (int i = 0; i < data.size(); i++)
            if (data[i].y < max)
                max = data[i].y;
        return max;
    }

    float MaxZ()
    {
        float max = data[0].z;
        for (int i = 0; i < data.size(); i++)
            if (data[i].z > max)
                max = data[i].z;
        return max;
    }

    float MinZ()
    {
        float max = data[0].z;
        for (int i = 0; i < data.size(); i++)
            if (data[i].z < max)
                max = data[i].z;
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
    while (len--)
        if (val < arr[len])
            val = arr[len];
    return val;
}

float find_min(float *arr, ssize_t len)
{
    float val = arr[0];
    while (len--)
        if (val > arr[len])
            val = arr[len];
    return val;
}

typedef struct
{
    char start[6];
    /**
     * @brief Current system state
     * 
     */
    uint8_t mode; // ACS Mode
    /**
     * @brief Current ACS step number
     * 
     */
    uint64_t step; // ACS Step
    /**
     * @brief Current time from UTC (usec)
     * 
     */
    uint64_t tnow; // Time now
    /**
     * @brief ACS start time from UTC (usec)
     * 
     */
    uint64_t tstart; // Time start
    /**
     * @brief Measured magnetic field
     * 
     */
    float Bx, By, Bz;

    float Btx, Bty, Btz;

    float Wx, Wy, Wz;

    float Sx, Sy, Sz;
    char end[4];
} datavis_p;

static datavis_p data[1];

volatile sig_atomic_t done = 0;

void sighandler(int sig)
{
    done = 1;
}

static float t;
struct ScrollBuf B[1], Bt[1], W[1], S[1], St[1];

pthread_mutex_t rcv_data[1], plot_data[1];
volatile bool conn_rdy = false;
static char rcv_buf[1024];
static long long counter = 0;
void *rcv_thr(void *sock)
{
    memset(rcv_buf, 0x0, sizeof(rcv_buf));
    memset(data, 0x0, sizeof(datavis_p));
    while (!done)
    {
        if (conn_rdy)
        {
            char msg_sz = 0;
            int sz = read(*(int *)sock, &msg_sz, 1);
            if (sz > 0)
                sz = recv(*(int *)sock, rcv_buf, msg_sz, MSG_WAITALL);
            fprintf(stderr, "%s: Received %d bytes, %s\n", __func__, sz, rcv_buf);
            if (sz == sizeof(datavis_p) && (strncmp(rcv_buf, "FBEGIN", 6) == 0))
            {
                counter++;
                pthread_mutex_lock(rcv_data);
                memcpy(data, rcv_buf, sizeof(datavis_p));
                pthread_mutex_unlock(rcv_data);
                pthread_mutex_lock(plot_data);
                t = (data->tnow - data->tstart) / 1000000.0; // seconds
                B->AddPoint(t, data->Bx, data->By, data->Bz);
                Bt->AddPoint(t, data->Btx, data->Bty, data->Btz);
                W->AddPoint(t, data->Wx, data->Wy, data->Wz);
                S->AddPoint(t, data->Sx, data->Sy, data->Sz);
                float sz = data->Sz;
                float sy = data->Sy;
                float sx = data->Sx;
                float snorm = (sx * sx + sy * sy + sz * sz);
                float Sz = snorm > 0 ? (sz > 0 ? 1 : -1) : 0;
                float Sx = 180 * atan2(sx, sz) / M_PI;
                float Sy = 180 * atan2(sy, sz) / M_PI;
                St->AddPoint(t, Sx, Sy, Sz);
                pthread_mutex_unlock(plot_data);
            }
        }
        usleep(1000 * 1000 / 60); // receive only at 30 Hz
    }
    return NULL;
}

int main(int, char **)
{
    // setup signal handler
    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN); // so that client does not die when server does
    // set up client socket etc
    int sock = -1, valread;
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;

    // we will have to add a port

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Client Example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    ImFont *font = io.Fonts->AddFontFromFileTTF("./imgui/font/Roboto-Medium.ttf", 16.0f);
    if (font == NULL)
        io.Fonts->AddFontDefault();
    //IM_ASSERT(font != NULL);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool show_readout_win = true;

    pthread_t rcv_thread;
    int rc = pthread_create(&rcv_thread, NULL, rcv_thr, (void *)&sock);
    if (rc < 0)
    {
        fprintf(stderr, "main: Could not create receiver thread! Exiting...\n");
        goto end;
    }
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static float hist = 30.0f;
        static float vsize = 250;
        static float sunsize = 250;

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static int port = 12376;
            static char ipaddr[16] = "127.0.0.1";
            auto flag = ImGuiInputTextFlags_ReadOnly;
            if (!conn_rdy)
                flag = (ImGuiInputTextFlags_)0;

            ImGui::Begin("Connection Manager"); // Create a window called "Hello, world!" and append into it.

            ImGui::InputText("IP Address", ipaddr, sizeof(ipaddr), flag);
            ImGui::InputInt("Port", &port, 0, 0, flag);

            if (!conn_rdy || sock < 0)
            {
                if (ImGui::Button("Connect"))
                {
                    serv_addr.sin_port = htons(port);
                    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        printf("\n Socket creation error \n");
                        return -1;
                    }
                    if (inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr) <= 0)
                    {
                        printf("\nInvalid address/ Address not supported \n");
                    }
                    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                    {
                        printf("\nConnection Failed \n");
                    }
                    else
                        conn_rdy = true;
                }
            }
            else
            {
                if (ImGui::Button("Disconnect"))
                {
                    close(sock);
                    sock = -1;
                    conn_rdy = false;
                }
            }

            
            static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_None;
            ImGui::SliderFloat("History", &hist, 1, 300, "%.1f s");

            if (ImGui::SliderFloat("Vertical Height", &vsize, 100, 300, "%.0f"))
                vsize = floor(vsize);

            if (ImGui::SliderFloat("Sun Map Height", &sunsize, 100, 500, "%.0f"))
                sunsize = floor(sunsize);

            if (ImGui::Button("Clear Data"))
            {
                counter = 0;
                B->Erase();
                Bt->Erase();
                W->Erase();
                S->Erase();
                St->Erase();
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        {
            ImGui::Begin("Mag Sensor Data Window");
            if (conn_rdy && (counter > 10))
            {
                pthread_mutex_lock(plot_data);
                float b_min[] = {B->MinX(), B->MinY(), B->MinZ()};
                float B_min = find_min(b_min, 3);
                float b_max[] = {B->MaxX(), B->MaxY(), B->MaxZ()};
                float B_max = find_max(b_max, 3);
                B_min = fsgn(B_min) * (fabs(B_min) * 1.05);
                B_max = fsgn(B_max) * (fabs(B_max) * 1.05);
                float bt_min[] = {Bt->MinX(), Bt->MinY(), Bt->MinZ()};
                float Bt_min = find_min(bt_min, 3);
                float bt_max[] = {Bt->MaxX(), Bt->MaxY(), Bt->MaxZ()};
                float Bt_max = find_max(bt_max, 3);
                Bt_min = fsgn(Bt_min) * (fabs(Bt_min) * 1.05);
                Bt_max = fsgn(Bt_max) * (fabs(Bt_max) * 1.05);
                float w_min[] = {W->MinX(), W->MinY(), W->MinZ()};
                float W_min = find_min(w_min, 3);
                float w_max[] = {W->MaxX(), W->MaxY(), W->MaxZ()};
                float W_max = find_max(w_max, 3);
                W_min = fsgn(W_min) * (fabs(W_min) * 1.05);
                W_max = fsgn(W_max) * (fabs(W_max) * 1.05);
                ImPlot::SetNextPlotLimitsX(t - hist, t, ImGuiCond_Always);
                ImPlot::SetNextPlotLimitsY(B_min, B_max, ImGuiCond_Always);
                if (ImPlot::BeginPlot("Magnetic Field", "Time (s)", "B (mG)", ImVec2(-1, vsize)))
                {
                    ImPlot::PlotLine("X", &(B->data[0].w), &(B->data[0].x), B->data.size(), B->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Y", &(B->data[0].w), &(B->data[0].y), B->data.size(), B->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Z", &(B->data[0].w), &(B->data[0].z), B->data.size(), B->ofst, 4 * sizeof(float));
                    ImPlot::EndPlot();
                }
                ImPlot::SetNextPlotLimitsX(t - hist, t, ImGuiCond_Always);
                ImPlot::SetNextPlotLimitsY(Bt_min, Bt_max, ImGuiCond_Always);
                if (ImPlot::BeginPlot("Change in Magnetic Field", "Time (s)", "dB/dt (mG/s)", ImVec2(-1, vsize)))
                {
                    ImPlot::PlotLine("X", &(Bt->data[0].w), &(Bt->data[0].x), Bt->data.size(), Bt->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Y", &(Bt->data[0].w), &(Bt->data[0].y), Bt->data.size(), Bt->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Z", &(Bt->data[0].w), &(Bt->data[0].z), Bt->data.size(), Bt->ofst, 4 * sizeof(float));
                    ImPlot::EndPlot();
                }
                ImPlot::SetNextPlotLimitsX(t - hist, t, ImGuiCond_Always);
                ImPlot::SetNextPlotLimitsY(W_min, W_max, ImGuiCond_Always);
                if (ImPlot::BeginPlot("Angular Velocity", "Time (s)", "w (rad/s)", ImVec2(-1, vsize)))
                {
                    ImPlot::PlotLine("X", &(W->data[0].w), &(W->data[0].x), W->data.size(), W->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Y", &(W->data[0].w), &(W->data[0].y), W->data.size(), W->ofst, 4 * sizeof(float));
                    ImPlot::PlotLine("Z", &(W->data[0].w), &(W->data[0].z), W->data.size(), W->ofst, 4 * sizeof(float));
                    ImPlot::EndPlot();
                }
                pthread_mutex_unlock(plot_data);
            }
            ImGui::End();
        }

        {
            ImGui::Begin("Sun Sensor Data Window");
            if (conn_rdy && (counter > 10))
            {
                pthread_mutex_lock(plot_data);
                ImPlot::SetNextPlotLimitsX(-180, 180, ImGuiCond_Always);
                ImPlot::SetNextPlotLimitsY(-180, 180, ImGuiCond_Always);
                char svecbuf[256];
                float scond = St->data[St->ofst].z;
                snprintf(svecbuf, sizeof(svecbuf), "Sun Vector %s", scond == 0 ? "Unavailable" : (scond < 0 ? "-Z" : "+Z"));
                if (ImPlot::BeginPlot(svecbuf, "X", "Y", ImVec2(sunsize, sunsize)))
                {
                    ImPlot::PlotLine("##X", &(St->data[0].x), &(St->data[0].y), St->data.size(), St->ofst, 4 * sizeof(float));
                    ImPlot::EndPlot();
                }
                pthread_mutex_unlock(plot_data);
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
        // you may need to backup/reset/restore current shader using the commented lines below.
        //GLint last_program;
        //glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        //glUseProgram(0);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        //glUseProgram(last_program);

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
end:
    done = 1;
    close(sock);
    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}