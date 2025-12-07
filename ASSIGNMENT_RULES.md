# Boat Slip Assignment Rules

This document explains how the Slippage system assigns boat slips to club members.

## Overview

The slip assignment system allocates boat slips to members based on:
- Dock status (five-level priority: permanent, year-off, waiting-list, temporary, unassigned)
- Member ID (within each dock status level)
- Boat and slip dimensions (boats must fit in slips)
- Current slip occupancy

The system aims to keep members in their current slips when possible, while ensuring higher-priority members get slips before lower-priority ones. Members who keep their current slip are automatically upgraded to permanent status.

## Priority System

**Member priority is determined by two factors:**

### 1. Dock Status (Primary Priority)
Members are processed in this order:
1. **PERMANENT**: Highest priority, cannot be moved or evicted
2. **WAITING_LIST**: Can evict TEMPORARY and UNASSIGNED members
3. **TEMPORARY**: Can evict UNASSIGNED members
4. **UNASSIGNED**: Lowest priority, cannot evict anyone
5. **YEAR_OFF**: Not assigned (special case)

### 2. Member ID (Tie-Breaker Within Status)
Within each dock status level:
- Lower member IDs have **higher priority**
- Member "M1" has higher priority than "M2"
- Member "100" has higher priority than "200"

Priority determines who gets preference when multiple members want the same slip.

## Assignment Statuses

Each assignment results in one of these statuses:

 Status | Meaning |
--------|---------|
 **PERMANENT** | Member has a permanent assignment (either original or auto-upgraded) |
 **TEMPORARY** | Member is assigned to a different slip than before |
 **UNASSIGNED** | Member did not receive a slip assignment |

**Note:** Members who keep their current slip are automatically upgraded to PERMANENT status. The `upgraded` field indicates whether this auto-upgrade occurred.

## Core Assignment Rules

### Rule 1: Dock Status Determines Priority

**Members are processed in dock status order: PERMANENT → YEAR_OFF → WAITING_LIST → TEMPORARY → UNASSIGNED**

- **PERMANENT**: Locked to their current slip, cannot be moved or evicted by anyone
- **YEAR_OFF**: Not assigned (slip left empty), processed but not given a slip
- **WAITING_LIST**: High priority, can evict TEMPORARY and UNASSIGNED members
- **TEMPORARY**: Standard priority, can evict UNASSIGNED members
- **UNASSIGNED**: Lowest priority, cannot evict anyone

Within each dock status level, members with lower IDs have higher priority.

**Example:**
```
Member M100 (permanent) is in Slip S5
Member M1 (waiting-list) wants Slip S5
→ M100 keeps S5, M1 must find another slip (cannot evict permanent)

Member M2 (temporary) is in Slip S3  
Member M1 (waiting-list) wants Slip S3
→ M1 evicts M2 and takes S3 (waiting-list > temporary)
→ System tries to reassign M2 to another slip
```

### Rule 2: Within Same Dock Status, Member ID Determines Priority

**When members have the same dock status, lower ID wins.**

- Members with the same dock status are processed in ID order (ascending)
- Higher-priority members (lower ID) can evict lower-priority members (higher ID)
- Evicted members are automatically reconsidered for other available slips

**Example:**
```
Member M3 (temporary) currently has Slip S1
Member M1 (temporary) wants Slip S1
→ M1 evicts M3 and takes S1 (M1 < M3)
→ System tries to reassign M3 to another slip
```

### Rule 3: Year-Off Members Are Not Assigned

**Members with year-off status do not receive slip assignments.**

- Their slip becomes available for other members
- They appear in the output with status UNASSIGNED and comment "Year off - not assigned"
- The slip field is empty in the output

**Example:**
```
Member M5 (year-off) was in Slip S2
→ M5 does not get assigned
→ S2 becomes available for other members
```

### Rule 4: Members Prefer Their Current Slip

**The system first tries to keep each member in their current slip.**

- If a member's current slip is available and fits their boat, they keep it
- This preference applies only if no higher-priority member wants that slip
- Keeping members in their current slips minimizes disruption

**Example:**
```
Member M5 is in Slip S2
Slip S2 fits M5's boat
No higher-priority member wants S2
→ M5 stays in S2 (status: SAME)
```

### Rule 5: Size Determines Eligibility

**A boat can only be assigned to a slip if the boat fits within the slip's dimensions.**

- By default, both length and width must fit
- A 20' × 10' boat cannot fit in a 20' × 8' slip (width too small)
- A 25' × 10' boat cannot fit in a 20' × 10' slip (length too large)

**Ignore-Length Mode (`--ignore-length`):**
When this flag is enabled, only the boat width is checked:
- Boats can be longer than the slip length
- Width must still fit within the slip's maximum width
- Length differences are shown in the comment field (e.g., "NOTE: boat is 3' 6\" longer than slip")
- Useful for marinas where boats can extend beyond slip boundaries

**Size-Based Protection:**
If a lower-priority member occupies a small slip that a higher-priority member's larger boat cannot fit into, the lower-priority member keeps that slip.

**Example:**
```
Member M2 (small boat: 14' × 7') is in Slip S1 (15' × 8')
Member M1 (large boat: 22' × 10') wants a slip
→ M1 cannot evict M2 because M1's boat won't fit in S1
→ M2 keeps S1 due to size protection
```

### Rule 6: Best Fit Selection

**When finding a new slip, the system chooses the best fitting slip based on the current mode:**

**Normal Mode:**
- Selects the smallest slip that fits the boat
- Minimizes wasted space
- Calculated by total slip area (length × width)
- Helps ensure larger slips remain available for larger boats

**Ignore-Length Mode (`--ignore-length`):**
- **First priority**: Minimize boat overhang (amount boat is longer than slip)
- **Second priority**: Among slips with equal overhang, select smallest by area
- This ensures boats extend beyond slips as little as possible

**Examples:**

*Normal mode:*
```
Available slips: S1 (20' × 10'), S2 (30' × 15'), S3 (25' × 12')
Member's boat: 18' × 8'
→ System assigns S1 (smallest fitting slip by area)
```

*Ignore-length mode:*
```
Available slips: S1 (20' × 10'), S2 (28' × 10'), S3 (25' × 10')
Member's boat: 27' × 8'
→ System assigns S2 (0 overhang, boat is 1' shorter than slip)
→ Not S3 (2' overhang) or S1 (7' overhang)
```

### Rule 7: Eviction and Reassignment

**When a member is evicted, they are automatically reconsidered for other slips.**

The system uses an iterative process:
1. Process all members in priority order
2. When a higher-priority member evicts someone, mark that person as unassigned
3. Restart the process, considering all unassigned members again
4. Continue until no more changes occur

This ensures evicted members get reassigned to alternative slips when available.

**Example:**
```
Initial state:
- M3 in S1
- M2 in S2
- S3 available

Iteration 1:
- M1 wants S1, evicts M3
- M2 keeps S2
- M3 is unassigned

Iteration 2:
- M1 stays in S1
- M2 stays in S2
- M3 gets S3 (previously available)

Final result: All three members have slips
```

### Rule 8: Tight Fit Warnings

**Assignments with less than 6 inches of width clearance are flagged with a "TIGHT FIT" warning.**

- When a boat's width is within 6 inches of the slip's width, a "TIGHT FIT" note appears in the comment field
- This alerts marina staff to assignments with minimal width clearance
- Can be combined with other notes (e.g., "NOTE: boat is 5' longer than slip; TIGHT FIT")

### Rule 9: Price Calculation (Optional)

**With `--price-per-sqft` flag, calculates dockage price based on slip/boat area.**

- Price = max(boat area, slip area) × price per square foot
- Uses the larger of boat or slip area to ensure fair pricing
- Rounded to 2 decimal places
- Unassigned members have a price of 0.0

### Rule 10: Automatic Status Upgrade

**Members who keep their current slip are automatically upgraded to PERMANENT status.**

- Auto-upgrade happens for all members who retain their slip
- Upgrade occurs after all assignments are complete
- The `upgraded` field is set to `true` for auto-upgraded members
- Already-permanent members have `upgraded=false` (they weren't upgraded, already were permanent)
- This rewards member retention and provides stability

### Rule 11: Unassigned Members Appear in Output

**If no available slip fits a member's boat, they appear in the output with UNASSIGNED status.**

- The member appears with an empty slip ID and status `UNASSIGNED`
- This can happen when:
  - All slips are too small for the boat
  - All fitting slips are occupied by permanent or higher-priority members
  - The member was evicted and no alternative slip fits
- Unassigned members are clearly identified in the output for reporting purposes

## Assignment Process Flow

The system follows this process:

```
1. Parse Input
   ├─ Read members CSV (boat dimensions, current slips, dock_status)
   └─ Read slips CSV (slip dimensions)

2. Phase 1: Assign Permanent Members
   └─ Lock PERMANENT members in their current slips (cannot be moved)

3. Phase 2: Process Year-Off Members
   └─ Mark YEAR_OFF members as unassigned, free up their slips

4. Phase 3: Process Waiting-List Members (Iteratively)
   ├─ Sort by member ID (ascending)
   ├─ For each unassigned member:
   │  ├─ Try to keep in current slip
   │  │  ├─ If available and fits → assign
   │  │  └─ If occupied by lower-priority member → evict and assign
   │  └─ If current slip unavailable:
   │     ├─ Find best fitting available slip
   │     ├─ If occupied by lower-priority member → evict and assign
   │     └─ If no slip found → leave unassigned
   └─ If any evictions occurred, repeat iteration

5. Phase 4: Process Temporary Members (Iteratively)
   └─ Same process as Phase 3

6. Phase 5: Process Unassigned Members (Iteratively)
   └─ Same process as Phase 3

7. Auto-Upgrade
   └─ Members who kept their current slip → upgraded to PERMANENT

8. Generate Output
   └─ Write assignments to CSV with status and dock_status
```

## Examples

### Example 1: Simple Priority-Based Assignment

**Input:**
```
Slips:
- S1: 20' × 10'

Members:
- M2: 18' × 8' boat, currently in S1
- M1: 18' × 8' boat, wants S1
```

**Process:**
1. M1 processes first (higher priority)
2. M1 wants S1, which is occupied by M2
3. M1 evicts M2 and takes S1
4. M2 has no other slip available

**Output:**
```
M1, S1, SAME  (M1 got their desired slip)
(M2 not assigned)
```

### Example 2: Eviction with Reassignment

**Input:**
```
Slips:
- S1: 20' × 10'
- S2: 20' × 10'

Members:
- M2: 18' × 8' boat, currently in S1
- M1: 18' × 8' boat, wants S1
```

**Process:**
1. M1 evicts M2 from S1
2. M2 is reconsidered and assigned to S2

**Output:**
```
M1, S1, SAME
M2, S2, TEMPORARY
```

### Example 3: Permanent Member Protection

**Input:**
```
Slips:
- S1: 20' × 10'
- S2: 25' × 12'

Members:
- M5: 18' × 8' boat, currently in S1, PERMANENT
- M1: 18' × 8' boat, wants S1
```

**Process:**
1. M5 is permanent and keeps S1
2. M1 cannot evict M5
3. M1 gets S2 instead

**Output:**
```
M5, S1, PERMANENT
M1, S2, TEMPORARY
```

### Example 4: Size-Based Protection

**Input:**
```
Slips:
- S1: 15' × 8' (small)
- S2: 25' × 12' (large)

Members:
- M2: 14' × 7' boat, currently in S1
- M1: 22' × 10' boat, wants any slip
```

**Process:**
1. M1 cannot evict M2 from S1 (boat too large for S1)
2. M1 gets S2
3. M2 keeps S1

**Output:**
```
M1, S2, TEMPORARY
M2, S1, SAME
```

## Frequently Asked Questions

**Q: What happens if my boat is too large for all slips?**  
A: You will not receive a slip assignment. The system will not assign boats to slips they don't fit in. Note: If using `--ignore-length`, boats only need to fit by width.

**Q: What does the `--ignore-length` flag do?**  
A: With `--ignore-length`, the system only checks if the boat width fits the slip, ignoring length. Boats can be longer than slips. The comment field will show how much longer or shorter the boat is (e.g., "NOTE: boat is 2' 3\" longer than slip").

**Q: Can a permanent member be moved to a different slip?**  
A: No. Permanent members always keep their designated slip and cannot be moved or evicted by anyone.

**Q: If I'm evicted, will I definitely get another slip?**  
A: Not necessarily. You'll only get reassigned if there's another available slip that fits your boat and isn't needed by someone with higher priority.

**Q: How is "smallest fitting slip" determined?**  
A: In normal mode, by total area (length × width). A 20' × 10' slip (200 sq ft) is smaller than a 25' × 12' slip (300 sq ft). In `--ignore-length` mode, the system first minimizes boat overhang (how much longer the boat is than the slip), then uses area as a tiebreaker.

**Q: Can I request a specific slip?**  
A: The system uses your "current slip" field as your preferred slip. If you're already in a slip, the system will try to keep you there.

**Q: What determines my priority level?**  
A: Two factors: (1) Your dock status (permanent > waiting-list > temporary > unassigned), and (2) Your member ID within your dock status level (lower IDs = higher priority).

## Technical Notes

- All dimensions are stored internally as total inches for precise comparison
- The assignment algorithm runs iteratively until no more changes occur
- Member comparison uses the `<` operator on member IDs (string comparison)
- The system guarantees that all permanent members are satisfied before processing non-permanent members
