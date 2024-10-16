#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <map>
#include <regex>
#include <sstream>
#include <conio.h> // For _getch()

namespace fs = std::filesystem;

// func prototypes
std::string sanitiseFileName(const std::string& fileName);
bool isValidMediaFileName(const std::string& fileName);
bool isAudioVideoFile(const std::string& extension);
std::string getCurrentDateTime();
void generatePlaylist(const std::string& exeName, const fs::path& currentPath);
std::string wideStringToString(const std::wstring& wstr);

// quick super-simple check for illegal characters in proposed m3u filename
std::string sanitiseFileName(const std::string& fileName) {
    std::regex illegalChars(R"([<>:"/\\|?*])");
    std::string sanitised = std::regex_replace(fileName, illegalChars, "_");
    sanitised.erase(sanitised.find_last_not_of(' ') + 1); // trim trailing spaces
    sanitised.erase(0, sanitised.find_first_not_of(' ')); // trim leading spaces
    return sanitised.empty() ? "default_playlist.m3u" : sanitised;
}

// basic check for potentially problematic characters in media filename
bool isValidMediaFileName(const std::string& fileName) {
    std::regex validPattern(R"(^[^<>:"/\\|?*]+\.[^.]+$)");
    return std::regex_match(fileName, validPattern);
}

// basic match for common relevant media filetypes based on extension
bool isAudioVideoFile(const std::string& extension) {
    static const std::vector<std::string> validExtensions = {
        ".mp3", ".wav", ".ogg", ".flac", ".m4a", ".aac",
        ".mp4", ".avi", ".mkv", ".mov", ".wmv"
    };
    std::string lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    return std::find(validExtensions.begin(), validExtensions.end(), lowerExt) != validExtensions.end();
}

std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm bt;
    localtime_s(&bt, &in_time_t);

    std::stringstream ss;
    ss << std::put_time(&bt, "%Y%m%d-%H%M%S");
    return ss.str();
}

std::string wideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void generatePlaylist(const std::string& exeName, const fs::path& currentPath, bool& errorOccurred) {
    bool recursive = exeName.find("--R+") != std::string::npos;
    bool sortAlphabetically = exeName.find("--A+") != std::string::npos;
    bool sortByDateTime = exeName.find("--D+") != std::string::npos;

    std::map<fs::path, std::vector<fs::path>> fileGroups;
    std::stringstream errorStream;

    // Collect files
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(currentPath)) {
            if (fs::is_regular_file(entry) && isAudioVideoFile(entry.path().extension().string())) {
                std::string fileName = entry.path().filename().string();
                if (!isValidMediaFileName(fileName)) {
                    errorStream << "Warning: Skipping file with invalid name: " << fileName << std::endl;
                    errorOccurred = true;
                    continue;
                }
                fileGroups[entry.path().parent_path()].push_back(entry.path());
            }
        }
    }
    else {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            if (fs::is_regular_file(entry) && isAudioVideoFile(entry.path().extension().string())) {
                std::string fileName = entry.path().filename().string();
                if (!isValidMediaFileName(fileName)) {
                    errorStream << "Warning: Skipping file with invalid name: " << fileName << std::endl;
                    errorOccurred = true;
                    continue;
                }
                fileGroups[currentPath].push_back(entry.path());
            }
        }
    }

    // Sort files within each group
    for (auto& group : fileGroups) {
        if (sortAlphabetically) {
            std::sort(group.second.begin(), group.second.end());
        }
        else if (sortByDateTime) {
            std::sort(group.second.begin(), group.second.end(), [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) < fs::last_write_time(b);
                });
        }
    }

    std::string playlistName = currentPath.filename().string() + ".m3u";
    if (fs::exists(currentPath / playlistName)) {
        playlistName = currentPath.filename().string() + "-" + getCurrentDateTime() + ".m3u";
    }
    playlistName = sanitiseFileName(playlistName);

    std::ofstream playlist(currentPath / playlistName);
    if (!playlist.is_open()) {
        throw std::runtime_error("Unable to create playlist file");
    }

    playlist << "#EXTM3U" << std::endl;

    for (const auto& group : fileGroups) {
        for (const auto& file : group.second) {
            try {
                std::string relativePath = fs::relative(file, currentPath).string();
                playlist << relativePath << std::endl;
            }
            catch (const std::exception& e) {
                errorStream << "Error calculating relative path for: " << file << ". " << e.what() << std::endl;
                errorOccurred = true;
            }
        }
    }

    playlist.close();

    std::cout << "Playlist generated: " << playlistName << std::endl;

    if (errorOccurred) {
        std::string errorContent = errorStream.str();
        std::cout << "\nWarnings/Errors occurred:\n" << errorContent;

        std::string errorFileName = "error-" + getCurrentDateTime() + ".txt";
        std::ofstream errorFile(errorFileName);
        if (errorFile.is_open()) {
            errorFile << errorContent;
            errorFile.close();
            std::cout << "Detailed error log written to: " << errorFileName << std::endl;
        }
        else {
            std::cerr << "Failed to create error log file." << std::endl;
        }
    }
}

int main() {
    try {
        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        fs::path fullExePath(exePath);
        std::string exeName = wideStringToString(fullExePath.filename().wstring());
        fs::path currentPath = fullExePath.parent_path();

        bool errorOccurred = false;
        generatePlaylist(exeName, currentPath, errorOccurred);

        if (errorOccurred) {
            std::cout << "\nPress any key to exit...";
            _getch(); // pause for user input - chance to review that error occured
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        std::string errorFileName = "error-" + getCurrentDateTime() + ".txt";
        std::ofstream errorFile(errorFileName);
        if (errorFile.is_open()) {
            errorFile << "Fatal Error: " << e.what() << std::endl;
            errorFile.close();
            std::cerr << "Error details written to: " << errorFileName << std::endl;
        }
        else {
            std::cerr << "Failed to create error log file." << std::endl;
        }

        std::cout << "Press any key to exit...";
        _getch(); // pause for user input - aware of error
        return 1;
    }

    return 0;
}
