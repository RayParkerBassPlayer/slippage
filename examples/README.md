# Slippage Library Examples

This directory contains examples demonstrating how to use the Slippage library in your own C++ projects.

## Building the Examples

First, install the Slippage library:

```bash
# From the project root
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build

# Or install to a custom prefix
cmake --install build --prefix /usr/local
```

Then build the examples:

```bash
cd examples
cmake -B build -S .
cmake --build build

# Run the main example
./build/example_usage

# Run the version example
./build/version_example
```

If you installed to a custom prefix, tell CMake where to find the library:

```bash
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build build
./build/example_usage
```

## Example Overview

### example_usage.cpp

Demonstrates all the CLI options available through the library API:

- **Example 1: Basic Assignment** - Basic slip assignment without any special options
- **Example 2: Ignore Length Mode** - `engine.setIgnoreLength(true)` (CLI: `--ignore-length`)
- **Example 3: Price Calculation** - `engine.setPricePerSqFt(2.75)` (CLI: `--price-per-sqft 2.75`)
- **Example 4: Verbose Mode** - `engine.setVerbose(true)` (CLI: `--verbose`)
- **Example 5: Combined Options** - Multiple options together

### version_example.cpp

Demonstrates how to access version information programmatically:

- Runtime version queries using `slippage::version::*` functions
- Compile-time version checks using macros
- Version comparison for API compatibility checks

```cpp
#include <slippage/version.hpp>

std::cout << slippage::version::full() << "\n";  // "Slippage v1.0.0"
std::cout << slippage::version::major() << "\n"; // 1

// Compile-time check
#if SLIPPAGE_VERSION_MAJOR >= 1
    // Use v1.x API
#endif
```

## Library API Quick Reference

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <slippage/dimensions.hpp>

// Create slips and members
std::vector<Slip> slips;
slips.emplace_back("S1", 30, 0, 12, 0);  // ID, length_ft, length_in, width_ft, width_in

std::vector<Member> members;
members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
// ID, boat_length_ft, boat_length_in, boat_width_ft, boat_width_in, current_slip (optional), status

// Create engine and configure options
AssignmentEngine engine(std::move(members), std::move(slips));
engine.setIgnoreLength(true);      // --ignore-length
engine.setPricePerSqFt(2.75);      // --price-per-sqft 2.75
engine.setVerbose(true);           // --verbose

// Run assignment
auto assignments = engine.assign();

// Process results
for (const auto &assignment : assignments){
    std::cout << "Member: " << assignment.memberId() << "\n";
    std::cout << "Slip: " << assignment.slipId() << "\n";
    std::cout << "Status: " << (int)assignment.status() << "\n";
    std::cout << "Price: $" << assignment.price() << "\n";
    std::cout << "Comment: " << assignment.comment() << "\n";
}
```

## Dock Status Values

```cpp
Member::DockStatus::PERMANENT      // Cannot be moved or evicted
Member::DockStatus::YEAR_OFF       // Not assigned (taking year off)
Member::DockStatus::WAITING_LIST   // Higher priority, can evict TEMPORARY/UNASSIGNED
Member::DockStatus::TEMPORARY      // Can evict UNASSIGNED
Member::DockStatus::UNASSIGNED     // Lowest priority
```

## Assignment Status Values

```cpp
Assignment::Status::PERMANENT   // Member kept their current slip (auto-upgraded)
Assignment::Status::NEW         // Member assigned to a different slip
Assignment::Status::UNASSIGNED  // Member could not be assigned
```
