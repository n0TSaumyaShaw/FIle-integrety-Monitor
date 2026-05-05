#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <functional>

namespace fs = std::filesystem;

class FileIntegrityMonitor {

private: 
    std::unordered_map<std::string, std::string> fileBaseline;
    const std::string BASELINE_FILE = "baseline_data.txt";

    std::string calculateHash(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return "";
        
        std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::hash<std::string> hasher;
        return std::to_string(hasher(contents)); 
    }

    void saveBaseline() {
        std::ofstream outFile(BASELINE_FILE);
        if (!outFile) {
            std::cerr << "[-] Error: Could not save baseline to disk.\n";
            return;
        }
        for (const auto& pair : fileBaseline) {
            outFile << pair.second << " " << pair.first << "\n";
        }
    }

    bool loadBaseline() {
        std::ifstream inFile(BASELINE_FILE);
        if (!inFile) return false;

        std::string hash, path;
        fileBaseline.clear();
        
        while (inFile >> hash) {
            std::getline(inFile >> std::ws, path);
            fileBaseline[path] = hash;
        }
        return true;
    }

public: 
    FileIntegrityMonitor() {
        if (loadBaseline()) {
            std::cout << "[*] Found existing baseline data. Loaded " << fileBaseline.size() << " records.\n";
        }
    }

    void createBaseline(const std::string& targetDirectory) {
        if (!fs::exists(targetDirectory) || !fs::is_directory(targetDirectory)) {
            std::cout << "[-] Error: Invalid directory path.\n";
            return;
        }

        std::cout << "[*] Calculating baseline for directory: " << targetDirectory << "\n";
        fileBaseline.clear(); 

        for (const auto& entry : fs::recursive_directory_iterator(targetDirectory)) {
            if (fs::is_regular_file(entry)) {
                std::string path = entry.path().string();
                fileBaseline[path] = calculateHash(path);
            }
        }
        
        std::cout << "[+] Baseline successfully created. " << fileBaseline.size() << " files secured.\n";
        saveBaseline(); 
    }

    void monitorDirectory(const std::string& targetDirectory) {
        if (fileBaseline.empty()) {
            std::cout << "[-] Error: No baseline found in memory or on disk. Please create one first.\n";
            return;
        }

        std::cout << "[*] Beginning monitor scan on: " << targetDirectory << "\n";
        int alerts = 0;

        for (const auto& entry : fs::recursive_directory_iterator(targetDirectory)) {
            if (fs::is_regular_file(entry)) {
                std::string path = entry.path().string();
                std::string currentHash = calculateHash(path);

                if (fileBaseline.find(path) == fileBaseline.end()) {
                    std::cout << "[ALERT] NEW FILE CREATED: " << path << "\n";
                    alerts++;
                }
                else if (fileBaseline[path] != currentHash) {
                    std::cout << "[ALERT] FILE MODIFIED: " << path << "\n";
                    alerts++;
                }
            }
        }

        for (const auto& baselineEntry : fileBaseline) {
            if (!fs::exists(baselineEntry.first)) {
                std::cout << "[ALERT] FILE DELETED: " << baselineEntry.first << "\n";
                alerts++;
            }
        }
        
        if (alerts == 0) {
            std::cout << "[+] Scan complete. All files are secure and unchanged.\n";
        } else {
            std::cout << "[-] Scan complete. Found " << alerts << " security alerts!\n";
        }
    }
};

int main() {
    FileIntegrityMonitor fim; 
    
    int choice;
    std::string directory;

    std::cout << "====================================\n";
    std::cout << "       FILE INTEGRITY MONITOR       \n";
    std::cout << "====================================\n";

    while (true) {
        std::cout << "\n1. Create/Overwrite Baseline\n2. Run Security Scan\n3. Exit\nChoice: ";
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        if (choice == 1) {
            std::cout << "Enter full directory path to secure: ";
            std::cin.ignore(); 
            std::getline(std::cin, directory); 
            fim.createBaseline(directory); 
        } 
        else if (choice == 2) {
            std::cout << "Enter full directory path to scan: ";
            std::cin.ignore();
            std::getline(std::cin, directory);
            fim.monitorDirectory(directory); 
        } 
        else if (choice == 3) {
            std::cout << "Exiting File Integrity Monitor...\n";
            break;
        } else {
            std::cout << "[-] Invalid choice. Please select 1, 2, or 3.\n";
        }
    }
    return 0;
}