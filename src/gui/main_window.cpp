#include "gui/main_window.hpp"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace fcasher::gui {
namespace {

constexpr int kWindowWidth = 980;
constexpr int kWindowHeight = 760;

enum ControlId {
    IDC_COMMAND = 1001,
    IDC_PRESET,
    IDC_SORT,
    IDC_ALL,
    IDC_TEMP,
    IDC_LOGS,
    IDC_BROWSER,
    IDC_THUMBNAILS,
    IDC_SHADER,
    IDC_DUMPS,
    IDC_RECENT,
    IDC_RECYCLE,
    IDC_DRYRUN,
    IDC_VERBOSE,
    IDC_MIN_SIZE,
    IDC_MAX_SIZE,
    IDC_WITHIN_DAYS,
    IDC_OLDER_DAYS,
    IDC_LIMIT,
    IDC_RUN,
    IDC_SAVE,
    IDC_JSON,
    IDC_CLEAR,
    IDC_SUMMARY,
    IDC_RESULTS_TAB,
    IDC_FILES_LIST,
    IDC_SKIPPED_LIST,
    IDC_DETAILS_EDIT,
    IDC_STATUS
};

struct ComboEntry {
    const char* label;
    const char* token;
};

constexpr ComboEntry kCommandEntries[] = {
    {"Preview", "preview"},
    {"Analyze", "analyze"},
    {"Scan", "scan"},
    {"Clean", "clean"},
    {"Report", "report"},
    {"Categories", "categories"}
};

constexpr ComboEntry kPresetEntries[] = {
    {"(none)", ""},
    {"Quick Sweep", "quick-sweep"},
    {"Browser Focus", "browser-focus"},
    {"Graphics Stack", "graphics-stack"},
    {"Diagnostics", "diagnostics"},
    {"Safe All", "safe-all"}
};

constexpr ComboEntry kSortEntries[] = {
    {"Native", "native"},
    {"Largest First", "size"},
    {"Smallest First", "size-asc"},
    {"Newest First", "newest"},
    {"Oldest First", "oldest"},
    {"Path", "path"},
    {"Category", "category"}
};

void addComboItems(HWND combo, const ComboEntry* entries, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entries[index].label));
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);
}

bool isChecked(HWND control) {
    return SendMessageA(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::string comboToken(HWND combo, const ComboEntry* entries, std::size_t count) {
    const LRESULT selection = SendMessageA(combo, CB_GETCURSEL, 0, 0);
    if (selection < 0 || static_cast<std::size_t>(selection) >= count) {
        return entries[0].token;
    }
    return entries[selection].token;
}

void setChecked(HWND control, bool checked) {
    SendMessageA(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool isBannerBorder(std::string_view line) {
    if (line.size() < 3 || line.front() != '+' || line.back() != '+') {
        return false;
    }
    for (char ch : line) {
        if (ch != '+' && ch != '-') {
            return false;
        }
    }
    return true;
}

bool isAsciiLogoLine(std::string_view line) {
    return line.find("______") != std::string_view::npos || line.find("F C A S H E R") != std::string_view::npos ||
           line.find("/ ____/") != std::string_view::npos || line.find("/_/   /____/") != std::string_view::npos;
}

std::string trimAscii(std::string_view value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1U])) != 0) {
        --last;
    }
    return std::string(value.substr(first, last - first));
}

void addTab(HWND tabControl, const char* label) {
    TCITEMA item {};
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<char*>(label);
    TabCtrl_InsertItem(tabControl, TabCtrl_GetItemCount(tabControl), &item);
}

void clearList(HWND listView) {
    ListView_DeleteAllItems(listView);
}

void setListColumn(HWND listView, int index, const char* text, int width) {
    LVCOLUMNA column {};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = const_cast<char*>(text);
    column.cx = width;
    column.iSubItem = index;
    ListView_InsertColumn(listView, index, &column);
}

void setListItem(HWND listView, int row, int column, const std::string& value) {
    if (column == 0) {
        LVITEMA item {};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = const_cast<char*>(value.c_str());
        ListView_InsertItem(listView, &item);
    } else {
        ListView_SetItemText(listView, row, column, const_cast<char*>(value.c_str()));
    }
}

}  // namespace

MainWindow::MainWindow() = default;

int MainWindow::run(HINSTANCE instance, int showCommand) {
    INITCOMMONCONTROLSEX commonControls {};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&commonControls);

    if (!registerClass(instance) || !create(instance)) {
        return 1;
    }

    ShowWindow(window_, showCommand);
    UpdateWindow(window_);

    MSG message {};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}

bool MainWindow::registerClass(HINSTANCE instance) {
    WNDCLASSEXA windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &MainWindow::windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "FcasherWindow";
    windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = windowClass.hIcon;
    return RegisterClassExA(&windowClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool MainWindow::create(HINSTANCE instance) {
    instance_ = instance;
    RECT windowRect {0, 0, kWindowWidth, kWindowHeight};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, style, FALSE);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    window_ = CreateWindowExA(0,
                              "FcasherWindow",
                              "Fcasher",
                              style,
                              x > 0 ? x : CW_USEDEFAULT,
                              y > 0 ? y : CW_USEDEFAULT,
                              width,
                              height,
                              nullptr,
                              nullptr,
                              instance,
                              this);
    return window_ != nullptr;
}

void MainWindow::createControls() {
    uiFont_ = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    outputFont_ = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));

    CreateWindowExA(0, "BUTTON", "Operation", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 10, 300, 120, window_, nullptr,
                    instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Mode", WS_CHILD | WS_VISIBLE, 22, 34, 70, 18, window_, nullptr, instance_, nullptr);
    commandCombo_ = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 96, 30,
                                    196, 220, window_, reinterpret_cast<HMENU>(IDC_COMMAND), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Preset", WS_CHILD | WS_VISIBLE, 22, 64, 70, 18, window_, nullptr, instance_, nullptr);
    presetCombo_ = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 96, 60,
                                   196, 220, window_, reinterpret_cast<HMENU>(IDC_PRESET), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Sort", WS_CHILD | WS_VISIBLE, 22, 94, 70, 18, window_, nullptr, instance_, nullptr);
    sortCombo_ = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 96, 90,
                                 196, 220, window_, reinterpret_cast<HMENU>(IDC_SORT), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Categories", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 320, 10, 270, 170, window_, nullptr,
                    instance_, nullptr);
    allCheck_ = CreateWindowExA(0, "BUTTON", "All safe categories", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 334, 32, 150,
                                18, window_, reinterpret_cast<HMENU>(IDC_ALL), instance_, nullptr);
    tempCheck_ = CreateWindowExA(0, "BUTTON", "Temp", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 334, 58, 90, 18, window_,
                                 reinterpret_cast<HMENU>(IDC_TEMP), instance_, nullptr);
    logsCheck_ = CreateWindowExA(0, "BUTTON", "Logs", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 334, 82, 90, 18, window_,
                                 reinterpret_cast<HMENU>(IDC_LOGS), instance_, nullptr);
    browserCheck_ = CreateWindowExA(0, "BUTTON", "Browser", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 334, 106, 90, 18,
                                    window_, reinterpret_cast<HMENU>(IDC_BROWSER), instance_, nullptr);
    thumbnailsCheck_ = CreateWindowExA(0, "BUTTON", "Thumbnails", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 334, 130, 100,
                                       18, window_, reinterpret_cast<HMENU>(IDC_THUMBNAILS), instance_, nullptr);
    shaderCheck_ = CreateWindowExA(0, "BUTTON", "Shader cache", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 456, 58, 110,
                                   18, window_, reinterpret_cast<HMENU>(IDC_SHADER), instance_, nullptr);
    dumpsCheck_ = CreateWindowExA(0, "BUTTON", "Crash dumps", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 456, 82, 110, 18,
                                  window_, reinterpret_cast<HMENU>(IDC_DUMPS), instance_, nullptr);
    recentCheck_ = CreateWindowExA(0, "BUTTON", "Recent", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 456, 106, 110, 18,
                                   window_, reinterpret_cast<HMENU>(IDC_RECENT), instance_, nullptr);
    recycleCheck_ = CreateWindowExA(0, "BUTTON", "Recycle bin", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 456, 130, 110,
                                    18, window_, reinterpret_cast<HMENU>(IDC_RECYCLE), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Filters", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 600, 10, 350, 170, window_, nullptr,
                    instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Min size", WS_CHILD | WS_VISIBLE, 614, 34, 70, 18, window_, nullptr, instance_, nullptr);
    minSizeEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 694, 30, 90, 22,
                                   window_, reinterpret_cast<HMENU>(IDC_MIN_SIZE), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Max size", WS_CHILD | WS_VISIBLE, 794, 34, 70, 18, window_, nullptr, instance_, nullptr);
    maxSizeEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 854, 30, 78, 22,
                                   window_, reinterpret_cast<HMENU>(IDC_MAX_SIZE), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Within", WS_CHILD | WS_VISIBLE, 614, 66, 70, 18, window_, nullptr, instance_, nullptr);
    withinDaysEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 694, 62, 90,
                                      22, window_, reinterpret_cast<HMENU>(IDC_WITHIN_DAYS), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Older than", WS_CHILD | WS_VISIBLE, 794, 66, 70, 18, window_, nullptr, instance_, nullptr);
    olderDaysEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 854, 62, 78, 22,
                                     window_, reinterpret_cast<HMENU>(IDC_OLDER_DAYS), instance_, nullptr);
    CreateWindowExA(0, "STATIC", "Limit", WS_CHILD | WS_VISIBLE, 614, 98, 70, 18, window_, nullptr, instance_, nullptr);
    limitEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 694, 94, 90, 22,
                                 window_, reinterpret_cast<HMENU>(IDC_LIMIT), instance_, nullptr);
    dryRunCheck_ = CreateWindowExA(0, "BUTTON", "Dry run", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 794, 96, 90, 18, window_,
                                   reinterpret_cast<HMENU>(IDC_DRYRUN), instance_, nullptr);
    verboseCheck_ = CreateWindowExA(0, "BUTTON", "Verbose", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 794, 122, 90, 18,
                                    window_, reinterpret_cast<HMENU>(IDC_VERBOSE), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Actions", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 190, 940, 64, window_, nullptr,
                    instance_, nullptr);
    runButton_ = CreateWindowExA(0, "BUTTON", "Run", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 24, 214, 120, 26, window_,
                                 reinterpret_cast<HMENU>(IDC_RUN), instance_, nullptr);
    saveButton_ = CreateWindowExA(0, "BUTTON", "Save Report...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 154, 214, 120, 26,
                                  window_, reinterpret_cast<HMENU>(IDC_SAVE), instance_, nullptr);
    jsonButton_ = CreateWindowExA(0, "BUTTON", "Export JSON...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 284, 214, 120, 26,
                                  window_, reinterpret_cast<HMENU>(IDC_JSON), instance_, nullptr);
    clearButton_ = CreateWindowExA(0, "BUTTON", "Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 414, 214, 120, 26, window_,
                                   reinterpret_cast<HMENU>(IDC_CLEAR), instance_, nullptr);
    statusLabel_ = CreateWindowExA(0, "STATIC", "Ready.", WS_CHILD | WS_VISIBLE, 554, 218, 380, 18, window_,
                                   reinterpret_cast<HMENU>(IDC_STATUS), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Summary", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 264, 940, 132, window_, nullptr,
                    instance_, nullptr);
    summaryList_ = CreateWindowExA(WS_EX_CLIENTEDGE,
                                   WC_LISTVIEWA,
                                   "",
                                   WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
                                   24,
                                   286,
                                   912,
                                   94,
                                   window_,
                                   reinterpret_cast<HMENU>(IDC_SUMMARY),
                                   instance_,
                                   nullptr);

    CreateWindowExA(0, "BUTTON", "Results", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 404, 940, 310, window_, nullptr,
                    instance_, nullptr);
    resultTabs_ = CreateWindowExA(0,
                                  WC_TABCONTROLA,
                                  "",
                                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                  24,
                                  426,
                                  912,
                                  274,
                                  window_,
                                  reinterpret_cast<HMENU>(IDC_RESULTS_TAB),
                                  instance_,
                                  nullptr);
    addTab(resultTabs_, "Files");
    addTab(resultTabs_, "Skipped");
    addTab(resultTabs_, "Details");

    RECT tabRect {0, 0, 0, 0};
    GetWindowRect(resultTabs_, &tabRect);
    MapWindowPoints(nullptr, window_, reinterpret_cast<LPPOINT>(&tabRect), 2);
    RECT pageRect = tabRect;
    TabCtrl_AdjustRect(resultTabs_, FALSE, &pageRect);

    filesList_ = CreateWindowExA(WS_EX_CLIENTEDGE,
                                 WC_LISTVIEWA,
                                 "",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                 pageRect.left,
                                 pageRect.top,
                                 pageRect.right - pageRect.left,
                                 pageRect.bottom - pageRect.top,
                                 window_,
                                 reinterpret_cast<HMENU>(IDC_FILES_LIST),
                                 instance_,
                                 nullptr);
    skippedList_ = CreateWindowExA(WS_EX_CLIENTEDGE,
                                   WC_LISTVIEWA,
                                   "",
                                   WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                   pageRect.left,
                                   pageRect.top,
                                   pageRect.right - pageRect.left,
                                   pageRect.bottom - pageRect.top,
                                   window_,
                                   reinterpret_cast<HMENU>(IDC_SKIPPED_LIST),
                                   instance_,
                                   nullptr);
    detailsEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE,
                                   "EDIT",
                                   "",
                                   WS_CHILD | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                                   pageRect.left,
                                   pageRect.top,
                                   pageRect.right - pageRect.left,
                                   pageRect.bottom - pageRect.top,
                                   window_,
                                   reinterpret_cast<HMENU>(IDC_DETAILS_EDIT),
                                   instance_,
                                   nullptr);

    addComboItems(commandCombo_, kCommandEntries, sizeof(kCommandEntries) / sizeof(kCommandEntries[0]));
    addComboItems(presetCombo_, kPresetEntries, sizeof(kPresetEntries) / sizeof(kPresetEntries[0]));
    addComboItems(sortCombo_, kSortEntries, sizeof(kSortEntries) / sizeof(kSortEntries[0]));
    setChecked(allCheck_, true);
    setChecked(dryRunCheck_, true);

    configureListViews();
    setActiveResultPage(0);
    setOutputText("Run any mode to fill the structured result views.");
}

void MainWindow::applyFonts() const {
    const HWND controls[] = {
        commandCombo_, presetCombo_, sortCombo_, allCheck_,      tempCheck_,   logsCheck_,    browserCheck_,
        thumbnailsCheck_, shaderCheck_, dumpsCheck_, recentCheck_, recycleCheck_, dryRunCheck_, verboseCheck_,
        minSizeEdit_,     maxSizeEdit_, withinDaysEdit_, olderDaysEdit_, limitEdit_, runButton_, saveButton_,
        jsonButton_,      clearButton_, statusLabel_, summaryList_, resultTabs_, filesList_, skippedList_, detailsEdit_
    };

    for (HWND control : controls) {
        if (control != nullptr) {
            SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont_), TRUE);
        }
    }

    if (detailsEdit_ != nullptr) {
        SendMessageA(detailsEdit_,
                     WM_SETFONT,
                     reinterpret_cast<WPARAM>(outputFont_ != nullptr ? outputFont_ : uiFont_),
                     TRUE);
    }
}
void MainWindow::layout(int, int) const {}
void MainWindow::refreshControlStates() const {
    const bool categoriesMode = selectedCommandToken() == "categories";
    const bool cleanMode = selectedCommandToken() == "clean";
    const bool allChecked = isChecked(allCheck_);

    EnableWindow(presetCombo_, categoriesMode ? FALSE : TRUE);
    EnableWindow(sortCombo_, categoriesMode ? FALSE : TRUE);
    EnableWindow(minSizeEdit_, categoriesMode ? FALSE : TRUE);
    EnableWindow(maxSizeEdit_, categoriesMode ? FALSE : TRUE);
    EnableWindow(withinDaysEdit_, categoriesMode ? FALSE : TRUE);
    EnableWindow(olderDaysEdit_, categoriesMode ? FALSE : TRUE);
    EnableWindow(limitEdit_, categoriesMode ? FALSE : TRUE);
    EnableWindow(allCheck_, categoriesMode ? FALSE : TRUE);
    EnableWindow(tempCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(logsCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(browserCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(thumbnailsCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(shaderCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(dumpsCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(recentCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(recycleCheck_, (!categoriesMode && !allChecked) ? TRUE : FALSE);
    EnableWindow(dryRunCheck_, categoriesMode ? FALSE : TRUE);
    EnableWindow(verboseCheck_, TRUE);
    EnableWindow(jsonButton_, isJsonExportAllowed() ? TRUE : FALSE);

    if (cleanMode && !isChecked(dryRunCheck_)) {
        updateStatus("Clean mode armed. Run asks for confirmation before deletion.");
    }
}

void MainWindow::updateStatus(const std::string& text) const {
    if (statusLabel_ != nullptr) {
        SetWindowTextA(statusLabel_, text.c_str());
    }
}

std::vector<std::string> MainWindow::buildArguments() const {
    std::vector<std::string> arguments;
    arguments.push_back(selectedCommandToken());

    if (isChecked(allCheck_)) {
        arguments.push_back("--all");
    } else {
        if (isChecked(tempCheck_)) {
            arguments.push_back("--temp");
        }
        if (isChecked(logsCheck_)) {
            arguments.push_back("--logs");
        }
        if (isChecked(browserCheck_)) {
            arguments.push_back("--browser");
        }
        if (isChecked(thumbnailsCheck_)) {
            arguments.push_back("--thumbnails");
        }
        if (isChecked(shaderCheck_)) {
            arguments.push_back("--shader-cache");
        }
        if (isChecked(dumpsCheck_)) {
            arguments.push_back("--crash-dumps");
        }
        if (isChecked(recentCheck_)) {
            arguments.push_back("--recent");
        }
        if (isChecked(recycleCheck_)) {
            arguments.push_back("--recycle-bin");
        }
    }

    const std::string preset = selectedPresetToken();
    if (!preset.empty()) {
        arguments.push_back("--preset");
        arguments.push_back(preset);
    }

    const std::string sort = selectedSortToken();
    if (!sort.empty() && sort != "native") {
        arguments.push_back("--sort");
        arguments.push_back(sort);
    }

    const std::string minSize = readEditText(minSizeEdit_);
    if (!minSize.empty()) {
        arguments.push_back("--min-size");
        arguments.push_back(minSize);
    }
    const std::string maxSize = readEditText(maxSizeEdit_);
    if (!maxSize.empty()) {
        arguments.push_back("--max-size");
        arguments.push_back(maxSize);
    }
    const std::string withinDays = readEditText(withinDaysEdit_);
    if (!withinDays.empty()) {
        arguments.push_back("--modified-within-days");
        arguments.push_back(withinDays);
    }
    const std::string olderDays = readEditText(olderDaysEdit_);
    if (!olderDays.empty()) {
        arguments.push_back("--older-than-days");
        arguments.push_back(olderDays);
    }
    const std::string limit = readEditText(limitEdit_);
    if (!limit.empty()) {
        arguments.push_back("--limit");
        arguments.push_back(limit);
    }

    if (isChecked(dryRunCheck_)) {
        arguments.push_back("--dry-run");
    }
    if (isChecked(verboseCheck_)) {
        arguments.push_back("--verbose");
    }
    return arguments;
}

bool MainWindow::isCommandClean() const {
    return selectedCommandToken() == "clean";
}

bool MainWindow::isJsonExportAllowed() const {
    return selectedCommandToken() != "categories" && selectedCommandToken() != "clean";
}

void MainWindow::executeCurrentCommand() {
    bool assumeYes = false;
    if (isCommandClean() && !isChecked(dryRunCheck_)) {
        const int response =
            MessageBoxA(window_,
                        "Real cleanup will delete the currently matched files.\n\nContinue with cleanup?",
                        "Fcasher",
                        MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
        if (response != IDYES) {
            updateStatus("Cleanup cancelled before execution.");
            return;
        }
        assumeYes = true;
    }

    updateStatus("Running...");
    const ExecutionResult result = controller_.execute(buildArguments(), assumeYes);
    setStructuredResult(result.structured);
    setOutputText(result.output.empty() ? "No output was generated." : result.output);

    if (result.exitCode == 0) {
        updateStatus("Completed successfully.");
    } else if (result.exitCode == 2) {
        updateStatus("Operation cancelled.");
    } else {
        updateStatus("Completed with warnings or errors.");
        MessageBoxA(window_,
                    "The operation completed with warnings or errors.\nOpen the Details tab for information.",
                    "Fcasher",
                    MB_OK | MB_ICONEXCLAMATION);
    }
}

void MainWindow::exportJson() {
    if (!isJsonExportAllowed()) {
        MessageBoxA(window_,
                    "JSON export is available for scan, preview, analyze, and report modes.",
                    "Fcasher",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    const std::string path = promptSavePath("JSON Files\0*.json\0All Files\0*.*\0", "json");
    if (path.empty()) {
        return;
    }

    updateStatus("Running with JSON export...");
    const ExecutionResult result = controller_.execute(buildArguments(), false, path);
    setStructuredResult(result.structured);
    setOutputText(result.output.empty() ? "No output was generated." : result.output);

    if (result.exitCode == 0) {
        updateStatus("Completed successfully. JSON exported.");
        const std::string message = "JSON exported to:\n" + path;
        MessageBoxA(window_, message.c_str(), "Fcasher", MB_OK | MB_ICONINFORMATION);
    } else {
        updateStatus("JSON export run completed with warnings or errors.");
    }
}

void MainWindow::saveReport() {
    if (currentOutput_.empty()) {
        MessageBoxA(window_, "There is no current report to save.", "Fcasher", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const std::string path = promptSavePath("Text Files\0*.txt\0All Files\0*.*\0", "txt");
    if (path.empty()) {
        return;
    }

    std::string error;
    if (!controller_.saveTextReport(path, currentOutput_, error)) {
        MessageBoxA(window_, error.c_str(), "Unable to save report", MB_OK | MB_ICONERROR);
        updateStatus("Save failed.");
        return;
    }
    updateStatus("Report saved.");
}

void MainWindow::clearOutput() {
    currentOutput_.clear();
    clearStructuredResult();
    SetWindowTextA(detailsEdit_, "");
    updateStatus("Output cleared.");
}
void MainWindow::configureListViews() const {
    ListView_SetExtendedListViewStyle(summaryList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    ListView_SetExtendedListViewStyle(filesList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    ListView_SetExtendedListViewStyle(skippedList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    setListColumn(summaryList_, 0, "Field", 180);
    setListColumn(summaryList_, 1, "Value", 710);

    setListColumn(filesList_, 0, "Category", 120);
    setListColumn(filesList_, 1, "Size", 90);
    setListColumn(filesList_, 2, "Last Write", 140);
    setListColumn(filesList_, 3, "Path", 300);
    setListColumn(filesList_, 4, "Reason", 230);

    setListColumn(skippedList_, 0, "Path", 420);
    setListColumn(skippedList_, 1, "Reason", 430);
}

std::string MainWindow::selectedCommandToken() const {
    return comboToken(commandCombo_, kCommandEntries, sizeof(kCommandEntries) / sizeof(kCommandEntries[0]));
}

std::string MainWindow::selectedPresetToken() const {
    return comboToken(presetCombo_, kPresetEntries, sizeof(kPresetEntries) / sizeof(kPresetEntries[0]));
}

std::string MainWindow::selectedSortToken() const {
    return comboToken(sortCombo_, kSortEntries, sizeof(kSortEntries) / sizeof(kSortEntries[0]));
}
std::string MainWindow::readEditText(HWND control) const {
    const int length = GetWindowTextLengthA(control);
    if (length <= 0) {
        return {};
    }
    std::string value(static_cast<std::size_t>(length) + 1U, '\0');
    GetWindowTextA(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

std::string MainWindow::formatOutputForViewer(const std::string& text) const {
    std::vector<std::string> lines;
    std::string current;

    for (char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty() || text.empty()) {
        lines.push_back(current);
    }

    std::string out;
    bool previousBlank = true;
    for (const auto& rawLine : lines) {
        if (isAsciiLogoLine(rawLine) || isBannerBorder(rawLine)) {
            continue;
        }

        std::string line = rawLine;
        if (line.rfind("| ", 0) == 0 && line.size() >= 4 && line.substr(line.size() - 2U) == " |") {
            line = trimAscii(std::string_view(line).substr(2U, line.size() - 4U));
        }

        const bool blank = trimAscii(line).empty();
        if (blank && previousBlank) {
            continue;
        }

        if (!out.empty()) {
            out += "\r\n";
        }
        out += line;
        previousBlank = blank;
    }

    return out.empty() ? std::string("No details available.") : out;
}

void MainWindow::setOutputText(const std::string& text) {
    currentOutput_ = formatOutputForViewer(text);
    SetWindowTextA(detailsEdit_, currentOutput_.c_str());
}

void MainWindow::clearStructuredResult() {
    fileRowPaths_.clear();
    skippedRowPaths_.clear();
    clearList(summaryList_);
    clearList(filesList_);
    clearList(skippedList_);
}

void MainWindow::populateSummaryList(const StructuredResult& structured) {
    clearList(summaryList_);
    for (int index = 0; index < static_cast<int>(structured.summary.size()); ++index) {
        const auto& item = structured.summary[static_cast<std::size_t>(index)];
        setListItem(summaryList_, index, 0, item.label);
        setListItem(summaryList_, index, 1, item.value);
    }
}

void MainWindow::populateFilesList(const StructuredResult& structured) {
    clearList(filesList_);
    fileRowPaths_.clear();
    for (int index = 0; index < static_cast<int>(structured.files.size()); ++index) {
        const auto& item = structured.files[static_cast<std::size_t>(index)];
        setListItem(filesList_, index, 0, item.category);
        setListItem(filesList_, index, 1, item.size);
        setListItem(filesList_, index, 2, item.lastWrite);
        setListItem(filesList_, index, 3, item.path);
        setListItem(filesList_, index, 4, item.reason);
        fileRowPaths_.push_back(item.path);
    }
}

void MainWindow::populateSkippedList(const StructuredResult& structured) {
    clearList(skippedList_);
    skippedRowPaths_.clear();
    for (int index = 0; index < static_cast<int>(structured.skipped.size()); ++index) {
        const auto& item = structured.skipped[static_cast<std::size_t>(index)];
        setListItem(skippedList_, index, 0, item.path);
        setListItem(skippedList_, index, 1, item.reason);
        skippedRowPaths_.push_back(item.path);
    }
}

void MainWindow::setStructuredResult(const StructuredResult& structured) {
    clearStructuredResult();
    if (!structured.available) {
        return;
    }
    populateSummaryList(structured);
    populateFilesList(structured);
    populateSkippedList(structured);
}

void MainWindow::setActiveResultPage(int pageIndex) const {
    ShowWindow(filesList_, pageIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(skippedList_, pageIndex == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(detailsEdit_, pageIndex == 2 ? SW_SHOW : SW_HIDE);
}

void MainWindow::openSelectedPath(HWND listView, const std::vector<std::string>& rowPaths) const {
    const int selected = ListView_GetNextItem(listView, -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<std::size_t>(selected) >= rowPaths.size()) {
        return;
    }
    openPathInExplorer(rowPaths[static_cast<std::size_t>(selected)]);
}

void MainWindow::openPathInExplorer(const std::string& path) const {
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
        ShellExecuteA(window_, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    const std::string arguments = "/select,\"" + path + "\"";
    ShellExecuteA(window_, "open", "explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL);
}

std::string MainWindow::promptSavePath(const char* filter, const char* defaultExtension) const {
    char buffer[MAX_PATH] = {};
    OPENFILENAMEA dialog {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window_;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = defaultExtension;
    if (GetSaveFileNameA(&dialog) == 0) {
        return {};
    }
    return buffer;
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        createControls();
        applyFonts();
        refreshControlStates();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_RUN:
            executeCurrentCommand();
            return 0;
        case IDC_SAVE:
            saveReport();
            return 0;
        case IDC_JSON:
            exportJson();
            return 0;
        case IDC_CLEAR:
            clearOutput();
            return 0;
        case IDC_ALL:
            refreshControlStates();
            return 0;
        case IDC_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                refreshControlStates();
            }
            return 0;
        default:
            return DefWindowProcA(window_, message, wParam, lParam);
        }
    case WM_NOTIFY: {
        const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
        if (header == nullptr) {
            return DefWindowProcA(window_, message, wParam, lParam);
        }
        if (header->idFrom == IDC_RESULTS_TAB && header->code == TCN_SELCHANGE) {
            setActiveResultPage(TabCtrl_GetCurSel(resultTabs_));
            return 0;
        }
        if (header->idFrom == IDC_FILES_LIST && header->code == NM_DBLCLK) {
            openSelectedPath(filesList_, fileRowPaths_);
            return 0;
        }
        if (header->idFrom == IDC_SKIPPED_LIST && header->code == NM_DBLCLK) {
            openSelectedPath(skippedList_, skippedRowPaths_);
            return 0;
        }
        return DefWindowProcA(window_, message, wParam, lParam);
    }
    case WM_CLOSE:
        DestroyWindow(window_);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(window_, message, wParam, lParam);
    }
}

LRESULT CALLBACK MainWindow::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        self = static_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self != nullptr) {
            self->window_ = window;
        }
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    }
    if (self != nullptr) {
        return self->handleMessage(message, wParam, lParam);
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

}  // namespace fcasher::gui
