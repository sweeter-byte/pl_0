#ifndef BANNER_H
#define BANNER_H

#include <iostream>
#include <string>

namespace Banner {

// ANSI颜色代码
namespace Color {
    const char* Reset   = "\033[0m";
    const char* Bold    = "\033[1m";
    const char* Cyan    = "\033[36m";
    const char* Magenta = "\033[35m";
    const char* Green   = "\033[32m";
    const char* Yellow  = "\033[33m";
}

inline void printLogo(bool useColor = true) {
    if (useColor) std::cout << Color::Cyan;
    
    std::cout << R"(
       ██████╗ ██╗      ██╗ ██████╗      ██████╗ 
       ██╔══██╗██║     ██╔╝██╔═████╗    ██╔════╝ 
       ██████╔╝██║    ██╔╝ ██║██╔██║    ██║      
       ██╔═══╝ ██║   ██╔╝  ████╔╝██║    ██║      
       ██║     ██████╔╝    ╚██████╔╝    ╚██████╗ 
       ╚═╝     ╚═════╝      ╚═════╝      ╚═════╝ 
                   COMPILER v1.0                        
)" << std::endl;

    if (useColor) std::cout << Color::Magenta;

    std::cout << R"(
            @@@%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%###%%#####
            @@@%%%%%%%%%%%%%%%%%%%#%%%%%%%%%%%%%#%%%%%##%%####
            @@@@%%%%%%%%%%%%%%%%%%%##%%%%%%%%%%%%##%%%%##%%###
            @@@@@%%%%%%%%%%%%%%###%%###%%%%%%%%%%%%##%%##%%###
            @@@@%%%%%%%%%###%%%###########%%%%%%%%%%%%%%#%%%##
            @@@@%#%%%%%%#**##%###****++**+++*+*#%%%%%%%%%%%%##
            @@@%#%%##%%%#*+**####**+*+---=-+++=+#%%%%%%%%%%%##
            @@@%#@%#%%%###*==**###*++---:.:-+*--+###%%%%%%%%%#
            @@@@%@@#%%%####+:=+-+***+::.:-==-:::-###%%%%%%%%%#
            @@@@@@@%%@@%%%###+=::::--:::::::::::+###%%%%%%%%##
            @@@@@@@@%@@@@%%%%*:::::::::::::::::-###%%%%%%%%%%%
            @@@@@@@@@@@@@%%%%#+-:::::::::::::::+##%%%%%%%%%%%%
            @@@@@@@@@@@@@%%%%%#+-:::::::::::::-*###%%%%%%%%%%%
            %%%%%%@@@@@@@%%%%%%##*+=-::::::-=+*##%%%%%%%%%%%%%
            %%%%%%@@@@@@@%%%%%%##%%%#*+=-+*###%%%%%%%%%%%%%%%%
            %%%%%%%@@@@@@%%%%%%#%%%%%%%####%%%%%%%%%%%%%%%%%%%
            %%%%%%%@@@@@@%%%%%%%%%%%%%###%%%%%%%%%%%%%%%%%%%%%
            %%%%%%%@@@@@@%%%%%%%%%%%%###%%%%%%%%%%%%%%%%%%%%%%
                                        -- Haibara Ai --
)" << std::endl;

    if (useColor) std::cout << Color::Reset;
}

inline void printVersion(bool useColor = true) {
    if (useColor) std::cout << Color::Green << Color::Bold;
    
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════╗
║                     PL/0 COMPILER v1.0                           ║
╠══════════════════════════════════════════════════════════════════╣
║  A compiler and interpreter for the PL/0 programming language    ║
║  Supports the full PL/0 grammar with Clang-style diagnostics     ║
║                                                                  ║
║  Author  : myRan @ NUAA                                          ║
║  GitHub  : https://github.com/sweeter-byte/pl_0                  ║
╚══════════════════════════════════════════════════════════════════╝
)" << std::endl;

    if (useColor) std::cout << Color::Reset;
}

inline void printBuildSuccess(bool useColor = true) {
    printLogo(useColor);
    
    if (useColor) std::cout << Color::Green;
    
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════╗
║                    ✓ BUILD SUCCESSFUL!                           ║
╚══════════════════════════════════════════════════════════════════╝
)" << std::endl;

    if (useColor) std::cout << Color::Reset;
}

// 简化版logo（适合在帮助信息中显示）
inline void printMiniLogo(bool useColor = true) {
    if (useColor) std::cout << Color::Cyan;
    
    std::cout << R"(
    ____  __    ____  ______
   / __ \/ /   / __ \/ ____/
  / /_/ / /   / / / / /     
 / ____/ /___/ /_/ / /___   
/_/   /_____/\____/\____/   
        COMPILER v1.0
)" << std::endl;

    if (useColor) std::cout << Color::Reset;
}

} // namespace Banner

#endif // BANNER_H