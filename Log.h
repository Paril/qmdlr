#pragma once

#include <imgui.h>
#include <format>

struct Log
{
    std::string         scratch;
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    Log()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
        scratch.reserve(1024);
    }

    template<typename ...TArgs>
    void AddLog(std::format_string<TArgs...> format, TArgs&& ...args)
    {
        scratch.clear();
        auto out = std::format_to(std::back_inserter(scratch), format, std::forward<TArgs>(args)...);
        *(out++) = '\n';
        int old_size = Buf.size();
        Buf.append(scratch.data(), scratch.data() + scratch.size());
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char* title, bool* p_open = nullptr);
};

Log &logger();