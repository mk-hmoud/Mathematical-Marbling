#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <string>

/**
 * @brief Native open/save/folder dialogs via the system helper (zenity or
 * kdialog). Each returns the chosen path, or an empty string if the user
 * cancelled or no helper is available (in which case callers fall back to the
 * typed path).
 */
namespace FileDialog
{
    bool available();
    std::string open(const std::string &title);
    std::string save(const std::string &title, const std::string &defaultName);
    std::string folder(const std::string &title);
}

#endif // FILE_DIALOG_H
