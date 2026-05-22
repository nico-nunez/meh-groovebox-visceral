#pragma once

namespace app {
struct AppContext;
struct EditorRuntime;
} // namespace app

namespace app::display {

void drawEditorSelector(EditorRuntime& editor);
void drawFileControls(EditorRuntime& editor);
void drawEditorDisplayView(AppContext& app);

} // namespace app::display
