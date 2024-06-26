#pragma once

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include "Events.h"
#include "UI.h"
#include "System.h"
#include "Math.h"

namespace ImGui::qmdlr
{
    inline void MenuItemWithEvent(const char *label, EventType ev, EventContext context, bool selected)
    {
        static std::string sh;
        sh.clear();

        if (auto shortcut = settings().shortcuts.find(ev); shortcut.scancode != SDL_SCANCODE_UNKNOWN)
            std::format_to(std::back_inserter(sh), "{}", shortcut);

        if (ImGui::MenuItem(label, sh.empty() ? nullptr : sh.c_str(), selected))
            events().Push(ev);
    }

    inline void MenuItemWithEvent(const char *label, EventType ev, EventContext context = EventContext::Any)
    {
        static std::string sh;
        sh.clear();

        if (auto shortcut = settings().shortcuts.find(ev); shortcut.scancode != SDL_SCANCODE_UNKNOWN)
            std::format_to(std::back_inserter(sh), "{}", shortcut);

        if (ImGui::MenuItem(label, sh.empty() ? nullptr : sh.c_str()))
            events().Push(ev, context);
    }

    inline bool CheckBoxButton(const char *id, bool pressed, const ImVec2 &size = ImVec2(0, 0))
    {
        if (pressed)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    
        bool clicked = ImGui::Button(id, size);

        if (pressed)
            ImGui::PopStyleColor();

        return clicked;
    }

    inline void DrawToolBoxButton(const char *id, EditorTool tool, EditorTool selected, EventType event, EventContext context = EventContext::Any)
    {
        if (CheckBoxButton(id, selected == tool, ImVec2(-1, 0)))
            events().Push(event, context);
    }

    template<typename T>
    inline void HandleViewport(T &renderer)
    {
        ImVec2 topLeft = ImGui::GetCursorScreenPos();

        {
            ImVec2 available = ImGui::GetContentRegionAvail();
            ImVec2 bottomRight { topLeft.x + available.x, topLeft.y + available.y };

            renderer.resize(available.x, available.y);

            ImGui::GetWindowDrawList()->AddRectFilled(topLeft, { topLeft.x + (float) renderer.width(), topLeft.y + (float) renderer.height() }, IM_COL32(63, 63, 63, 255));
        }
        
        ImVec2 cursor = ImGui::GetCursorPos();
        renderer.draw();
        ImGui::SetCursorPos(cursor);
        ImGui::InvisibleButton("renderer", { (float) renderer.width(), (float) renderer.height() });
        ImGui::SetCursorPos(cursor);

        auto &io = ImGui::GetIO();

        ImVec2 bottomRight { topLeft.x + renderer.width(), topLeft.y + renderer.height() };
        bool isWithinViewport = ImGui::IsItemHovered() &&
                                io.MousePos.x >= topLeft.x && io.MousePos.y >= topLeft.y &&
                                io.MousePos.x < bottomRight.x && io.MousePos.y < bottomRight.y;
        ImVec2 localPos(io.MousePos.x - topLeft.x, io.MousePos.y - topLeft.y);
        glm::ivec2 relativePos;
        static glm::ivec2 mouseRestorePosition;

        if (renderer.editorMouseToViewport())
        {
            // check if we need to modify down clicks
            ImGuiMouseButton pressed = 0;

            for (int i = 0; i < ImGuiMouseButton_COUNT; i++)
                if (ImGui::IsMouseDown(i))
                    pressed |= 1 << i;

            if (pressed != renderer.editorMouseToViewport())
            {
                renderer.mouseReleaseEvent(localPos);

                if (pressed)
                {
                    SDL_SetRelativeMouseMode((SDL_bool) renderer.mousePressEvent(pressed, localPos));
                    SDL_GetRelativeMouseState(nullptr, nullptr);
                }
                else if (SDL_GetRelativeMouseMode())
                {
                    SDL_SetRelativeMouseMode((SDL_bool) false);
                    SDL_WarpMouseGlobal(mouseRestorePosition.x, mouseRestorePosition.y);
                }

                renderer.editorMouseToViewport() = pressed;
            }

            if (renderer.editorMouseToViewport())
            {
                if (SDL_GetRelativeMouseMode())
                {
                    SDL_GetRelativeMouseState(&relativePos.x, &relativePos.y);
                    renderer.mouseMoveEvent({ (float) relativePos.x, (float) relativePos.y });
                }
                else
                    renderer.mouseMoveEvent(localPos);
            }

            sys().WantsRedraw();
        }
        else if (io.WantCaptureMouse && isWithinViewport)
        {
            ImGuiMouseButton clicked = 0;

            for (int i = 0; i < ImGuiMouseButton_COUNT; i++)
                if (ImGui::IsMouseClicked(i))
                    clicked |= 1 << i;

            if (clicked)
            {
                SDL_GetGlobalMouseState(&mouseRestorePosition.x, &mouseRestorePosition.y);
                renderer.editorMouseToViewport() = clicked;
                SDL_SetRelativeMouseMode((SDL_bool) renderer.mousePressEvent(clicked, localPos));
                SDL_GetRelativeMouseState(nullptr, nullptr);
            }
            else
            {
                if (SDL_GetRelativeMouseMode())
                {
                    SDL_GetRelativeMouseState(&relativePos.x, &relativePos.y);
                    renderer.mouseMoveEvent({ (float) relativePos.x, (float) relativePos.y });
                }
                else
                    renderer.mouseMoveEvent(localPos);
            }

            sys().WantsRedraw();
        }

        if (isWithinViewport && io.MouseWheel)
            renderer.mouseWheelEvent(io.MouseWheel);

        renderer.paint();
    }

    inline void BufferedInputText(const char *label, const char *text, std::function<void(std::string &)> finished)
    {
        static char emptyString[1] = {};

        if (!text)
            text = emptyString;

        static std::string awaitingBuffer;
        static std::optional<decltype(finished)> awaitingCallback;
        static ImGuiID awaitingTextId;

        ImGuiID nextTextId = ImGui::GetID(label);

        if (awaitingCallback && awaitingTextId == nextTextId)
            ImGui::InputText(label, &awaitingBuffer);
        else
            ImGui::InputText(label, (char *) text, 0);

        if (IsItemActive())
        {
            if (awaitingCallback)
            {
                if (awaitingTextId != nextTextId)
                {
                    awaitingCallback.value()(awaitingBuffer);
                    awaitingCallback.reset();
                }
            }

            if (!awaitingCallback)
            {
                awaitingBuffer = text;
                awaitingTextId = nextTextId;
                awaitingCallback = finished;

                if (ImGuiInputTextState* state = ImGui::GetInputTextState(nextTextId)) // id may be ImGui::GetItemID() is last item
                    state->ReloadUserBufAndSelectAll();
            }
        }
        else if (awaitingCallback)
        {
            awaitingCallback.value()(awaitingBuffer);
            awaitingCallback.reset();
        }
    }
}