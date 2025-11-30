# Slippage - Boat Slip Assignment System

A C++ command-line application that assigns boat slips to marina club members based on boat dimensions, member priority, and slip availability.

## Features

- **Priority-based assignment**: Members with lower IDs get preference
- **Permanent member protection**: Permanent assignments cannot be evicted
- **Smart eviction and reassignment**: Higher-priority members can reclaim slips
- **Size-based protection**: Members keep small slips that larger boats can't fit in
- **Best-fit algorithm**: Assigns smallest suitable slip to minimize waste
- **Preference for current slips**: Tries to keep members in their existing slips

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
```

### Command-Line Options

```
USAGE:
  slippage --slips <slips.csv> --members <members.csv>
  slippage --version
  slippage --help

REQUIRED ARGUMENTS:
  --slips <file>     CSV file containing slip information
  --members <file>   CSV file containing member information

OPTIONS:
  --help, -h         Show help message and exit
  --version, -v      Show version information and exit
```

## Input File Formats

### members.csv

CSV file with member information:

```csv
member_id,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,current_slip,is_permanent
M1,20,0,10,0,S5,0
M2,18,6,9,0,S3,0
M100,22,0,11,0,S10,1
```

**Fields:**
- `member_id`: Unique member identifier (string)
- `boat_length_ft`: Boat length in feet (integer)
- `boat_length_in`: Additional inches for boat length (integer, 0-11)
- `boat_width_ft`: Boat width/beam in feet (integer)
- `boat_width_in`: Additional inches for boat width (integer, 0-11)
- `current_slip`: Current slip ID or empty if none (string)
- `is_permanent`: 1 if permanent assignment, 0 otherwise (integer)

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

The program outputs assignments to stdout in CSV format:

```csv
member_id,assigned_slip,status,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,comment
M1,S5,SAME,20,0,10,0,
M2,S3,SAME,18,6,9,0,
M100,S10,PERMANENT,22,0,11,0,WARNING: Boat does not fit in assigned slip
```

**Status values:**
- `PERMANENT`: Member has a permanent assignment
- `SAME`: Member kept their current slip
- `NEW`: Member assigned to a different slip
- `UNASSIGNED`: Member did not receive a slip assignment

**Comment field:**
- Usually empty for normal assignments
- Shows `WARNING: Boat does not fit in assigned slip` when a permanent member's boat exceeds their slip dimensions
- This warning indicates a data issue that should be reviewed

**Note:** All members appear in the output. Members who don't receive an assignment have an empty `assigned_slip` field and status `UNASSIGNED`.

## Assignment Rules

For detailed information about how slips are assigned, see [ASSIGNMENT_RULES.md](ASSIGNMENT_RULES.md).

### Quick Summary

1. **Permanent members** keep their slips and cannot be evicted
2. **Priority** is determined by member ID (lower IDs = higher priority)
3. Members **prefer to stay** in their current slip when possible
4. **Higher-priority members** can evict lower-priority members
5. **Size matters**: You can't evict someone if your boat won't fit in their slip
6. System chooses **smallest fitting slip** to minimize waste
7. **Evicted members** are automatically reconsidered for other slips

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
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, false);
    
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
