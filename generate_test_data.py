#!/usr/bin/env python3
"""
Generate complex test data for Slippage assignment system.
Creates 200 slips and 225 members with realistic distribution and conflicts.
"""

import csv
import random

# Slip size distribution (length in feet, width in feet)
# Realistic marina distribution: more medium slips, fewer very large/small
SLIP_SIZES = [
    (18, 7),   # Small - 15 slips
    (20, 8),   # Small-Medium - 30 slips
    (22, 9),   # Medium - 40 slips
    (25, 10),  # Medium-Large - 50 slips
    (28, 11),  # Large - 35 slips
    (30, 12),  # Large - 20 slips
    (35, 14),  # Very Large - 8 slips
    (40, 16),  # Extra Large - 2 slips
]

def generate_slips(filename, num_slips=200):
    """Generate slip data with realistic size distribution."""
    slips = []
    slip_id = 1
    
    # Calculate how many of each size
    total_weight = sum(count for _, count in [
        (SLIP_SIZES[0], 15),
        (SLIP_SIZES[1], 30),
        (SLIP_SIZES[2], 40),
        (SLIP_SIZES[3], 50),
        (SLIP_SIZES[4], 35),
        (SLIP_SIZES[5], 20),
        (SLIP_SIZES[6], 8),
        (SLIP_SIZES[7], 2),
    ])
    
    distribution = [
        (SLIP_SIZES[0], 15),
        (SLIP_SIZES[1], 30),
        (SLIP_SIZES[2], 40),
        (SLIP_SIZES[3], 50),
        (SLIP_SIZES[4], 35),
        (SLIP_SIZES[5], 20),
        (SLIP_SIZES[6], 8),
        (SLIP_SIZES[7], 2),
    ]
    
    for (length_ft, width_ft), count in distribution:
        for _ in range(count):
            if slip_id > num_slips:
                break
            # Add some variation in inches
            length_in = random.choice([0, 0, 0, 6])  # Mostly even feet
            width_in = random.choice([0, 0, 0, 0, 6])  # Mostly even feet
            
            slips.append({
                'slip_id': f'S{slip_id:03d}',
                'max_length_ft': length_ft,
                'max_length_in': length_in,
                'max_width_ft': width_ft,
                'max_width_in': width_in,
            })
            slip_id += 1
    
    # Shuffle to mix sizes
    random.shuffle(slips)
    
    with open(filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['slip_id', 'max_length_ft', 'max_length_in', 
                                                 'max_width_ft', 'max_width_in'])
        writer.writeheader()
        writer.writerows(slips)
    
    return slips

def generate_members(filename, slips, num_members=225):
    """Generate member data with realistic boat sizes and conflicts."""
    members = []
    
    # 10% permanent members
    num_permanent = int(num_members * 0.10)
    
    # Assign roughly 75% of members to existing slips initially
    num_with_slips = int(num_members * 0.75)
    available_slips = [s['slip_id'] for s in slips]
    random.shuffle(available_slips)
    
    for member_id in range(1, num_members + 1):
        # Determine if permanent
        is_permanent = 1 if member_id <= num_permanent else 0
        
        # Boat size - slightly smaller than slips on average
        # This creates competition for slips
        base_sizes = [
            (17, 6),   # 25 boats
            (19, 7),   # 35 boats
            (21, 8),   # 40 boats
            (24, 9),   # 50 boats
            (27, 10),  # 40 boats
            (29, 11),  # 25 boats
            (34, 13),  # 8 boats
            (39, 15),  # 2 boats
        ]
        
        # Weight distribution
        weights = [25, 35, 40, 50, 40, 25, 8, 2]
        boat_base = random.choices(base_sizes, weights=weights, k=1)[0]
        
        # Add variation
        length_ft = boat_base[0] + random.choice([-1, 0, 0, 1])
        width_ft = boat_base[1] + random.choice([-1, 0, 0, 0])
        length_in = random.choice([0, 0, 0, 6])
        width_in = random.choice([0, 0, 0, 6])
        
        # Ensure reasonable minimums
        length_ft = max(15, length_ft)
        width_ft = max(6, width_ft)
        
        # Assign current slip
        current_slip = ''
        if member_id <= num_with_slips and available_slips:
            current_slip = available_slips.pop(0)
        
        members.append({
            'member_id': f'M{member_id:03d}',
            'boat_length_ft': length_ft,
            'boat_length_in': length_in,
            'boat_width_ft': width_ft,
            'boat_width_in': width_in,
            'current_slip': current_slip,
            'is_permanent': is_permanent,
        })
    
    with open(filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['member_id', 'boat_length_ft', 'boat_length_in',
                                                 'boat_width_ft', 'boat_width_in', 
                                                 'current_slip', 'is_permanent'])
        writer.writeheader()
        writer.writerows(members)
    
    return members

if __name__ == '__main__':
    random.seed(42)  # Reproducible results
    
    print("Generating 200 slips...")
    slips = generate_slips('complex_test_slips.csv', 200)
    print(f"Generated {len(slips)} slips")
    
    print("\nGenerating 225 members...")
    members = generate_members('complex_test_members.csv', slips, 225)
    print(f"Generated {len(members)} members")
    
    # Print statistics
    permanent_count = sum(1 for m in members if m['is_permanent'] == 1)
    with_slips = sum(1 for m in members if m['current_slip'])
    without_slips = len(members) - with_slips
    
    print(f"\nStatistics:")
    print(f"  Permanent members: {permanent_count}")
    print(f"  Members with current slips: {with_slips}")
    print(f"  Members without slips: {without_slips}")
    print(f"  Total members: {len(members)}")
    print(f"  Total slips: {len(slips)}")
    print(f"  Competition: {len(members)} members for {len(slips)} slips")
    print(f"  Expected unassigned: ~{len(members) - len(slips)}")
