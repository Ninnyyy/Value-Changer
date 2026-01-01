// Gorilla Tag - Value (MAX) + Name Changer (2 options, auto-apply)
// Build: Visual Studio C++17 (/std:c++17)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

// -------------------- Styling --------------------
constexpr WORD LIGHT_PINK = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD HOT_PINK = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr WORD SOFT_PINK = FOREGROUND_RED | FOREGROUND_BLUE;
constexpr WORD BRIGHT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD RED_BOLD = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr WORD GREEN_BOLD = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
constexpr WORD DIM = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

static constexpr std::array<WORD, 4> PINK_SHADES{ LIGHT_PINK, HOT_PINK, SOFT_PINK, BRIGHT };

// -------------------- Registry targets --------------------
static constexpr const char* kRegPath = "SOFTWARE\\Another Axiom\\Gorilla Tag";
static constexpr std::array<const char*, 3> kRgbNames{
    "redValue_h2868626173",
    "greenValue_h2874538165",
    "blueValue_h3443272976"
};
static constexpr const char* kPlayerNameValue = "playerName_h3979151953";
static constexpr ULONGLONG kMaxQword = 0xFFFFFFFFFFFFFFFFULL;

// -------------------- Console helpers --------------------
class Console {
public:
    Console() : h_(GetStdHandle(STD_OUTPUT_HANDLE)) {}

    std::optional<WORD> currentAttributes() const {
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(h_, &csbi)) return std::nullopt;
        return csbi.wAttributes;
    }

    void setColor(WORD color) const { SetConsoleTextAttribute(h_, color); }

    int width() const {
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(h_, &csbi)) return 80;
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }

    void clear() const {
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(h_, &csbi)) return;
        const DWORD cellCount = DWORD(csbi.dwSize.X) * DWORD(csbi.dwSize.Y);
        DWORD written = 0;
        COORD home{ 0, 0 };
        FillConsoleOutputCharacterA(h_, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(h_, csbi.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(h_, home);
    }

    void showCursor(bool show) const {
        CONSOLE_CURSOR_INFO ci{};
        if (!GetConsoleCursorInfo(h_, &ci)) return;
        ci.bVisible = show ? TRUE : FALSE;
        SetConsoleCursorInfo(h_, &ci);
    }

private:
    HANDLE h_{ nullptr };
};

class ScopedColor {
public:
    ScopedColor(const Console& c, WORD color) : console_(c) {
        original_ = console_.currentAttributes().value_or(DIM);
        console_.setColor(color);
    }
    ~ScopedColor() { console_.setColor(original_); }
    ScopedColor(const ScopedColor&) = delete;
    ScopedColor& operator=(const ScopedColor&) = delete;

private:
    const Console& console_;
    WORD original_{ DIM };
};

// -------------------- Small utils --------------------
static void SleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static std::string Repeat(char c, int n) { return std::string(n > 0 ? n : 0, c); }

static std::string Trim(std::string s) {
    auto notSpace = [](unsigned char ch) { return ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n'; };
    while (!s.empty() && !notSpace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && !notSpace((unsigned char)s.back())) s.pop_back();
    return s;
}

static std::string SanitizeOneLine(std::string s) {
    for (char& c : s) {
        if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    }
    return s;
}

static std::string Win32ErrorString(DWORD err) {
    LPSTR msgBuf = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD len = FormatMessageA(flags, nullptr, err, 0, (LPSTR)&msgBuf, 0, nullptr);

    std::string out = (len && msgBuf) ? std::string(msgBuf, len) : "Unknown error";
    if (msgBuf) LocalFree(msgBuf);
    while (!out.empty() && (out.back() == '\r' || out.back() == '\n' || out.back() == ' ')) out.pop_back();
    return out;
}

static void PauseEnter() {
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static void ProgressBar(const Console& console, const std::string& label, int steps = 22, int delayMs = 25) {
    console.showCursor(false);
    std::cout << label << std::flush;
    for (int i = 0; i <= steps; ++i) {
        int pct = (i * 100) / steps;
        ScopedColor c(console, PINK_SHADES[i % PINK_SHADES.size()]);
        std::cout << "\r" << label << " [";
        for (int j = 0; j < i; ++j) std::cout << "#";
        std::cout << ">";
        for (int j = i; j < steps; ++j) std::cout << ".";
        std::cout << "] " << std::setw(3) << pct << "%";
        std::cout.flush();
        SleepMs(delayMs);
    }
    std::cout << "\n";
    console.showCursor(true);
}

static void ShimmerTitle(const Console& console, const std::string& title) {
    console.showCursor(false);
    const int w = std::max(console.width(), 70);
    const int pad = (w - (int)title.size()) / 2;

    for (int i = 0; i < 10; ++i) {
        ScopedColor c(console, PINK_SHADES[i % PINK_SHADES.size()]);
        std::cout << "\r" << Repeat(' ', pad > 0 ? pad : 0) << title << std::flush;
        SleepMs(80);
    }
    std::cout << "\n";
    console.showCursor(true);
}

// -------------------- Simple box UI --------------------
static void BoxHeader(const Console& console, const std::string& title) {
    const int w = std::max(console.width(), 70);
    const int inner = w - 2;
    std::cout << "+" << Repeat('-', inner) << "+\n";
    {
        ScopedColor c(console, LIGHT_PINK);
        std::string t = " " + title + " ";
        if ((int)t.size() > inner) t.resize(inner);
        int left = (inner - (int)t.size()) / 2;
        std::cout << "|" << Repeat(' ', left) << t << Repeat(' ', inner - left - (int)t.size()) << "|\n";
    }
    std::cout << "+" << Repeat('-', inner) << "+\n";
}

static void BoxLine(const Console& console, const std::string& left, const std::string& right = "") {
    const int w = std::max(console.width(), 70);
    const int inner = w - 2;

    std::string content = left;
    if (!right.empty()) {
        int dots = inner - (int)left.size() - (int)right.size() - 2;
        if (dots < 1) dots = 1;
        content = left + " " + Repeat('.', dots) + " " + right;
    }
    if ((int)content.size() > inner) content.resize(inner);
    std::cout << "|" << content << Repeat(' ', inner - (int)content.size()) << "|\n";
}

static void BoxFooter(const Console& console) {
    const int w = std::max(console.width(), 70);
    const int inner = w - 2;
    std::cout << "+" << Repeat('-', inner) << "+\n";
}

static int Menu2(const Console& console) {
    BoxLine(console, " 1) Set brightness to MAX (auto apply)");
    BoxLine(console, " 2) Set custom player name (auto apply)");
    BoxLine(console, " 3) Exit");
    BoxLine(console, " Select option:");
    BoxFooter(console);

    std::cout << "> " << std::flush;
    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return 0;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

// -------------------- Registry RAII --------------------
class RegKey {
public:
    RegKey() = default;
    explicit RegKey(HKEY h) : h_(h) {}
    ~RegKey() { if (h_) RegCloseKey(h_); }

    RegKey(const RegKey&) = delete;
    RegKey& operator=(const RegKey&) = delete;

    RegKey(RegKey&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
    RegKey& operator=(RegKey&& o) noexcept {
        if (this != &o) {
            if (h_) RegCloseKey(h_);
            h_ = o.h_;
            o.h_ = nullptr;
        }
        return *this;
    }

    HKEY get() const { return h_; }
    explicit operator bool() const { return h_ != nullptr; }

private:
    HKEY h_{ nullptr };
};

static std::optional<RegKey> OpenGTKey(LONG& outErr) {
    HKEY raw = nullptr;
    outErr = RegOpenKeyExA(HKEY_CURRENT_USER, kRegPath, 0, KEY_SET_VALUE, &raw);
    if (outErr != ERROR_SUCCESS) return std::nullopt;
    return RegKey(raw);
}

static bool WriteQword(HKEY key, const char* name, ULONGLONG value, LONG& outErr) {
    outErr = RegSetValueExA(key, name, 0, REG_QWORD,
        reinterpret_cast<const BYTE*>(&value),
        static_cast<DWORD>(sizeof(value)));
    return outErr == ERROR_SUCCESS;
}

static bool WriteString(HKEY key, const char* name, const std::string& value, LONG& outErr) {
    outErr = RegSetValueExA(key, name, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>(value.size() + 1));
    return outErr == ERROR_SUCCESS;
}

// -------------------- Actions --------------------
static void ApplyMaxBrightness(const Console& console) {
    LONG err = 0;
    auto keyOpt = OpenGTKey(err);
    if (!keyOpt) {
        ScopedColor r(console, RED_BOLD);
        std::cout << "\nCould not open registry key.\n"
            << "Error " << err << ": " << Win32ErrorString((DWORD)err) << "\n"
            << "Launch Gorilla Tag at least once.\n";
        return;
    }
    RegKey key = std::move(*keyOpt);

    ProgressBar(console, "Applying MAX brightness", 22, 25);

    bool ok = true;
    for (int i = 0; i < 3; ++i) {
        LONG werr = 0;
        if (!WriteQword(key.get(), kRgbNames[i], kMaxQword, werr)) {
            ok = false;
            ScopedColor r(console, RED_BOLD);
            std::cout << "\nWrite failed for " << kRgbNames[i]
                << " (" << werr << ": " << Win32ErrorString((DWORD)werr) << ")\n";
        }
    }

    if (ok) {
        ScopedColor g(console, GREEN_BOLD);
        std::cout << "\nSuccess. Brightness set to MAX.\n"
            << "Restart Gorilla Tag.\n";
    }
}

static void ApplyPlayerName(const Console& console) {
    std::cout << "\nEnter new player name: ";
    std::string name;
    std::getline(std::cin, name);

    name = SanitizeOneLine(Trim(name));
    if (name.empty()) {
        ScopedColor r(console, RED_BOLD);
        std::cout << "\nName cannot be empty.\n";
        return;
    }
    if (name.size() > 32) {
        ScopedColor r(console, RED_BOLD);
        std::cout << "\nName too long (keep it <= 32).\n";
        return;
    }

    LONG err = 0;
    auto keyOpt = OpenGTKey(err);
    if (!keyOpt) {
        ScopedColor r(console, RED_BOLD);
        std::cout << "\nCould not open registry key.\n"
            << "Error " << err << ": " << Win32ErrorString((DWORD)err) << "\n"
            << "Launch Gorilla Tag at least once.\n";
        return;
    }
    RegKey key = std::move(*keyOpt);

    ProgressBar(console, "Applying player name", 18, 30);

    LONG werr = 0;
    if (!WriteString(key.get(), kPlayerNameValue, name, werr)) {
        ScopedColor r(console, RED_BOLD);
        std::cout << "\nWrite failed (" << werr << ": " << Win32ErrorString((DWORD)werr) << ")\n";
        return;
    }

    ScopedColor g(console, GREEN_BOLD);
    std::cout << "\nSuccess. Name updated.\n"
        << "Restart Gorilla Tag.\n";
}

// -------------------- Main --------------------
int main() {
    Console console;
    SetConsoleTitleA("Gorilla Tag Value + Name Changer");

    console.clear();
    ShimmerTitle(console, "Gorilla Tag Value + Name Changer");

    for (;;) {
        console.clear();
        BoxHeader(console, "Value + Name Changer");
        BoxLine(console, " Brightness target", "ALWAYS MAX");
        BoxLine(console, " Name target", "playerName_h3979151953");
        BoxLine(console, "");
        int choice = Menu2(console);

        console.clear();
        BoxHeader(console, "Action");

        if (choice == 1) {
            ApplyMaxBrightness(console);
            PauseEnter();
        }
        else if (choice == 2) {
            ApplyPlayerName(console);
            PauseEnter();
        }
        else if (choice == 3) {
            console.showCursor(true);
            return 0;
        }
        else {
            ScopedColor r(console, RED_BOLD);
            std::cout << "\nInvalid selection.\n";
            PauseEnter();
        }
    }
}
