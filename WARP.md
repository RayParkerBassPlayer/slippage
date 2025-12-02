# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

Slippage is a C++ command-line application for assigning boat slips to marina club members based on boat dimensions, member priority, and slip availability. It implements a priority-based assignment algorithm with eviction and reassignment logic.

## Cloning and Setup

This project uses git submodules for the csv-parser dependency:

```bash
# Clone with submodules
git clone --recursive <repository-url>

# Or if already cloned, initialize submodules
git submodule update --init --recursive
```

## Build Commands

```bash
# Configure and build (release)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# Build debug version
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Build only tests
cmake --build build --target slippage_tests

# Clean build
cmake --build build --target clean

# Full rebuild
rm -rf build && cmake -B build -S . && cmake --build build
```

## Running Tests

```bash
# Run tests directly
./build/slippage_tests

# Run tests with CTest
ctest --test-dir build --output-on-failure

# Run specific test by name
./build/slippage_tests "Your test description"
```

## Running the Application

```bash
# Show help
./build/slippage --help

# Show version
./build/slippage --version

# Run assignment (stdout with markers)
./build/slippage --slips test_slips.csv --members test_members.csv

# Save output to file
./build/slippage --slips test_slips.csv --members test_members.csv --output output.csv

# Verbose mode (shows Pass 1, Pass 2, etc. on stdout)
./build/slippage --slips test_slips.csv --members test_members.csv --verbose

# Verbose with file output (progress to stdout, CSV to file)
./build/slippage --slips test_slips.csv --members test_members.csv --output output.csv --verbose

# Ignore boat length when fitting (only check width)
./build/slippage --slips test_slips.csv --members test_members.csv --ignore-length

# Calculate price per square foot (e.g., $2.75/sqft)
./build/slippage --slips test_slips.csv --members test_members.csv --price-per-sqft 2.75

```

## Packaging

```bash
# Build Debian package
./build-deb.sh

# Package created in parent directory as: ../slippage_1.0.0_amd64.deb
```

## Architecture

### Core Components

**AssignmentEngine** (`assignment_engine.h/cpp`): The heart of the system. Implements a multi-phase assignment algorithm:
- Phase 1: Lock PERMANENT members into their designated slips (cannot be moved or evicted)
- Phase 2: Process YEAR_OFF members (not assigned, slip freed)
- Phase 3: Assign WAITING_LIST members (iterative with eviction support)
- Phase 4: Assign TEMPORARY members (iterative with eviction support)
- Phase 5: Assign UNASSIGNED members (iterative with eviction support)
- Final: Auto-upgrade members who kept their slip to PERMANENT

The engine maintains two critical maps:
- `mSlipOccupant`: Maps slip IDs to currently assigned members
- `mMemberAssignment`: Maps members to their assigned slip IDs

**Assignment Algorithm Flow**:
1. Process members by dock status priority: PERMANENT → YEAR_OFF → WAITING_LIST → TEMPORARY → UNASSIGNED
2. Within each dock status, sort by member ID (lower = higher priority)
3. For each unassigned member, try current slip first (minimize disruption)
4. If current slip unavailable, find best-fit alternative (smallest slip that fits, with width margin as tie-breaker)
5. Higher-priority members can evict lower-priority members (based on dock status first, then ID)
6. Evicted members are reconsidered in next iteration
7. Loop until no changes occur (stable assignment reached for that dock status)
8. Add comments for tight fits (< 6" width clearance), length overhangs, or unassignment reasons
9. Calculate price if requested (max of boat/slip area × price-per-sqft)
10. Auto-upgrade members who kept their slip to PERMANENT status

**Data Models** (`dimensions.h/cpp`, `slip.h/cpp`, `member.h/cpp`, `assignment.h/cpp`):
- `Dimensions`: Stores boat/slip dimensions in total inches for precise comparison; provides `fitsInWidthOnly()` for width-only checks
- `Slip`: Slip ID and maximum dimensions; provides `fits()` method and `fitsWidthOnly()` for --ignore-length mode
- `Member`: Member ID, boat dimensions, current slip, dock status (enum: PERMANENT, YEAR_OFF, WAITING_LIST, TEMPORARY, UNASSIGNED); implements comparison operators for priority
- `Assignment`: Result structure with status (PERMANENT, NEW, UNASSIGNED), dock status, boat and slip dimensions, price, upgraded flag, and comment

**CsvParser** (`csv_parser.h/cpp`): Handles CSV I/O using csv-parser library
- `parseMembers()`: Reads members.csv
- `parseSlips()`: Reads slips.csv  
- `writeAssignments()`: Outputs assignments to a stream (private method)
- `operator<<`: Stream operator for writing assignments to any output stream
- Properly quotes CSV fields containing commas per RFC 4180

**Main** (`main.cpp`): CLI entry point with argument parsing and help text

### Dependencies

- **csv-parser** (Vincent La): Header-only CSV parsing library in `external/csv-parser/` (git submodule)
- **Catch2 v2**: Testing framework in `external/catch2/` (vendored)
- csv-parser requires submodule initialization; Catch2 is directly included

### Key Algorithm Concepts

**Dock Status Priority**: Primary priority is dock status: PERMANENT (locked) > WAITING_LIST > TEMPORARY > UNASSIGNED. YEAR_OFF members are not assigned.

**Member ID Priority**: Within each dock status level, members with lower IDs get preference. Member "M1" > "M2" > "M100" in priority.

**Permanent Member Protection**: Permanent members cannot be moved or evicted by anyone. They are locked in during Phase 1.

**Year-Off Handling**: Year-off members do not receive slip assignments. Their slips become available for other members.

**Size-Based Protection**: A member keeps their slip if a higher-priority member's boat won't fit in it. Prevents futile evictions.

**Best-Fit Selection**: When finding alternatives:
- **Normal mode**: System chooses smallest fitting slip by area (length × width) to minimize waste; width margin used as tie-breaker when areas are equal
- **Ignore-length mode**: System prioritizes slips that minimize boat overhang (length difference), then selects smallest by area among slips with equal overhang, then uses width margin as final tie-breaker

**Iterative Eviction**: Evicted members are automatically reconsidered. Algorithm loops until stable (no more changes).

**Ignore-Length Mode (`--ignore-length`)**: When enabled, only boat width is checked for fitting. Boats can be longer than slips. The algorithm prioritizes assigning boats to slips with the least length overhang to minimize the amount boats extend beyond their slips. Length differences are shown in assignment comments (e.g., "NOTE: boat is 3' 6\" longer than slip").

**Tight Fit Detection**: When a boat's width is less than 6 inches narrower than the slip width, a "TIGHT FIT" note is added to the assignment comment. This alerts users to assignments with minimal width clearance.

**Price Calculation (`--price-per-sqft`)**: When enabled with a price value (e.g., 2.75), calculates the rental/dockage price for each assignment:
- Price = max(boat area, slip area) × price per square foot
- Uses the larger of boat or slip area to ensure fair pricing
- Rounded to 2 decimal places
- Unassigned members have price of 0.0
- Adds a 'price' column to CSV output

**Automatic Status Upgrade**: Members who keep their current slip are automatically upgraded to PERMANENT status:
- Upgrade happens in final pass after all assignments are complete
- Upgraded members have `upgraded=true` in CSV output
- Already-permanent members have `upgraded=false` (they weren't upgraded, already were permanent)
- The 'upgraded' column is always present in CSV output

## Code Style Conventions

Follow these C++ coding standards (enforced by user rules):

- **Pointer/reference symbols**: Adjacent to variable name, not type
  - Correct: `void save(const std::string &name, Member *member)`
  - Wrong: `void save(const std::string& name, Member* member)`

- **else/else if/catch on new lines**: Opening brace at end of line, but else/else if/catch on new line
  ```cpp
  // Correct
  if (condition) {
  }
  else {
  }
  
  // Wrong
  if (condition) {
  } else {
  }
  ```

- **Spacing between declaration and condition check**: Add empty line between variable declarations/assignments and following if statements
  ```cpp
  // Correct
  Slip *slip = findSlipById(id);
  
  if (slip)
  {
      // ...
  }
  ```

- **Pronouns in comments**: Use "you" or no pronouns, never "we"
  - Correct: "Check if boat fits in slip"
  - Correct: "You can evict lower-priority members"
  - Wrong: "We check if boat fits"

- **Member variables**: Prefixed with `m` (e.g., `mMembers`, `mSlips`, `mSlipOccupant`)

- **Class organization**: Private members and methods come first, then public interface
  - For classes, private is the default and doesn't need explicit `private:` keyword
  - If public types (enums, typedefs) are needed by private members, declare them first in a public section
  - Order: `class Name {` → private members/methods → `public:` → public interface → `};`
  ```cpp
  // Correct
  class Example {
      int mValue;  // private by default
      void helper();
  
  public:
      Example();
      void doSomething();
  };
  
  // With public enum needed by private member
  class Assignment {
  public:
      enum class Status { PERMANENT, NEW, UNASSIGNED };
  
  private:
      Status mStatus;  // uses public enum
  
  public:
      Assignment();
      Status status() const;
  };
  ```

## Testing

Tests use Catch2 v2 framework in `tests/test_assignment.cpp`. The test file contains comprehensive scenarios covering:
- Basic assignments
- Priority-based eviction
- Dock status priority (permanent, year-off, waiting-list, temporary, unassigned)
- Permanent member protection
- Year-off member handling
- Size-based protection
- Iterative reassignment
- Best-fit selection with width priority tie-breaking
- Tight fit warnings
- Price calculation (various scenarios)
- Automatic status upgrade
- Ignore-length mode
- Edge cases (no slips, no fits, multiple evictions)

When adding tests, follow the existing pattern:
```cpp
TEST_CASE("Description of what you're testing", "[category]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
}
```

## CLI Features

**Verbose Mode (`--verbose`)**:
- Prints detailed assignment progress to stdout
- Shows all 5 phases: PERMANENT → YEAR_OFF → WAITING_LIST → TEMPORARY → UNASSIGNED
- Displays pass numbers (Pass 1, Pass 2, etc.) and individual decisions within each phase
- Useful for understanding assignment behavior and debugging
- When verbose is enabled, stdout markers are NOT shown (CSV output appears without wrapping)

**File Output (`--output <file>`)**:
- Writes CSV to specified file instead of stdout
- No marker wrapping when outputting to file
- Can be combined with `--verbose` (progress to stdout, CSV to file)

**Stdout Markers**:
- When outputting to stdout (no `--output`) WITHOUT `--verbose`, CSV is wrapped:
  - `>>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS START`
  - `>>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS END`
- Makes programmatic extraction easy from combined output
- With `--verbose`, markers are NOT shown (verbose output and CSV are both on stdout)

**Unassigned Diagnostics**:
- Comment field explains why each member wasn't assigned
- Different messages for different scenarios (too large, evicted, outranked)
- Shows count of suitable slips that were unavailable
- All comments with commas are properly quoted per CSV RFC 4180

## Test Data Generation

**`generate_test_data.py`**: Python script to create realistic test scenarios
- Generates 200 slips and 225 members by default
- Realistic size distributions (more medium slips, fewer very large)
- Creates capacity constraints and priority conflicts
- Reproducible with random seed for consistent testing
- Run: `python3 generate_test_data.py`

## Important Files

- `ASSIGNMENT_RULES.md`: Comprehensive documentation of assignment algorithm and rules. Reference this when modifying assignment logic.
- `README.md`: User-facing documentation with file formats, usage examples, and quick start guide
- `version.h.in`: Version template configured by CMake; generates `build/version.h`
- `CMakeLists.txt`: Build configuration; defines two targets: `slippage` (main) and `slippage_tests`
- `generate_test_data.py`: Script to generate complex test scenarios with realistic data

## VSCode Integration

The repository includes VSCode configurations in `.vscode/`:
- **Build tasks** (Ctrl+Shift+B): CMake build, clean, rebuild, test targets
- **Launch configs** (F5): Debug main application or tests
- **C++ IntelliSense**: Configured for CMake build system

## Common Development Patterns

**Dimension Handling**: All dimensions are stored in total inches internally. Use the `Dimensions` class constructor which takes feet and inches separately but converts to total inches for comparison.

**Optional Current Slips**: Members may have no current slip. Always check `member.currentSlip().has_value()` before accessing with `.value()`.

**Iterative Assignment**: The assignment algorithm uses a `changesMade` flag. Any eviction sets this to true, triggering another iteration. This ensures all members are reconsidered after evictions.

**Status Determination**: Assignment status is determined by the outcome. If member kept their slip: PERMANENT (auto-upgraded), if different slip: NEW, if no slip: UNASSIGNED.
