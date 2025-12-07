#include <slippage/version.hpp>
#include <iostream>

int main(){
    std::cout << "Slippage Library Version Information\n";
    std::cout << "====================================\n\n";
    
    // Using the namespace API functions
    std::cout << "Version: " << slippage::version::string() << "\n";
    std::cout << "Full: " << slippage::version::full() << "\n";
    std::cout << "Major: " << slippage::version::major() << "\n";
    std::cout << "Minor: " << slippage::version::minor() << "\n";
    std::cout << "Patch: " << slippage::version::patch() << "\n\n";
    
    // Using macros (for compile-time checks)
    std::cout << "Compile-time macros:\n";
    std::cout << "  SLIPPAGE_VERSION_MAJOR: " << SLIPPAGE_VERSION_MAJOR << "\n";
    std::cout << "  SLIPPAGE_VERSION_MINOR: " << SLIPPAGE_VERSION_MINOR << "\n";
    std::cout << "  SLIPPAGE_VERSION_PATCH: " << SLIPPAGE_VERSION_PATCH << "\n";
    std::cout << "  SLIPPAGE_VERSION_STRING: " << SLIPPAGE_VERSION_STRING << "\n\n";
    
    // Example: Version checking at runtime
    if (slippage::version::major() >= 1){
        std::cout << "Using stable API (v1.x or later)\n";
    }
    
    // Example: Conditional compilation based on version
#if SLIPPAGE_VERSION_MAJOR >= 1 && SLIPPAGE_VERSION_MINOR >= 0
    std::cout << "Features available in v1.0+: All core functionality\n";
#endif
    
    return 0;
}
