#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>

#include "app/cli.hpp"
#include "app/command_dispatcher.hpp"
#include "app/report_formatter.hpp"

namespace {

constexpr WORD kRetroNormalAttributes =
    BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD kRetroPromptAttributes =
    BACKGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD kRetroErrorAttributes = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

bool hasTransientConsole() {
    using get_console_process_list_fn = DWORD(WINAPI*)(LPDWORD, DWORD);
    const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32 == nullptr) {
        return false;
    }

    const auto getConsoleProcessList =
        reinterpret_cast<get_console_process_list_fn>(GetProcAddress(kernel32, "GetConsoleProcessList"));
    if (getConsoleProcessList == nullptr) {
        return false;
    }

    DWORD processIds[2] = {};
    return getConsoleProcessList(processIds, 2) == 1;
}

HANDLE consoleOutputHandle() {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (handle == INVALID_HANDLE_VALUE || handle == NULL || GetConsoleMode(handle, &mode) == 0) {
        return NULL;
    }
    return handle;
}

void setConsoleAttributes(WORD attributes) {
    HANDLE output = consoleOutputHandle();
    if (output != NULL) {
        SetConsoleTextAttribute(output, attributes);
    }
}

void configureRetroConsole() {
    HANDLE output = consoleOutputHandle();
    if (output == NULL) {
        return;
    }

    SetConsoleTitleA("Fcasher");

    CONSOLE_SCREEN_BUFFER_INFO info {};
    if (GetConsoleScreenBufferInfo(output, &info) != 0) {
        const COORD largest = GetLargestConsoleWindowSize(output);
        const SHORT width = (largest.X >= 108) ? 108 : largest.X;
        const SHORT height = (largest.Y >= 34) ? 34 : largest.Y;

        if (width > 0 && height > 0) {
            COORD bufferSize = info.dwSize;
            if (bufferSize.X < width) {
                bufferSize.X = width;
            }
            if (bufferSize.Y < 500) {
                bufferSize.Y = 500;
            }
            SetConsoleScreenBufferSize(output, bufferSize);

            SMALL_RECT windowRect = {0, 0, (SHORT) (width - 1), (SHORT) (height - 1)};
            SetConsoleWindowInfo(output, TRUE, &windowRect);

            bufferSize.X = width;
            if (bufferSize.Y < height) {
                bufferSize.Y = height;
            }
            SetConsoleScreenBufferSize(output, bufferSize);
        }
    }

    setConsoleAttributes(kRetroNormalAttributes);

    if (GetConsoleScreenBufferInfo(output, &info) != 0) {
        DWORD written = 0;
        const DWORD totalCells = (DWORD) info.dwSize.X * (DWORD) info.dwSize.Y;
        const COORD home = {0, 0};
        FillConsoleOutputCharacterA(output, ' ', totalCells, home, &written);
        FillConsoleOutputAttribute(output, kRetroNormalAttributes, totalCells, home, &written);
        SetConsoleCursorPosition(output, home);
    }
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::vector<std::string> tokenizeInput(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';

    for (char ch : line) {
        if ((ch == '"' || ch == '\'') && (!inQuotes || quoteChar == ch)) {
            if (inQuotes && quoteChar == ch) {
                inQuotes = false;
                quoteChar = '\0';
            } else if (!inQuotes) {
                inQuotes = true;
                quoteChar = ch;
            }
            continue;
        }

        if (!inQuotes && std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

fcasher::app::CliOptions parseTokens(const fcasher::app::CliParser& parser,
                                     const std::vector<std::string>& tokens) {
    std::vector<std::string> argvStorage;
    argvStorage.reserve(tokens.size() + 1U);
    argvStorage.push_back("fcasher");
    argvStorage.insert(argvStorage.end(), tokens.begin(), tokens.end());

    std::vector<char*> argv;
    argv.reserve(argvStorage.size());
    for (auto& argument : argvStorage) {
        argv.push_back(argument.data());
    }

    return parser.parse(static_cast<int>(argv.size()), argv.data());
}

void pauseBeforeExitIfNeeded(bool enabled) {
    if (!enabled) {
        return;
    }

    std::cout << "\nPress Enter to exit...";
    std::cout.flush();

    std::cin.clear();
    std::string line;
    std::getline(std::cin, line);
}

void printPrompt() {
    setConsoleAttributes(kRetroPromptAttributes);
    std::cout << "fcasher> ";
    std::cout.flush();
    setConsoleAttributes(kRetroNormalAttributes);
}

void printErrorLine(const std::string& line) {
    setConsoleAttributes(kRetroErrorAttributes);
    std::cerr << line << '\n';
    setConsoleAttributes(kRetroNormalAttributes);
}

int executeInvocation(const fcasher::app::CliParser& parser,
                      const fcasher::app::CliOptions& options,
                      bool pauseOnExit) {
    if (!options.errors.empty()) {
        for (const auto& error : options.errors) {
            printErrorLine("error: " + error);
        }
        std::cerr << '\n' << parser.helpText();
        pauseBeforeExitIfNeeded(pauseOnExit);
        return 1;
    }

    if (options.showHelp || options.command == fcasher::app::CommandType::Help) {
        std::cout << parser.helpText();
        pauseBeforeExitIfNeeded(pauseOnExit);
        return 0;
    }

    fcasher::app::CommandDispatcher dispatcher;
    const int exitCode = dispatcher.run(options);
    pauseBeforeExitIfNeeded(pauseOnExit);
    return exitCode;
}

int runInteractiveLauncher(const fcasher::app::CliParser& parser) {
    using fcasher::app::ReportFormatter;

    std::cout << ReportFormatter::logo() << '\n';
    std::cout << ReportFormatter::banner("FCASHER INTERACTIVE LAUNCHER",
                                         "Type a command below. Use 'help' for reference or 'exit' to close.")
              << "\n\n";
    std::cout << "Examples:\n";
    std::cout << "  scan --all\n";
    std::cout << "  preview --preset quick-sweep --sort size --limit 25\n";
    std::cout << "  analyze --all --min-size 16MB --sort size\n";
    std::cout << "  clean --temp --max-size 128MB --dry-run\n";
    std::cout << "  categories\n\n";

    while (true) {
        printPrompt();

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << '\n';
            return 0;
        }

        const std::string trimmed = trim(input);
        if (trimmed.empty()) {
            continue;
        }

        const std::string lowered = toLower(trimmed);
        if (lowered == "exit" || lowered == "quit") {
            return 0;
        }

        const auto options = parseTokens(parser, tokenizeInput(trimmed));
        std::cout << '\n';
        const int exitCode = executeInvocation(parser, options, false);
        std::cout << '\n';

        if (exitCode != 0 && exitCode != 2) {
            std::cout << "Command exited with code " << exitCode << ".\n\n";
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    const fcasher::app::CliParser parser;
    const bool transientConsole = hasTransientConsole();
    if (transientConsole) {
        configureRetroConsole();
    }

    if (argc <= 1 && transientConsole) {
        return runInteractiveLauncher(parser);
    }

    const fcasher::app::CliOptions options = parser.parse(argc, argv);
    return executeInvocation(parser, options, false);
}
