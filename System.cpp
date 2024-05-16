#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <SDL2/SDL_opengl.h>
#include <nfd.hpp>
#include <thread>

#include "System.h"
#include "UI.h"
#include "Events.h"
#include "Settings.h"
#include "UndoRedo.h"
#include "Log.h"

struct SystemPrivate
{
    SDL_Window      *window;
    SDL_GLContext   gl_context;
    ImGuiContext    *imgui_context;
};

System::System() :
    priv(new SystemPrivate {})
{
}

static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    const char* sourceStr;
    const char* typeStr;
    const char* severityStr;

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        sourceStr = "API";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "WINDOW SYSTEM";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "SHADER COMPILER";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "THIRD PARTY";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "APPLICATION";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "UNKNOWN";
        break;

        default:
        sourceStr = "UNKNOWN";
        break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UDEFINED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;

        case GL_DEBUG_TYPE_OTHER:
        typeStr = "OTHER";
        break;

        case GL_DEBUG_TYPE_MARKER:
        typeStr = "MARKER";
        break;

        default:
        typeStr = "UNKNOWN";
        break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityStr = "NOTIFICATION";
        break;

        default:
        severityStr = "UNKNOWN";
        break;
    }

    logger().AddLog("{}: [{}]({}): {}", std::string(sourceStr), typeStr, severityStr, message);
}

//<a href="https://www.flaticon.com/free-icons/model" title="model icons">Model icons created by SBTS2018 - Flaticon</a>
/* C:\Users\Paril\Downloads\3d-modeling.png (2024-05-25 6:54:22 PM)
   StartOffset(h): 00000000, EndOffset(h): 0000033A, Length(h): 0000033B */
constexpr const uint8_t iconData[827] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7A, 0x7A, 0xF4, 0x00, 0x00, 0x00,
	0x04, 0x73, 0x42, 0x49, 0x54, 0x08, 0x08, 0x08, 0x08, 0x7C, 0x08, 0x64,
	0x88, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x00,
	0xEC, 0x00, 0x00, 0x00, 0xEC, 0x01, 0x79, 0x28, 0x71, 0xBD, 0x00, 0x00,
	0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61,
	0x72, 0x65, 0x00, 0x77, 0x77, 0x77, 0x2E, 0x69, 0x6E, 0x6B, 0x73, 0x63,
	0x61, 0x70, 0x65, 0x2E, 0x6F, 0x72, 0x67, 0x9B, 0xEE, 0x3C, 0x1A, 0x00,
	0x00, 0x02, 0xB8, 0x49, 0x44, 0x41, 0x54, 0x58, 0x85, 0xBD, 0xD7, 0x5D,
	0x88, 0x55, 0x55, 0x14, 0x07, 0xF0, 0xDF, 0x38, 0x13, 0xA5, 0x43, 0x84,
	0x52, 0x57, 0x1D, 0x9F, 0x8A, 0x02, 0x1F, 0x8C, 0xC4, 0x21, 0x48, 0x8D,
	0x32, 0x48, 0x7C, 0x09, 0x91, 0xE8, 0x43, 0x1F, 0x14, 0x11, 0x02, 0x05,
	0xC5, 0x81, 0x21, 0x22, 0x02, 0x19, 0x1F, 0xFC, 0xC0, 0x97, 0x28, 0x7C,
	0x49, 0x4A, 0x6C, 0x24, 0x2A, 0xF2, 0x45, 0x10, 0x21, 0x1A, 0x1A, 0x26,
	0xFC, 0x82, 0x68, 0x14, 0xB5, 0x2C, 0xFC, 0x22, 0x88, 0x1E, 0x7A, 0x53,
	0x9A, 0xC8, 0x4C, 0xC7, 0x87, 0xB5, 0x4F, 0x73, 0xBC, 0x73, 0xEF, 0xB9,
	0xE7, 0x9E, 0x31, 0xFF, 0xB0, 0xB8, 0xF7, 0xEE, 0xB3, 0xF6, 0x5A, 0xFF,
	0xBB, 0xCE, 0x5A, 0x6B, 0xAF, 0x4D, 0x75, 0xCC, 0xC1, 0x2F, 0xF8, 0x19,
	0xB5, 0x29, 0xD8, 0xA9, 0x8C, 0x8D, 0x18, 0x4F, 0xB2, 0xB1, 0xAA, 0x91,
	0x8E, 0x0A, 0x7B, 0xE6, 0x60, 0x37, 0xE6, 0x61, 0x0C, 0xB7, 0x70, 0x13,
	0x37, 0xF0, 0x2E, 0x7E, 0xAF, 0x4A, 0xA6, 0x15, 0x1E, 0xC0, 0x56, 0x9C,
	0xC3, 0xBA, 0xB4, 0xB6, 0x21, 0x09, 0xBC, 0x88, 0x13, 0x18, 0xC0, 0x43,
	0x65, 0x8D, 0x4E, 0x2B, 0xA9, 0xF7, 0x32, 0x8E, 0x61, 0x26, 0x9E, 0xC5,
	0x60, 0x03, 0x9D, 0x11, 0x3C, 0x8F, 0x2B, 0x38, 0x9E, 0x48, 0x56, 0x89,
	0xF0, 0x5D, 0x78, 0x12, 0x5F, 0xE2, 0x53, 0x11, 0xFA, 0x7A, 0xE4, 0x23,
	0x90, 0xC7, 0x4C, 0x7C, 0x80, 0x6F, 0xF0, 0x4C, 0x15, 0xC7, 0xDD, 0x22,
	0x94, 0x23, 0x78, 0xAE, 0x40, 0xAF, 0x19, 0x81, 0x0C, 0xF3, 0x71, 0x54,
	0x44, 0xAC, 0x65, 0xA5, 0xAC, 0xC2, 0x21, 0xBC, 0x83, 0x93, 0x78, 0x53,
	0xEB, 0x10, 0xB6, 0x22, 0x90, 0xE1, 0x35, 0x9C, 0xC6, 0x36, 0xEC, 0x17,
	0xAF, 0x74, 0x12, 0xAE, 0x8A, 0x92, 0x3A, 0x83, 0xE9, 0x25, 0x8C, 0x42,
	0x7F, 0x92, 0x32, 0x98, 0x8E, 0x1F, 0x92, 0x8F, 0x4B, 0x8D, 0x14, 0xDE,
	0xC3, 0xDF, 0x18, 0x16, 0x21, 0x9B, 0x5B, 0x60, 0xAC, 0x27, 0xE9, 0xDC,
	0x4E, 0x32, 0x98, 0xD6, 0x9A, 0xE1, 0x61, 0x51, 0xBA, 0x3F, 0xE2, 0x5F,
	0xEC, 0x6C, 0xA6, 0x78, 0x2C, 0x7D, 0xBE, 0x24, 0x5E, 0xC3, 0x00, 0x1E,
	0xCC, 0x3D, 0xCF, 0x4A, 0xF1, 0x9A, 0x89, 0x26, 0x94, 0xC9, 0x98, 0xC9,
	0x25, 0xD8, 0x81, 0xD7, 0x31, 0x9A, 0xF6, 0x75, 0xE6, 0x7C, 0x14, 0x12,
	0x80, 0xAE, 0xB4, 0xE9, 0x14, 0x56, 0x60, 0xA5, 0x08, 0x5D, 0xBD, 0xE3,
	0x7A, 0xB9, 0x94, 0x74, 0x17, 0xE3, 0x3B, 0xEC, 0x4A, 0x11, 0x68, 0xE4,
	0xA3, 0x90, 0x40, 0x86, 0x1A, 0x3E, 0xC7, 0x1A, 0x5C, 0x2E, 0x41, 0xE0,
	0x37, 0xAC, 0x17, 0xE5, 0xFB, 0x78, 0x2B, 0x1F, 0x65, 0x1A, 0xD1, 0x1F,
	0x22, 0x79, 0xFA, 0x45, 0xC6, 0xF7, 0xE1, 0xCF, 0x06, 0x7A, 0xFF, 0xE0,
	0x43, 0xBC, 0x8A, 0xD5, 0xF8, 0x5E, 0x24, 0x76, 0x21, 0xCA, 0x76, 0x42,
	0xE8, 0x15, 0x09, 0xDA, 0x2B, 0xDA, 0xEE, 0xC1, 0xDC, 0xB3, 0x23, 0x58,
	0x9A, 0xBE, 0x9F, 0x10, 0xAF, 0xAC, 0x14, 0xBA, 0xDA, 0x20, 0x40, 0x24,
	0xD5, 0x5A, 0xF1, 0x8E, 0x07, 0xF0, 0x59, 0x5A, 0x7B, 0x0A, 0x5F, 0x63,
	0x56, 0x9B, 0xF6, 0xDA, 0x26, 0x90, 0xE1, 0x11, 0xBC, 0x8F, 0xBD, 0xE9,
	0xF7, 0xE6, 0x8A, 0x76, 0x2A, 0x13, 0xC8, 0x30, 0x63, 0x8A, 0xFB, 0xDB,
	0xCA, 0x81, 0xFF, 0x05, 0x65, 0x09, 0x5C, 0xAF, 0x60, 0xBB, 0xD4, 0x9E,
	0xB2, 0x04, 0xF6, 0x61, 0x39, 0x2E, 0x94, 0xD0, 0xBD, 0x8C, 0x37, 0xD2,
	0x9E, 0x7B, 0x46, 0x00, 0x86, 0xC4, 0xD9, 0xDE, 0xA7, 0xF1, 0xBF, 0xFB,
	0x0B, 0xDB, 0xB1, 0x00, 0x5F, 0xB5, 0x61, 0xF7, 0x3F, 0xEC, 0x11, 0x07,
	0x45, 0x99, 0xE3, 0x35, 0x3B, 0x8C, 0x3E, 0x4E, 0xD2, 0xEA, 0x30, 0xCA,
	0xB0, 0x45, 0xCC, 0x90, 0x7B, 0x1A, 0x3D, 0xCC, 0xFA, 0xFC, 0xA8, 0xF2,
	0xD9, 0xBD, 0x3E, 0x49, 0x19, 0x74, 0x8B, 0x99, 0x60, 0x1C, 0x17, 0xB3,
	0xC5, 0xCE, 0x9C, 0xC2, 0x59, 0xD1, 0x4E, 0x0F, 0x8B, 0xE3, 0xF2, 0x06,
	0x7E, 0x6A, 0x61, 0x74, 0x91, 0x68, 0x44, 0xA7, 0x0B, 0x74, 0x3A, 0x44,
	0x6B, 0xFE, 0x04, 0x5F, 0x88, 0xF6, 0xBC, 0x03, 0xBF, 0x16, 0x19, 0xCE,
	0x8E, 0xDD, 0x21, 0x53, 0x1B, 0xC9, 0x16, 0xE2, 0x5B, 0x7C, 0x84, 0x47,
	0x8B, 0x1C, 0x36, 0x43, 0x0F, 0x0E, 0x88, 0xA1, 0x74, 0x76, 0x1B, 0x04,
	0x66, 0x99, 0x18, 0x4A, 0x9F, 0xAE, 0xE2, 0xB8, 0x1E, 0xF9, 0x99, 0x3F,
	0x3F, 0xA0, 0xD4, 0x13, 0x98, 0x26, 0xC6, 0xF1, 0x51, 0x13, 0x77, 0x87,
	0x42, 0x94, 0x2D, 0xC3, 0x11, 0xBC, 0x20, 0x6E, 0x3D, 0xC3, 0x78, 0xA5,
	0x81, 0xCE, 0x32, 0x71, 0x1F, 0x78, 0x02, 0x4B, 0x34, 0xBE, 0x3B, 0x4C,
	0x42, 0x95, 0x8B, 0x43, 0x4D, 0x4C, 0x39, 0x35, 0x51, 0xB6, 0xE3, 0xE9,
	0xF3, 0x16, 0xDE, 0x16, 0x03, 0xC9, 0x7D, 0xC1, 0x5B, 0x26, 0xA6, 0xA0,
	0x4D, 0xF7, 0xCB, 0x69, 0x1E, 0x8F, 0xE1, 0xBC, 0xB8, 0x2B, 0x56, 0xCA,
	0x70, 0xB8, 0x03, 0xAC, 0xB4, 0xA4, 0x69, 0xF8, 0x2C, 0x26, 0xF7, 0x00,
	0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

std::optional<const char *> System::Init()
{
    ui().LoadThemes();

    settings().Load();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        return SDL_GetError();

    const char* glsl_version = "#version 330 core";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    priv->window = SDL_CreateWindow("QMDLR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 800, window_flags);

    {
        auto image = images().Load(std::span<const uint8_t, std::size(iconData)>(iconData));
        SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(image.rgba(), image.width, image.height, 32, image.width * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        SDL_SetWindowIcon(priv->window, surf);
        SDL_FreeSurface(surf);
    }

    if (!priv->window)
        return SDL_GetError();

    priv->gl_context = SDL_GL_CreateContext(priv->window);
    SDL_GL_MakeCurrent(priv->window, priv->gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    if (!gladLoadGL())
    {
        Shutdown();
        return "Unable to load OpenGL";
    }
    
    if (GLAD_GL_KHR_debug)
    {
        glDebugMessageCallback(MessageCallback, nullptr);

        if (settings().openGLDebug)
            glEnable(GL_DEBUG_OUTPUT);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    priv->imgui_context = ImGui::CreateContext();

    if (!priv->imgui_context)
        return "Unable to create ImGui context.";

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    ui().Init();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(priv->window, priv->gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup NFD
    NFD::Init();

    return std::nullopt;
}

constexpr bool IsModifier(SDL_Scancode code)
{
    return code == SDL_SCANCODE_LSHIFT || code == SDL_SCANCODE_RSHIFT ||
        code == SDL_SCANCODE_LALT || code == SDL_SCANCODE_RALT ||
        code == SDL_SCANCODE_LCTRL || code == SDL_SCANCODE_RCTRL;
}

bool System::Run()
{
    bool done = false;
    static bool show_demo_window = false;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io = ImGui::GetIO();

    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done = true;
        else if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(priv->window))
            done = true;
        else if (event.type == SDL_KEYDOWN &&
            event.key.keysym.scancode == SDL_SCANCODE_F1 &&
            !event.key.repeat)
            show_demo_window = !show_demo_window;

        if (ui().eventContext != EventContext::Skip)
        {
            if (io.WantCaptureKeyboard && !io.WantTextInput)
            {
                if (event.type == SDL_KEYDOWN)
                {
                    KeyShortcut s { event.key.keysym.scancode, io.KeyCtrl, io.KeyShift, io.KeyAlt };

                    if (auto keyEvent = settings().shortcuts.find(s); keyEvent != EventType::Last)
                    {
                        events().Push(keyEvent, ui().eventContext, event.key.repeat);
                    }
                }
            }
        }
        else if (ui().shortcutWaiting != EventType::Last && !event.key.repeat)
        {
            bool modifier = IsModifier(event.key.keysym.scancode);
            bool reassign = false;
            EventType assignType = EventType::Last;

            if (event.type == SDL_KEYDOWN)
                reassign = true;
            else if (event.type == SDL_KEYUP)
            {
                reassign = true;

                if (!modifier)
                {
                    assignType = ui().shortcutWaiting;
                    ui().shortcutWaiting = EventType::Last;
                }
            }

            if (reassign)
            {
                ui().shortcutData = { modifier ? ui().shortcutData.scancode : event.key.keysym.scancode, io.KeyCtrl, io.KeyShift, io.KeyAlt };

                bool isCtrl = event.key.keysym.scancode == SDL_SCANCODE_LCTRL || event.key.keysym.scancode == SDL_SCANCODE_RCTRL;
                bool isShift = event.key.keysym.scancode == SDL_SCANCODE_LSHIFT || event.key.keysym.scancode == SDL_SCANCODE_RSHIFT;
                bool isAlt = event.key.keysym.scancode == SDL_SCANCODE_LALT || event.key.keysym.scancode == SDL_SCANCODE_RALT;
                
                if (event.type == SDL_KEYUP)
                {
                    ui().shortcutData.ctrl = ui().shortcutData.ctrl && !isCtrl;
                    ui().shortcutData.shift = ui().shortcutData.shift && !isShift;
                    ui().shortcutData.alt = ui().shortcutData.alt && !isAlt;
                }
                else
                {
                    ui().shortcutData.ctrl = ui().shortcutData.ctrl || isCtrl;
                    ui().shortcutData.shift = ui().shortcutData.shift || isShift;
                    ui().shortcutData.alt = ui().shortcutData.alt || isAlt;
                }

                if (ui().shortcutWaiting == EventType::Last)
                    settings().shortcuts.insert(assignType, ui().shortcutData);
            }
        }
    }

    // Handle events
    events().Dispatch();

    undo().RunDeferred();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (!_wantRedraw)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds((long long) ((1.0f / 60.0f) * 1000)));
    }
    else
    {
        _wantRedraw--;
    }

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ui().Draw();

    // Rendering
    ImGui::Render();
    
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(priv->window);

    return done;
}

void System::Shutdown()
{
    NFD::Quit();

    // only save settings if we had a good run
    if (priv->imgui_context &&
        priv->gl_context &&
        priv->window)
        settings().Save();

    if (priv->imgui_context)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(priv->imgui_context);
    }

    if (priv->gl_context)
        SDL_GL_DeleteContext(priv->gl_context);
    if (priv->window)
        SDL_DestroyWindow(priv->window);
    SDL_Quit();

    priv.reset();
}

System &sys()
{
    static System system;
    return system;
}