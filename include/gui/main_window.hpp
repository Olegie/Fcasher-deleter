#ifndef FCASHER_GUI_MAIN_WINDOW_HPP
#define FCASHER_GUI_MAIN_WINDOW_HPP

#include <string>
#include <vector>

#include <windows.h>

#include "gui/session_controller.hpp"

namespace fcasher::gui {

class MainWindow {
public:
    MainWindow();
    int run(HINSTANCE instance, int showCommand);

private:
    bool registerClass(HINSTANCE instance);
    bool create(HINSTANCE instance);
    void createControls();
    void applyFonts() const;
    void layout(int clientWidth, int clientHeight) const;
    void refreshControlStates() const;
    void updateStatus(const std::string& text) const;
    std::vector<std::string> buildArguments() const;
    bool isCommandClean() const;
    bool isJsonExportAllowed() const;
    void executeCurrentCommand();
    void exportJson();
    void saveReport();
    void clearOutput();
    std::string selectedCommandToken() const;
    std::string selectedPresetToken() const;
    std::string selectedSortToken() const;
    std::string readEditText(HWND control) const;
    std::string formatOutputForViewer(const std::string& text) const;
    void setOutputText(const std::string& text);
    void setStructuredResult(const StructuredResult& structured);
    void clearStructuredResult();
    void configureListViews() const;
    void populateSummaryList(const StructuredResult& structured);
    void populateFilesList(const StructuredResult& structured);
    void populateSkippedList(const StructuredResult& structured);
    void setActiveResultPage(int pageIndex) const;
    void openSelectedPath(HWND listView, const std::vector<std::string>& rowPaths) const;
    void openPathInExplorer(const std::string& path) const;
    std::string promptSavePath(const char* filter, const char* defaultExtension) const;
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE instance_{nullptr};
    HWND window_{nullptr};
    HFONT uiFont_{nullptr};
    HFONT outputFont_{nullptr};

    HWND commandCombo_{nullptr};
    HWND presetCombo_{nullptr};
    HWND sortCombo_{nullptr};
    HWND allCheck_{nullptr};
    HWND tempCheck_{nullptr};
    HWND logsCheck_{nullptr};
    HWND browserCheck_{nullptr};
    HWND thumbnailsCheck_{nullptr};
    HWND shaderCheck_{nullptr};
    HWND dumpsCheck_{nullptr};
    HWND recentCheck_{nullptr};
    HWND recycleCheck_{nullptr};
    HWND dryRunCheck_{nullptr};
    HWND verboseCheck_{nullptr};
    HWND minSizeEdit_{nullptr};
    HWND maxSizeEdit_{nullptr};
    HWND withinDaysEdit_{nullptr};
    HWND olderDaysEdit_{nullptr};
    HWND limitEdit_{nullptr};
    HWND runButton_{nullptr};
    HWND saveButton_{nullptr};
    HWND jsonButton_{nullptr};
    HWND clearButton_{nullptr};
    HWND statusLabel_{nullptr};
    HWND summaryList_{nullptr};
    HWND resultTabs_{nullptr};
    HWND filesList_{nullptr};
    HWND skippedList_{nullptr};
    HWND detailsEdit_{nullptr};

    SessionController controller_;
    std::string currentOutput_;
    std::vector<std::string> fileRowPaths_;
    std::vector<std::string> skippedRowPaths_;
};

}  // namespace fcasher::gui

#endif
