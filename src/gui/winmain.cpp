#include "gui/main_window.hpp"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    fcasher::gui::MainWindow window;
    return window.run(instance, showCommand);
}
