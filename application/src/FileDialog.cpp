#include "FileDialog.h"

#include <cstdio>
#include <cstdlib>
#include <array>

namespace
{
    enum class Helper { None, Zenity, KDialog };

    Helper detectHelper()
    {
        // Resolved once; which GUI dialog helper (if any) is installed.
        static const Helper helper = []()
        {
            if (std::system("command -v zenity >/dev/null 2>&1") == 0)
                return Helper::Zenity;
            if (std::system("command -v kdialog >/dev/null 2>&1") == 0)
                return Helper::KDialog;
            return Helper::None;
        }();
        return helper;
    }

    // Wrap a string as a single-quoted shell argument, escaping any quotes.
    std::string shellQuote(const std::string &s)
    {
        std::string out = "'";
        for (char c : s)
        {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += "'";
        return out;
    }

    // Run a command and return its trimmed first line of stdout (empty on
    // failure or cancellation).
    std::string runForPath(const std::string &cmd)
    {
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
            return {};

        std::string result;
        std::array<char, 4096> buf;
        while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
            result += buf.data();

        const int status = pclose(pipe);
        if (status != 0)
            return {}; // non-zero: user cancelled or error

        // Trim trailing newline/whitespace.
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
            result.pop_back();
        return result;
    }
}

namespace FileDialog
{
    bool available()
    {
        return detectHelper() != Helper::None;
    }

    std::string open(const std::string &title)
    {
        switch (detectHelper())
        {
        case Helper::Zenity:
            return runForPath("zenity --file-selection --title=" + shellQuote(title));
        case Helper::KDialog:
            return runForPath("kdialog --title " + shellQuote(title) + " --getopenfilename");
        default:
            return {};
        }
    }

    std::string save(const std::string &title, const std::string &defaultName)
    {
        switch (detectHelper())
        {
        case Helper::Zenity:
            return runForPath("zenity --file-selection --save --confirm-overwrite --title=" +
                              shellQuote(title) + " --filename=" + shellQuote(defaultName));
        case Helper::KDialog:
            return runForPath("kdialog --title " + shellQuote(title) +
                              " --getsavefilename " + shellQuote(defaultName));
        default:
            return {};
        }
    }

    std::string folder(const std::string &title)
    {
        switch (detectHelper())
        {
        case Helper::Zenity:
            return runForPath("zenity --file-selection --directory --title=" + shellQuote(title));
        case Helper::KDialog:
            return runForPath("kdialog --title " + shellQuote(title) + " --getexistingdirectory");
        default:
            return {};
        }
    }
}
