# Slippage - Boat Slip Assignment System

A C++ command-line application that assigns boat slips to marina club members based on boat dimensions, member priority, and slip availability.

## Features

- **Dock status priority system**: Five-level priority system (permanent, year-off, waiting-list, temporary, unassigned)
- **Priority-based assignment**: Within each status level, members with lower IDs get preference
- **Permanent member protection**: Permanent members cannot be moved or evicted
- **Automatic status upgrade**: Members who keep their slip are automatically upgraded to permanent
- **Smart eviction and reassignment**: Higher-priority members can reclaim slips
- **Size-based protection**: Members keep small slips that larger boats can't fit in
- **Best-fit algorithm**: Assigns smallest suitable slip to minimize waste, with width margin as tie-breaker
- **Preference for current slips**: Tries to keep members in their existing slips
- **Tight fit warnings**: Alerts when boats fit with less than 6 inches of width clearance (shown as "TIGHT FIT")
- **Price calculation**: Optional per-square-foot pricing based on larger of boat or slip area
- **Flexible length handling**: Optional mode to ignore length constraints, allowing boats to overhang slips

## Quick Start

### Cloning the Repository

This project uses git submodules for external dependencies. Clone with:

```bash
# Clone with submodules
git clone --recursive <repository-url>

# Or if already cloned without --recursive:
git submodule update --init --recursive
```

### Option 1: Install from .deb Package

```bash
# Build the Debian package
./build-deb.sh

# Install the package
sudo dpkg -i ../slippage_*.deb

# The slippage command is now available system-wide
slippage --slips slips.csv --members members.csv
```

### Option 2: Build from Source

```bash
# Configure the build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build the application
cmake --build build -j4

# Optionally install to system (default: /usr/local/bin)
sudo cmake --install build
```

### Running

```bash
# Show help
./build/slippage --help

# Show version
./build/slippage --version

# Run assignment (output to stdout with markers)
./build/slippage --slips slips.csv --members members.csv

# Save output to file
./build/slippage --slips slips.csv --members members.csv --output assignments.csv

# Verbose mode - shows assignment decisions in real-time
./build/slippage --slips slips.csv --members members.csv --verbose

# Combine verbose with file output
./build/slippage --slips slips.csv --members members.csv --output assignments.csv --verbose

# Ignore boat length when fitting (only check width)
./build/slippage --slips slips.csv --members members.csv --ignore-length

# Calculate price per square foot (e.g., $2.75/sqft)
./build/slippage --slips slips.csv --members members.csv --price-per-sqft 2.75

```

### Command-Line Options

```
USAGE:
  slippage --slips <slips.csv> --members <members.csv> [OPTIONS]
  slippage --version
  slippage --help

REQUIRED ARGUMENTS:
  --slips <file>     CSV file containing slip information
  --members <file>   CSV file containing member information

OPTIONS:
  --output <file>    Write assignments to file instead of stdout
  --verbose          Print detailed assignment progress (phases and passes)
  --ignore-length    Only check width when determining fit (show length
                     differences in comments)
  --price-per-sqft <amount>
                     Calculate price per square foot (uses larger of boat
                     or slip area); adds 'price' column to output
  --help, -h         Show help message and exit
  --version, -v      Show version information and exit
```

## Input File Formats

### members.csv

CSV file with member information:

```csv
member_id,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,current_slip,dock_status
M1,20,0,10,0,S5,temporary
M2,18,6,9,0,S3,waiting-list
M100,22,0,11,0,S10,permanent
M200,19,0,9,6,,unassigned
M50,21,0,10,0,S7,year-off
```

**Fields:**
- `member_id`: Unique member identifier (string)
- `boat_length_ft`: Boat length in feet (integer)
- `boat_length_in`: Additional inches for boat length (integer, 0-11)
- `boat_width_ft`: Boat width/beam in feet (integer)
- `boat_width_in`: Additional inches for boat width (integer, 0-11)
- `current_slip`: Current slip ID or empty if none (string)
- `dock_status`: Member's dock status (string: permanent, year-off, waiting-list, temporary, unassigned)

**Dock Status Values:**
- `permanent`: Member has permanent assignment, cannot be moved or evicted
- `year-off`: Member is taking a year off, will not be assigned (slip left empty)
- `waiting-list`: High priority for assignment, can evict temporary and unassigned members
- `temporary`: Standard priority, can evict unassigned members  
- `unassigned`: New member seeking first assignment, lowest priority

### slips.csv

CSV file with slip information:

```csv
slip_id,max_length_ft,max_length_in,max_width_ft,max_width_in
S1,20,0,10,0
S2,25,0,12,0
S3,30,0,15,0
```

**Fields:**
- `slip_id`: Unique slip identifier (string)
- `max_length_ft`: Maximum boat length in feet (integer)
- `max_length_in`: Additional inches for max length (integer, 0-11)
- `max_width_ft`: Maximum boat width in feet (integer)
- `max_width_in`: Additional inches for max width (integer, 0-11)

## Output Format

The program outputs assignments in CSV format:

```csv
member_id,assigned_slip,status,dock_status,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,price,upgraded,comment
M1,S5,PERMANENT,temporary,20,0,10,0,550.00,true,
M2,S3,PERMANENT,waiting-list,18,6,9,0,458.25,true,"TIGHT FIT"
M100,S10,PERMANENT,permanent,22,0,11,0,726.00,false,"NOTE: Boat does not fit in assigned slip"
M50,,UNASSIGNED,year-off,21,0,10,0,0.00,false,"Year off - not assigned"
M150,S8,NEW,temporary,19,0,9,6,462.00,false,
M200,,UNASSIGNED,unassigned,35,0,14,0,0.00,false,"Boat too large for all available slips"
```

**Status values:**
- `PERMANENT`: Member has a permanent assignment (either original or auto-upgraded)
- `NEW`: Member assigned to a different slip
- `UNASSIGNED`: Member did not receive a slip assignment

**Price field:**
- Only populated when `--price-per-sqft` is specified
- Calculated as: `(max of boat area or slip area) × price per sqft`
- Rounded to 2 decimal places
- Empty for unassigned members

**Dock Status field:**
- Shows the member's input dock status from the CSV
- Indicates their priority level in the assignment system

**Upgraded field:**
- `true`: Member was automatically upgraded to PERMANENT (kept their current slip)
- `false`: Member was not upgraded (NEW assignment, already PERMANENT, or UNASSIGNED)
- Auto-upgrade happens for all members who keep their current slip

**Comment field:**
- Usually empty for normal assignments
- `TIGHT FIT`: Boat width is within 6 inches of slip width (tight clearance warning)
- For permanent members: `NOTE: Boat does not fit in assigned slip` indicates data issue
- With `--ignore-length` flag: shows length difference (e.g., `NOTE: boat is 3' 6" longer than slip`)
- Multiple notes are separated by semicolons (e.g., `NOTE: boat is 5' longer than slip; TIGHT FIT`)
- For unassigned members, provides diagnostic reason:
  - `Boat too large for all available slips`
  - `Evicted - boat doesn't fit previous slip, all X suitable slips taken`
  - `Evicted - outranked by higher priority member(s), all X suitable slips taken`
  - `All X suitable slips taken by higher priority members`
- Comments containing commas are properly quoted per CSV RFC 4180

**Output modes:**
- **Without `--output` and without `--verbose`**: CSV is written to stdout wrapped with markers:
  ```
  >>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS START
  [CSV content]
  >>>>>>>>>>>>>>>>>>>>>>>>>>>ASSIGNMENTS END
  ```
  This makes programmatic extraction easy
  
- **With `--verbose`**: CSV output appears without markers (verbose progress goes to stdout)
- **With `--output <file>`**: CSV is written directly to the specified file without markers

**Verbose mode (`--verbose`):**
- Shows all assignment phases with detailed progress
- Phase 1: Permanent member assignments
- Phase 2: Year-off members (not assigned)
- Phase 3: Waiting-list members
- Phase 4: Temporary members
- Phase 5: Unassigned members  
- Displays pass numbers and individual assignment decisions
- Example output:
  ```
  ===== PHASE 1: Permanent Member Assignments =====
    Member M100 -> Slip S10 (PERMANENT)
  
  ===== PHASE 2: Year-Off Members =====
    Member M50 (YEAR-OFF) - previous slip: S7
  
  ===== PHASE 3: waiting-list Members =====
  
  --- Pass 1 ---
    Member M2 -> Slip S3 (keeping current)
  
  Phase 3 complete after 1 pass(es)
  
  ===== PHASE 4: temporary Members =====
  
  --- Pass 1 ---
    Member M1 -> Slip S5 (keeping current)
  
  Phase 4 complete after 1 pass(es)
  ```

**Note:** All members appear in the output. Members who don't receive an assignment have an empty `assigned_slip` field and status `UNASSIGNED`.

## Assignment Rules

For detailed information about how slips are assigned, see [ASSIGNMENT_RULES.md](ASSIGNMENT_RULES.md).

### Quick Summary

1. **Dock status priority**: PERMANENT (locked) > WAITING_LIST > TEMPORARY > UNASSIGNED
2. **Permanent members** keep their slips and cannot be moved or evicted
3. **Year-off members** are not assigned (slip left available)
4. **Within each dock status**, priority is determined by member ID (lower IDs = higher priority)
5. Members **prefer to stay** in their current slip when possible
6. **Higher-priority members** can evict lower-priority members (based on dock status first, then ID)
7. **Size matters**: You can't evict someone if your boat won't fit in their slip
8. System chooses **smallest fitting slip** to minimize waste
9. **Evicted members** are automatically reconsidered for other slips
10. **Auto-upgrade**: Members who keep their slip are automatically upgraded to PERMANENT

## Development

### Project Structure

```
Slippage/
├── assignment_engine.h/cpp  # Core assignment logic
├── assignment.h/cpp          # Assignment result data structure
├── csv_parser.h/cpp          # CSV file parsing
├── dimensions.h/cpp          # Boat/slip dimensions
├── main.cpp                  # CLI entry point
├── member.h/cpp              # Member data structure
├── slip.h/cpp                # Slip data structure
├── tests/                    # Unit tests
│   └── test_assignment.cpp
├── CMakeLists.txt            # Build configuration
└── README.md
```

### Building and Running Tests

```bash
# Build tests
cmake --build build --target slippage_tests

# Run tests
./build/slippage_tests

# Run tests with CTest
ctest --test-dir build --output-on-failure
```

### VSCode Integration

The project includes VSCode tasks and launch configurations:

**Build Tasks** (Ctrl+Shift+B):
- `CMake: Build` - Build main application
- `CMake: Build Tests` - Build tests
- `CMake: Clean` - Clean build artifacts
- `CMake: Rebuild` - Clean and rebuild

**Test Tasks**:
- `CMake: Run Tests` - Build and run tests
- `CTest: Run All Tests` - Run tests via CTest

**Debug Configurations** (F5):
- `Debug slippage` - Debug main application
- `Debug tests` - Debug test suite

### Requirements

- C++17 compatible compiler (g++, clang++)
- CMake 3.10 or higher
- Git (for submodule management)
- [csv-parser](https://github.com/vincentlaucsb/csv-parser) (included as submodule in `external/csv-parser/`)
- [Catch2](https://github.com/catchorg/Catch2) v2.x (vendored in `external/catch2/`)

### Adding New Tests

Tests use the Catch2 framework. Add new test cases to `tests/test_assignment.cpp`:

```cpp
TEST_CASE("Your test description", "[tag]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
}
```

## Packaging and Distribution

### Building a Debian Package

The project includes Debian packaging files for easy distribution:

```bash
# Build the .deb package
./build-deb.sh

# Package will be created in the parent directory
# Install with:
sudo dpkg -i ../slippage_1.0.0_amd64.deb

# Uninstall with:
sudo apt-get remove slippage
```

**Package contents:**
- Binary: `/usr/bin/slippage`
- Documentation: `/usr/share/doc/slippage/`
- Example files: `/usr/share/doc/slippage/examples/`

### Installing from Source

```bash
# Build and install to /usr/local
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build

# Install to custom prefix
cmake --install build --prefix /opt/slippage
```

## Code Style

This project follows these conventions:

- Pointer/reference symbols adjacent to variable name: `Type *ptr`, not `Type* ptr`
- No nested braces on same line for if/else, try/catch
- Empty line between variable declarations and condition checks
- Use "you" or no pronouns in comments, never "we"
- Member variables prefixed with `m` (e.g., `mMembers`)

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]

## Authors

[Add author information here]
