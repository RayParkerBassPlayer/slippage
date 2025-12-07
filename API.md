# Slippage C++ API Documentation

Complete reference for using Slippage as a library in your C++ applications.

## Table of Contents

- [Getting Started](#getting-started)
- [Core Concepts](#core-concepts)
- [Class Reference](#class-reference)
  - [Dimensions](#dimensions)
  - [Slip](#slip)
  - [Member](#member)
  - [Assignment](#assignment)
  - [AssignmentEngine](#assignmentengine)
  - [Version](#version)
  - [CsvParser](#csvparser)
- [Complete Usage Examples](#complete-usage-examples)

---

## Getting Started

### Installation

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
sudo cmake --install build
```

### Basic Project Setup

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

find_package(Slippage REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE Slippage::slippage_lib)
```

**main.cpp:**
```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <iostream>

int main(){
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    for (const auto &assignment : assignments){
        std::cout << assignment.memberId() << " -> " << assignment.slipId() << "\n";
    }
    
    return 0;
}
```

---

## Core Concepts

### Dimension Handling
All dimensions are stored internally in **total inches** for precise comparison. The public API accepts dimensions as **feet + inches** for convenience.

### Dock Status Priority
Members are processed in this priority order:
1. **PERMANENT** - Cannot be moved or evicted, locked to their current slip
2. **YEAR_OFF** - Not assigned, slip becomes available
3. **WAITING_LIST** - Can evict TEMPORARY and UNASSIGNED members
4. **TEMPORARY** - Can evict UNASSIGNED members
5. **UNASSIGNED** - Lowest priority, new members seeking first assignment

Within each priority level, lower member IDs have higher priority (e.g., M1 > M2 > M100).

### Assignment Algorithm
1. Lock PERMANENT members into their current slips
2. Free up slips from YEAR_OFF members
3. Assign WAITING_LIST members (iterative with eviction)
4. Assign TEMPORARY members (iterative with eviction)
5. Assign UNASSIGNED members (iterative with eviction)
6. Auto-upgrade members who kept their slip to PERMANENT

The algorithm loops within each phase until no changes occur (stable state).

---

## Class Reference

### Dimensions

Represents physical dimensions (length and width) of boats or slips.

**Header:** `<slippage/dimensions.hpp>`

#### Constructor

```cpp
Dimensions(int feetLength, int inchesLength, int feetWidth, int inchesWidth);
```

Creates a Dimensions object with specified measurements.

**Parameters:**
- `feetLength` - Length in feet
- `inchesLength` - Additional inches for length (0-11)
- `feetWidth` - Width in feet  
- `inchesWidth` - Additional inches for width (0-11)

**Example:**
```cpp
// 28 feet 6 inches long, 11 feet 3 inches wide
Dimensions boat(28, 6, 11, 3);
```

#### Public Methods

##### lengthInches()
```cpp
int lengthInches() const;
```

Returns the total length in inches.

**Example:**
```cpp
Dimensions d(28, 6, 11, 0);
int length = d.lengthInches();  // Returns 342 (28*12 + 6)
```

##### widthInches()
```cpp
int widthInches() const;
```

Returns the total width in inches.

**Example:**
```cpp
Dimensions d(28, 0, 11, 3);
int width = d.widthInches();  // Returns 135 (11*12 + 3)
```

##### fitsIn()
```cpp
bool fitsIn(const Dimensions &container) const;
```

Checks if these dimensions fit within a container (both length and width must fit).

**Parameters:**
- `container` - The container dimensions to check against

**Returns:** `true` if both length and width fit, `false` otherwise

**Example:**
```cpp
Dimensions boat(28, 0, 11, 0);
Dimensions slip(30, 0, 12, 0);

if (boat.fitsIn(slip)){
    std::cout << "Boat fits in slip\n";
}
```

##### fitsInWidthOnly()
```cpp
bool fitsInWidthOnly(const Dimensions &container) const;
```

Checks if width fits within container, ignoring length. Useful for `--ignore-length` mode.

**Parameters:**
- `container` - The container dimensions to check against

**Returns:** `true` if width fits, `false` otherwise

**Example:**
```cpp
Dimensions boat(35, 0, 11, 0);  // Long boat
Dimensions slip(30, 0, 12, 0);   // Shorter slip

if (boat.fitsInWidthOnly(slip)){
    std::cout << "Boat width fits (length may overhang)\n";
}
```

##### lengthDifferenceInches()
```cpp
int lengthDifferenceInches(const Dimensions &container) const;
```

Calculates the length difference between this object and container.

**Parameters:**
- `container` - The container dimensions to compare against

**Returns:** Difference in inches (negative if this is smaller, positive if larger)

**Example:**
```cpp
Dimensions boat(32, 0, 11, 0);
Dimensions slip(30, 0, 12, 0);

int diff = boat.lengthDifferenceInches(slip);  // Returns 24 (boat is 2 feet longer)
```

---

### Slip

Represents a boat slip with maximum dimensions.

**Header:** `<slippage/slip.hpp>`

#### Constructor

```cpp
Slip(const std::string &slipId, int feetLength, int inchesLength, 
     int feetWidth, int inchesWidth);
```

Creates a Slip object with specified ID and maximum dimensions.

**Parameters:**
- `slipId` - Unique slip identifier
- `feetLength` - Maximum length in feet
- `inchesLength` - Additional inches for length (0-11)
- `feetWidth` - Maximum width in feet
- `inchesWidth` - Additional inches for width (0-11)

**Example:**
```cpp
// Slip S1: 30 feet long, 12 feet wide
Slip slip("S1", 30, 0, 12, 0);
```

#### Public Methods

##### id()
```cpp
const std::string& id() const;
```

Returns the slip's unique identifier.

**Example:**
```cpp
Slip slip("S1", 30, 0, 12, 0);
std::cout << "Slip ID: " << slip.id() << "\n";  // Prints: Slip ID: S1
```

##### maxDimensions()
```cpp
const Dimensions& maxDimensions() const;
```

Returns the slip's maximum dimensions.

**Example:**
```cpp
Slip slip("S1", 30, 0, 12, 0);
const Dimensions &dims = slip.maxDimensions();
std::cout << "Length: " << dims.lengthInches() << " inches\n";
```

##### fits()
```cpp
bool fits(const Dimensions &boatDimensions) const;
```

Checks if a boat with given dimensions fits in this slip (both length and width).

**Parameters:**
- `boatDimensions` - The boat dimensions to check

**Returns:** `true` if boat fits, `false` otherwise

**Example:**
```cpp
Slip slip("S1", 30, 0, 12, 0);
Dimensions boat(28, 0, 11, 0);

if (slip.fits(boat)){
    std::cout << "Boat fits in this slip\n";
}
```

##### fitsWidthOnly()
```cpp
bool fitsWidthOnly(const Dimensions &boatDimensions) const;
```

Checks if a boat's width fits in this slip, ignoring length.

**Parameters:**
- `boatDimensions` - The boat dimensions to check

**Returns:** `true` if boat width fits, `false` otherwise

**Example:**
```cpp
Slip slip("S1", 25, 0, 12, 0);
Dimensions longBoat(32, 0, 11, 0);

if (slip.fitsWidthOnly(longBoat)){
    std::cout << "Width fits (boat will overhang)\n";
}
```

##### lengthDifference()
```cpp
int lengthDifference(const Dimensions &boatDimensions) const;
```

Calculates the length difference between slip and boat.

**Parameters:**
- `boatDimensions` - The boat dimensions to compare

**Returns:** Difference in inches (negative if boat is smaller, positive if larger)

**Example:**
```cpp
Slip slip("S1", 30, 0, 12, 0);
Dimensions boat(32, 6, 11, 0);

int diff = slip.lengthDifference(boat);  // Positive value = boat overhangs
std::cout << "Overhang: " << (diff / 12) << " feet " << (diff % 12) << " inches\n";
```

---

### Member

Represents a marina club member with boat and dock status.

**Header:** `<slippage/member.hpp>`

#### Enumerations

##### DockStatus
```cpp
enum class DockStatus {
    PERMANENT,      // Cannot be moved or evicted
    YEAR_OFF,       // Taking year off, not assigned
    WAITING_LIST,   // High priority, can evict TEMPORARY/UNASSIGNED
    TEMPORARY,      // Standard priority, can evict UNASSIGNED
    UNASSIGNED      // Lowest priority, new member
};
```

#### Constructor

```cpp
Member(const std::string &memberId, 
       int boatFeetLength, int boatInchesLength,
       int boatFeetWidth, int boatInchesWidth,
       const std::optional<std::string> &currentSlip, 
       DockStatus dockStatus);
```

Creates a Member object with boat dimensions and status.

**Parameters:**
- `memberId` - Unique member identifier
- `boatFeetLength` - Boat length in feet
- `boatInchesLength` - Additional inches for length (0-11)
- `boatFeetWidth` - Boat width in feet
- `boatInchesWidth` - Additional inches for width (0-11)
- `currentSlip` - Current slip ID or `std::nullopt` if none
- `dockStatus` - Member's dock status

**Example:**
```cpp
// Member M1 with 28' x 11' boat, currently in slip S5, temporary status
Member member("M1", 28, 0, 11, 0, "S5", Member::DockStatus::TEMPORARY);

// New member with no current slip
Member newMember("M2", 25, 6, 10, 0, std::nullopt, Member::DockStatus::UNASSIGNED);
```

#### Public Methods

##### id()
```cpp
const std::string& id() const;
```

Returns the member's unique identifier.

**Example:**
```cpp
Member member("M1", 28, 0, 11, 0, "S5", Member::DockStatus::TEMPORARY);
std::cout << "Member ID: " << member.id() << "\n";  // Prints: Member ID: M1
```

##### boatDimensions()
```cpp
const Dimensions& boatDimensions() const;
```

Returns the member's boat dimensions.

**Example:**
```cpp
Member member("M1", 28, 6, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
const Dimensions &dims = member.boatDimensions();
std::cout << "Boat length: " << dims.lengthInches() << " inches\n";
```

##### currentSlip()
```cpp
const std::optional<std::string>& currentSlip() const;
```

Returns the member's current slip ID (if any).

**Returns:** `std::optional<std::string>` - Contains slip ID or is empty

**Example:**
```cpp
Member member("M1", 28, 0, 11, 0, "S5", Member::DockStatus::TEMPORARY);

if (member.currentSlip().has_value()){
    std::cout << "Current slip: " << member.currentSlip().value() << "\n";
}
else{
    std::cout << "No current slip\n";
}
```

##### dockStatus()
```cpp
DockStatus dockStatus() const;
```

Returns the member's dock status.

**Example:**
```cpp
Member member("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::WAITING_LIST);

if (member.dockStatus() == Member::DockStatus::WAITING_LIST){
    std::cout << "Member is on waiting list\n";
}
```

##### stringToDockStatus() [static]
```cpp
static DockStatus stringToDockStatus(const std::string &str);
```

Converts a string to DockStatus enum value.

**Parameters:**
- `str` - String representation (case-insensitive): "permanent", "year-off", "waiting-list", "temporary", "unassigned"

**Returns:** Corresponding DockStatus enum value

**Throws:** `std::invalid_argument` if string is invalid

**Example:**
```cpp
auto status = Member::stringToDockStatus("waiting-list");
// Returns Member::DockStatus::WAITING_LIST
```

##### dockStatusToString() [static]
```cpp
static std::string dockStatusToString(DockStatus status);
```

Converts DockStatus enum value to string.

**Parameters:**
- `status` - DockStatus enum value

**Returns:** String representation (lowercase with hyphens)

**Example:**
```cpp
std::string statusStr = Member::dockStatusToString(Member::DockStatus::WAITING_LIST);
// Returns "waiting-list"
```

##### Comparison Operators

```cpp
bool operator<(const Member &other) const;
bool operator>(const Member &other) const;
bool operator==(const Member &other) const;
```

Compare members based on priority (dock status first, then member ID).

**Example:**
```cpp
Member m1("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::WAITING_LIST);
Member m2("M2", 30, 0, 12, 0, std::nullopt, Member::DockStatus::TEMPORARY);

if (m1 < m2){
    std::cout << "M1 has higher priority than M2\n";
}
```

---

### Assignment

Represents the result of assigning a member to a slip (or not assigning them).

**Header:** `<slippage/assignment.hpp>`

#### Enumerations

##### Status
```cpp
enum class Status {
    PERMANENT,   // Member kept current slip (auto-upgraded)
    SAME,        // Member kept current slip (but not upgraded) - internal use
    NEW,         // Member assigned to different slip
    UNASSIGNED   // Member not assigned
};
```

#### Constructor

```cpp
Assignment(const std::string &memberId, 
           const std::string &slipId,
           Status status, 
           const Dimensions &boatDimensions,
           const Dimensions &slipDimensions, 
           Member::DockStatus dockStatus,
           const std::string &comment = "", 
           double pricePerSqFt = 0.0, 
           bool upgraded = false);
```

Creates an Assignment object representing an assignment result.

**Parameters:**
- `memberId` - Member's unique identifier
- `slipId` - Assigned slip ID (empty string if unassigned)
- `status` - Assignment status
- `boatDimensions` - Member's boat dimensions
- `slipDimensions` - Assigned slip dimensions (or default if unassigned)
- `dockStatus` - Member's original dock status
- `comment` - Optional comment (e.g., "TIGHT FIT", overhang notes)
- `pricePerSqFt` - Price per square foot (0.0 if not calculating price)
- `upgraded` - Whether member was auto-upgraded to PERMANENT

**Example:**
```cpp
Dimensions boat(28, 0, 11, 0);
Dimensions slip(30, 0, 12, 0);

Assignment assignment("M1", "S5", Assignment::Status::NEW, 
                      boat, slip, Member::DockStatus::TEMPORARY,
                      "", 2.75, false);
```

#### Public Methods

##### memberId()
```cpp
const std::string& memberId() const;
```

Returns the member ID.

##### slipId()
```cpp
const std::string& slipId() const;
```

Returns the assigned slip ID (empty string if unassigned).

##### status()
```cpp
Status status() const;
```

Returns the assignment status.

**Example:**
```cpp
if (assignment.status() == Assignment::Status::PERMANENT){
    std::cout << "Member has permanent assignment\n";
}
```

##### boatDimensions()
```cpp
const Dimensions& boatDimensions() const;
```

Returns the member's boat dimensions.

##### slipDimensions()
```cpp
const Dimensions& slipDimensions() const;
```

Returns the assigned slip's dimensions.

##### comment()
```cpp
const std::string& comment() const;
```

Returns any comment about the assignment (e.g., "TIGHT FIT", length overhang notes).

**Example:**
```cpp
if (!assignment.comment().empty()){
    std::cout << "Note: " << assignment.comment() << "\n";
}
```

##### price()
```cpp
double price() const;
```

Returns the calculated price (if price-per-sqft was set, otherwise 0.0).

**Example:**
```cpp
if (assignment.price() > 0.0){
    std::cout << "Price: $" << assignment.price() << "\n";
}
```

##### upgraded()
```cpp
bool upgraded() const;
```

Returns `true` if member was auto-upgraded to PERMANENT status.

**Example:**
```cpp
if (assignment.upgraded()){
    std::cout << "Member was upgraded to PERMANENT\n";
}
```

##### dockStatus()
```cpp
Member::DockStatus dockStatus() const;
```

Returns the member's original dock status from input.

##### assigned()
```cpp
bool assigned() const;
```

Returns `true` if member was assigned a slip, `false` if unassigned.

**Example:**
```cpp
if (assignment.assigned()){
    std::cout << "Member assigned to slip " << assignment.slipId() << "\n";
}
else{
    std::cout << "Member unassigned: " << assignment.comment() << "\n";
}
```

##### upgradeToPermament()
```cpp
void upgradeToPermament();
```

Upgrades the assignment to PERMANENT status (sets `upgraded` flag).

**Note:** This is called internally by AssignmentEngine during final phase.

##### statusToString() [static]
```cpp
static std::string statusToString(Status status);
```

Converts Status enum to string representation.

**Returns:** "PERMANENT", "SAME", "NEW", or "UNASSIGNED"

**Example:**
```cpp
std::string statusStr = Assignment::statusToString(Assignment::Status::NEW);
// Returns "NEW"
```

---

### AssignmentEngine

The core assignment algorithm engine that processes members and slips.

**Header:** `<slippage/assignment_engine.hpp>`

#### Constructor

```cpp
AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips);
```

Creates an AssignmentEngine with members and slips to process.

**Parameters:**
- `members` - Vector of Member objects (moved)
- `slips` - Vector of Slip objects (moved)

**Example:**
```cpp
std::vector<Slip> slips;
slips.emplace_back("S1", 30, 0, 12, 0);
slips.emplace_back("S2", 25, 0, 10, 0);

std::vector<Member> members;
members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, Member::DockStatus::TEMPORARY);
members.emplace_back("M2", 22, 0, 9, 0, std::nullopt, Member::DockStatus::UNASSIGNED);

AssignmentEngine engine(std::move(members), std::move(slips));
```

#### Public Methods

##### setVerbose()
```cpp
void setVerbose(bool verbose);
```

Enables or disables verbose output to stdout during assignment.

**Parameters:**
- `verbose` - `true` to enable verbose output, `false` to disable

**CLI Equivalent:** `--verbose`

**Example:**
```cpp
engine.setVerbose(true);  // Show detailed assignment progress
```

##### setIgnoreLength()
```cpp
void setIgnoreLength(bool ignoreLength);
```

Enables or disables ignore-length mode (only check width for fitting).

**Parameters:**
- `ignoreLength` - `true` to ignore length, `false` to check both dimensions

**CLI Equivalent:** `--ignore-length`

**Example:**
```cpp
engine.setIgnoreLength(true);  // Allow boats longer than slips
```

##### setPricePerSqFt()
```cpp
void setPricePerSqFt(double pricePerSqFt);
```

Sets the price per square foot for calculating rental costs.

**Parameters:**
- `pricePerSqFt` - Price per square foot (e.g., 2.75 for $2.75/sqft)

**CLI Equivalent:** `--price-per-sqft 2.75`

**Example:**
```cpp
engine.setPricePerSqFt(2.75);  // Calculate prices at $2.75/sqft
```

##### assign()
```cpp
std::vector<Assignment> assign();
```

Executes the assignment algorithm and returns results.

**Returns:** Vector of Assignment objects, one per member

**Example:**
```cpp
AssignmentEngine engine(std::move(members), std::move(slips));
engine.setVerbose(false);
engine.setIgnoreLength(false);
engine.setPricePerSqFt(0.0);

std::vector<Assignment> assignments = engine.assign();

for (const auto &assignment : assignments){
    if (assignment.assigned()){
        std::cout << assignment.memberId() << " -> " 
                  << assignment.slipId() << "\n";
    }
}
```

---

### Version

Version information API for checking library version at runtime or compile-time.

**Header:** `<slippage/version.hpp>`

#### Namespace Functions

All version functions are in the `slippage::version` namespace.

##### major()
```cpp
int major();
```

Returns the major version number.

**Returns:** Major version (e.g., 1 for version 1.0.0)

**Example:**
```cpp
if (slippage::version::major() >= 1){
    std::cout << "Using stable v1.x API\n";
}
```

##### minor()
```cpp
int minor();
```

Returns the minor version number.

**Returns:** Minor version (e.g., 0 for version 1.0.0)

##### patch()
```cpp
int patch();
```

Returns the patch version number.

**Returns:** Patch version (e.g., 0 for version 1.0.0)

##### string()
```cpp
std::string string();
```

Returns the full version string.

**Returns:** Version string (e.g., "1.0.0")

**Example:**
```cpp
std::cout << "Library version: " << slippage::version::string() << "\n";
```

##### full()
```cpp
std::string full();
```

Returns a formatted version string with program name.

**Returns:** Formatted version (e.g., "Slippage v1.0.0")

**Example:**
```cpp
std::cout << slippage::version::full() << "\n";
// Output: Slippage v1.0.0
```

#### Compile-Time Macros

For compile-time version checks:

```cpp
SLIPPAGE_VERSION_MAJOR  // Major version number
SLIPPAGE_VERSION_MINOR  // Minor version number
SLIPPAGE_VERSION_PATCH  // Patch version number
SLIPPAGE_VERSION_STRING // Full version string
SLIPPAGE_VERSION        // Alias for SLIPPAGE_VERSION_STRING
```

**Example:**
```cpp
#if SLIPPAGE_VERSION_MAJOR >= 1 && SLIPPAGE_VERSION_MINOR >= 0
    // Code for v1.0 and later
#endif

std::cout << "Compiled against: " << SLIPPAGE_VERSION_STRING << "\n";
```

---

### CsvParser

Utility class for parsing CSV files and outputting assignments.

**Header:** `<slippage/csv_parser.hpp>`

#### Static Methods

##### parseMembers()
```cpp
static std::vector<Member> parseMembers(const std::string &filename);
```

Parses a members CSV file.

**Parameters:**
- `filename` - Path to members CSV file

**Returns:** Vector of Member objects

**Throws:** `std::runtime_error` if file cannot be opened or parsed

**CSV Format:**
```csv
member_id,boat_length_ft,boat_length_in,boat_width_ft,boat_width_in,current_slip,dock_status
M1,28,0,11,0,S5,temporary
M2,25,6,10,3,,unassigned
```

**Example:**
```cpp
try{
    std::vector<Member> members = CsvParser::parseMembers("members.csv");
    std::cout << "Loaded " << members.size() << " members\n";
}
catch (const std::runtime_error &e){
    std::cerr << "Error: " << e.what() << "\n";
}
```

##### parseSlips()
```cpp
static std::vector<Slip> parseSlips(const std::string &filename);
```

Parses a slips CSV file.

**Parameters:**
- `filename` - Path to slips CSV file

**Returns:** Vector of Slip objects

**Throws:** `std::runtime_error` if file cannot be opened or parsed

**CSV Format:**
```csv
slip_id,max_length_ft,max_length_in,max_width_ft,max_width_in
S1,30,0,12,0
S2,25,6,10,6
```

**Example:**
```cpp
try{
    std::vector<Slip> slips = CsvParser::parseSlips("slips.csv");
    std::cout << "Loaded " << slips.size() << " slips\n";
}
catch (const std::runtime_error &e){
    std::cerr << "Error: " << e.what() << "\n";
}
```

##### Stream Operator

```cpp
friend std::ostream& operator<<(std::ostream &out, 
                                const std::vector<Assignment> &assignments);
```

Writes assignments to any output stream in CSV format.

**Example:**
```cpp
std::vector<Assignment> assignments = engine.assign();

// Write to stdout
std::cout << assignments;

// Write to file
std::ofstream file("assignments.csv");
file << assignments;

// Write to string stream
std::ostringstream oss;
oss << assignments;
std::string csvData = oss.str();
```

---

## Complete Usage Examples

### Example 1: Basic Assignment from CSV Files

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/csv_parser.hpp>
#include <iostream>
#include <fstream>

int main(){
    try{
        // Load data from CSV files
        auto members = CsvParser::parseMembers("members.csv");
        auto slips = CsvParser::parseSlips("slips.csv");
        
        std::cout << "Loaded " << members.size() << " members\n";
        std::cout << "Loaded " << slips.size() << " slips\n";
        
        // Create engine and run assignment
        AssignmentEngine engine(std::move(members), std::move(slips));
        auto assignments = engine.assign();
        
        // Write results to file
        std::ofstream output("assignments.csv");
        output << assignments;
        
        std::cout << "Assignments written to assignments.csv\n";
    }
    catch (const std::exception &e){
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Example 2: Programmatic Member and Slip Creation

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <iostream>

int main(){
    // Create slips
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 0, 10, 0);
    slips.emplace_back("S3", 40, 0, 15, 0);
    slips.emplace_back("S4", 20, 0, 8, 0);
    
    // Create members with various statuses
    std::vector<Member> members;
    
    // Permanent member in S1
    members.emplace_back("M1", 28, 0, 11, 0, "S1", 
                         Member::DockStatus::PERMANENT);
    
    // Waiting list member, no current slip
    members.emplace_back("M2", 24, 0, 9, 0, std::nullopt, 
                         Member::DockStatus::WAITING_LIST);
    
    // Temporary member in S2
    members.emplace_back("M3", 22, 6, 9, 6, "S2", 
                         Member::DockStatus::TEMPORARY);
    
    // Unassigned member
    members.emplace_back("M4", 38, 0, 14, 0, std::nullopt, 
                         Member::DockStatus::UNASSIGNED);
    
    // Year-off member
    members.emplace_back("M5", 19, 0, 8, 0, "S4", 
                         Member::DockStatus::YEAR_OFF);
    
    // Run assignment
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    // Process results
    for (const auto &assignment : assignments){
        std::cout << "Member " << assignment.memberId() << ": ";
        
        if (assignment.assigned()){
            std::cout << "Assigned to " << assignment.slipId();
            
            if (assignment.status() == Assignment::Status::PERMANENT){
                std::cout << " (PERMANENT)";
            }
            else if (assignment.status() == Assignment::Status::NEW){
                std::cout << " (NEW)";
            }
            
            if (assignment.upgraded()){
                std::cout << " [UPGRADED]";
            }
        }
        else{
            std::cout << "UNASSIGNED - " << assignment.comment();
        }
        
        std::cout << "\n";
    }
    
    return 0;
}
```

### Example 3: Advanced Options with Price Calculation

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <iostream>
#include <iomanip>

int main(){
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 6, 10, 6);
    slips.emplace_back("S3", 35, 0, 14, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, 
                         Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 32, 0, 12, 6, std::nullopt, 
                         Member::DockStatus::WAITING_LIST);
    members.emplace_back("M3", 22, 0, 9, 0, std::nullopt, 
                         Member::DockStatus::UNASSIGNED);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    
    // Enable ignore-length mode and price calculation
    engine.setIgnoreLength(true);
    engine.setPricePerSqFt(3.50);  // $3.50 per square foot
    engine.setVerbose(true);        // Show assignment details
    
    auto assignments = engine.assign();
    
    std::cout << "\n=== ASSIGNMENT SUMMARY ===\n\n";
    
    double totalRevenue = 0.0;
    
    for (const auto &assignment : assignments){
        if (assignment.assigned()){
            std::cout << assignment.memberId() 
                      << " -> " << assignment.slipId();
            
            // Calculate dimensions
            int boatLen = assignment.boatDimensions().lengthInches();
            int boatWid = assignment.boatDimensions().widthInches();
            int slipLen = assignment.slipDimensions().lengthInches();
            int slipWid = assignment.slipDimensions().widthInches();
            
            std::cout << "\n  Boat: " << (boatLen / 12) << "' " 
                      << (boatLen % 12) << "\" x " 
                      << (boatWid / 12) << "' " 
                      << (boatWid % 12) << "\"\n";
            
            std::cout << "  Slip: " << (slipLen / 12) << "' " 
                      << (slipLen % 12) << "\" x " 
                      << (slipWid / 12) << "' " 
                      << (slipWid % 12) << "\"\n";
            
            std::cout << "  Price: $" << std::fixed 
                      << std::setprecision(2) << assignment.price() << "\n";
            
            if (!assignment.comment().empty()){
                std::cout << "  Note: " << assignment.comment() << "\n";
            }
            
            std::cout << "\n";
            totalRevenue += assignment.price();
        }
    }
    
    std::cout << "Total Revenue: $" << std::fixed 
              << std::setprecision(2) << totalRevenue << "\n";
    
    return 0;
}
```

### Example 4: Custom Priority Handling

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <algorithm>
#include <iostream>

int main(){
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 12, 0);
    slips.emplace_back("S2", 25, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M100", 28, 0, 11, 0, std::nullopt, 
                         Member::DockStatus::WAITING_LIST);
    members.emplace_back("M1", 24, 0, 9, 0, std::nullopt, 
                         Member::DockStatus::TEMPORARY);
    members.emplace_back("M50", 22, 0, 8, 0, std::nullopt, 
                         Member::DockStatus::UNASSIGNED);
    
    // Sort members by priority before assignment (optional, engine does this internally)
    std::sort(members.begin(), members.end(), 
              [](const Member &a, const Member &b){
                  return a < b;  // Uses Member's operator<
              });
    
    std::cout << "Priority order:\n";
    for (const auto &member : members){
        std::cout << "  " << member.id() 
                  << " (" << Member::dockStatusToString(member.dockStatus()) 
                  << ")\n";
    }
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    std::cout << "\nAssignment results:\n";
    for (const auto &assignment : assignments){
        std::cout << "  " << assignment.memberId();
        
        if (assignment.assigned()){
            std::cout << " -> " << assignment.slipId();
        }
        else{
            std::cout << " -> UNASSIGNED";
        }
        
        std::cout << "\n";
    }
    
    return 0;
}
```

### Example 5: Dimension Calculations and Tight Fits

```cpp
#include <slippage/assignment_engine.hpp>
#include <slippage/member.hpp>
#include <slippage/slip.hpp>
#include <slippage/dimensions.hpp>
#include <iostream>

int main(){
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 11, 4);   // Narrow slip
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    // Boat that will be a tight fit (< 6" width clearance)
    members.emplace_back("M1", 28, 0, 11, 0, std::nullopt, 
                         Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 22, 0, 9, 6, std::nullopt, 
                         Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    for (const auto &assignment : assignments){
        if (assignment.assigned()){
            const auto &boat = assignment.boatDimensions();
            const auto &slip = assignment.slipDimensions();
            
            int widthClearance = slip.widthInches() - boat.widthInches();
            int lengthClearance = slip.lengthInches() - boat.lengthInches();
            
            std::cout << assignment.memberId() 
                      << " in " << assignment.slipId() << ":\n";
            std::cout << "  Width clearance: " << widthClearance << " inches";
            
            if (widthClearance < 6){
                std::cout << " (TIGHT FIT!)";
            }
            
            std::cout << "\n";
            std::cout << "  Length clearance: " << lengthClearance << " inches\n";
            
            if (!assignment.comment().empty()){
                std::cout << "  Comment: " << assignment.comment() << "\n";
            }
            
            std::cout << "\n";
        }
    }
    
    return 0;
}
```

---

## Thread Safety

The Slippage library is **not thread-safe**. Each `AssignmentEngine` instance should be used from a single thread. If you need to run multiple assignments concurrently, create separate engine instances for each thread.

## Error Handling

- CSV parsing methods throw `std::runtime_error` on file or parse errors
- `Member::stringToDockStatus()` throws `std::invalid_argument` for invalid status strings
- All other methods use standard C++ exception handling conventions

## Performance Considerations

- The assignment algorithm complexity is O(n²) in worst case due to iterative eviction
- For large datasets (1000+ members), consider batch processing
- Move semantics are used throughout to minimize copying
- All dimensions are stored as integers (inches) for fast comparison

---

## See Also

- `examples/example_usage.cpp` - Practical examples demonstrating all CLI options
- `examples/README.md` - Quick reference guide
- `ASSIGNMENT_RULES.md` - Detailed algorithm documentation
- `README.md` - User-facing documentation and file formats
