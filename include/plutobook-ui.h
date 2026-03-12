#pragma once

#include "plutobook.hpp"

namespace plutobook {
    enum class MouseButton : uint8_t { Left, Right, Middle };
    enum class InputAction { Release, Press, Repeat };

    class UIContext;
    PLUTOBOOK_API UIContext* createUIContext();
    PLUTOBOOK_API void destroyUIContext(UIContext* ui);
    PLUTOBOOK_API void setSize(UIContext* ui, float width, float height);
    PLUTOBOOK_API void getSize(UIContext* ui, float& width, float& height);
    PLUTOBOOK_API bool loadUrl(UIContext* ui, std::string_view url);
    PLUTOBOOK_API bool update(UIContext* ui);
    PLUTOBOOK_API void output(UIContext* ui, FILE* fd);
    PLUTOBOOK_API void processMouseIn(UIContext* ui);
    PLUTOBOOK_API void processMouseOut(UIContext* ui);
    PLUTOBOOK_API void processMouseMove(UIContext* ui, float x, float y);
    PLUTOBOOK_API void processMouseDown(UIContext* ui, MouseButton button);
    PLUTOBOOK_API void processMouseUp(UIContext* ui, MouseButton button);
    PLUTOBOOK_API void renderDocument(const UIContext* ui,
                                      GraphicsContext& context, float x,
                                      float y, float width, float height);
} // namespace plutobook