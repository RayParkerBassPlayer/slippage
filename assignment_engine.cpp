#include "assignment_engine.h"
#include <algorithm>
#include <limits>
#include <iostream>

AssignmentEngine::AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips)
    : mMembers(std::move(members)), mSlips(std::move(slips)), mVerbose(false) {
}

// Main assignment algorithm entry point.
// Strategy: Two-phase assignment process
// Phase 1: Lock in permanent member assignments (cannot be evicted)
// Phase 2: Iteratively assign non-permanent members with eviction support
std::vector<Assignment> AssignmentEngine::assign() {
    std::vector<Assignment> assignments;
    
    assignPermanentMembers(assignments);
    assignRemainingMembers(assignments);
    
    if (mVerbose) {
        printStatistics(assignments);
    }
    
    return assignments;
}

// Phase 1: Assign permanent members to their designated slips.
// 
// Permanent members have guaranteed assignments that cannot be evicted by
// anyone, regardless of priority. This is the first phase to ensure these
// critical assignments are locked in before processing regular members.
//
// Key behaviors:
// - Permanent members are assigned regardless of whether their boat fits
// - A warning comment is added if the boat exceeds slip dimensions
// - The slip is marked as occupied and unavailable for other members
// - If a permanent member has no current slip, they are skipped
void AssignmentEngine::assignPermanentMembers(std::vector<Assignment> &assignments) {
    if (mVerbose)
    {
        std::cout << "\n===== PHASE 1: Permanent Member Assignments =====\n";
    }
    
    for (auto &member : mMembers) {
        // Skip non-permanent members - they're handled in phase 2
        if (!member.isPermanent()) {
            continue;
        }

        // Permanent members without a designated slip cannot be assigned
        if (!member.currentSlip().has_value()) {
            continue;
        }

        const std::string &slipId = member.currentSlip().value();
        Slip *slip = findSlipById(slipId);

        if (slip) {
            // Mark this slip as occupied by this permanent member
            // This prevents any other member from taking it
            assignMemberToSlip(&member, slipId);
            std::string comment = "";

            // Check if boat actually fits - add note if not
            // Note: We still assign it since it's permanent, but flag the issue
            if (!slipFits(slip, member.boatDimensions())) {
                comment = "NOTE: Boat does not fit in assigned slip";
            }
            
            // Add length difference comment if ignoring length
            std::string lengthComment = generateLengthComment(slip, member.boatDimensions());
            if (!lengthComment.empty()) {
                if (!comment.empty()) {
                    comment += "; " + lengthComment;
                } else {
                    comment = lengthComment;
                }
            }

            assignments.emplace_back(member.id(), slipId, 
                                    Assignment::Status::PERMANENT, 
                                    member.boatDimensions(), comment);
            
            if (mVerbose)
            {
                std::cout << "  Member " << member.id() << " -> Slip " << slipId << " (PERMANENT)";
                
                if (!comment.empty())
                {
                    std::cout << " [" << comment << "]";
                }
                
                std::cout << "\n";
            }
        }
    }
}

// Phase 2: Assign non-permanent members with iterative eviction support.
//
// This is the core assignment algorithm that handles priority-based assignment
// with eviction and reassignment. The algorithm runs iteratively until no more
// changes occur, ensuring evicted members are reconsidered for other slips.
//
// Algorithm overview:
// 1. Sort members by priority (lower ID = higher priority)
// 2. Process each unassigned member in priority order
// 3. Try to assign them to their preferred slip or find best alternative
// 4. If slip is occupied by lower-priority member, evict them
// 5. Repeat until no evictions occur (stable state reached)
// 6. Add all assigned members to output
// 7. Add all unassigned members to output with UNASSIGNED status
void AssignmentEngine::assignRemainingMembers(std::vector<Assignment> &assignments) {
    // Build list of assignable (non-permanent) members
    std::vector<Member *> assignableMembers;

    for (auto &member : mMembers) {
        if (!member.isPermanent()) {
            assignableMembers.push_back(&member);
        }
    }

    // Sort by priority: lower member ID = higher priority
    // This ensures higher-priority members are processed first and can
    // evict lower-priority members from desired slips
    std::sort(assignableMembers.begin(), assignableMembers.end(),
              [](const Member *a, const Member *b) { return *a < *b; });

    // Iterative assignment loop
    // Keep processing until no changes occur (no evictions)
    // This ensures evicted members get reconsidered for alternative slips
    bool changesMade = true;
    int passNumber = 1;
    
    if (mVerbose)
    {
        std::cout << "\n===== PHASE 2: Iterative Assignment =====\n";
    }

    while (changesMade) {
        changesMade = false;
        
        if (mVerbose)
        {
            std::cout << "\n--- Pass " << passNumber << " ---\n";
        }

        // Process each member in priority order
        for (Member *member : assignableMembers) {
            // Skip members who are already assigned
            // They've found their slip and won't be evicted
            if (isMemberAssigned(member)) {
                continue;
            }

            std::string assignedSlipId;

            // STEP 1: Try to assign member to their current/preferred slip
            // This minimizes disruption by keeping members where they are
            if (member->currentSlip().has_value()) {
                const std::string &currentSlipId = member->currentSlip().value();
                Slip *currentSlip = findSlipById(currentSlipId);

                // Check if current slip exists and boat fits
                if (currentSlip && slipFits(currentSlip, member->boatDimensions())) {
                    auto occupantIt = mSlipOccupant.find(currentSlipId);

                    // Case 1: Slip is available (not occupied)
                    if (occupantIt == mSlipOccupant.end()) {
                        assignedSlipId = currentSlipId;
                    }
                    // Case 2: Slip is occupied by lower-priority member
                    // Evict them if: (a) not permanent, and (b) we have higher priority
                    else if (!occupantIt->second->isPermanent() && *member < *occupantIt->second) {
                        // Evict the lower-priority member
                        // They'll be reconsidered in the next iteration
                        unassignMember(occupantIt->second);
                        assignedSlipId = currentSlipId;
                        changesMade = true;  // Signal need for another iteration
                    }
                    // Case 3: Slip occupied by permanent or higher-priority member
                    // Cannot evict them - will try to find alternative slip below
                }
            }

            // STEP 2: Find best alternative slip if current slip unavailable
            // "Best" = smallest slip that fits the boat (minimizes waste)
            if (assignedSlipId.empty()) {
                // Exclude current slip from search to avoid trying it again
                std::string excludeSlip = member->currentSlip().value_or("");
                Slip *bestSlip = findBestAvailableSlip(member->boatDimensions(), excludeSlip);

                if (bestSlip) {
                    auto occupantIt = mSlipOccupant.find(bestSlip->id());

                    // If slip is occupied, try to evict if we have higher priority
                    // Note: Permanent members cannot be evicted
                    if (occupantIt != mSlipOccupant.end() && 
                        !occupantIt->second->isPermanent() && 
                        *member < *occupantIt->second) {
                        unassignMember(occupantIt->second);
                        changesMade = true;
                    }

                    // After potential eviction, check if slip is now available
                    if (mSlipOccupant.find(bestSlip->id()) == mSlipOccupant.end()) {
                        assignedSlipId = bestSlip->id();
                    }
                }
            }

            // STEP 3: Assign member to slip if one was found
            if (!assignedSlipId.empty()) {
                assignMemberToSlip(member, assignedSlipId);
                
                if (mVerbose)
                {
                    std::cout << "  Member " << member->id() << " -> Slip " << assignedSlipId;
                    
                    if (member->currentSlip().has_value() && member->currentSlip().value() == assignedSlipId)
                    {
                        std::cout << " (keeping current)";
                    }
                    else
                    {
                        std::cout << " (new assignment)";
                    }
                    
                    std::cout << "\n";
                }
            }
            // If no slip found, member remains unassigned and will be
            // added to output with UNASSIGNED status later
        }
        
        passNumber++;
    }
    // End of iterative loop - stable assignment state reached
    
    if (mVerbose)
    {
        std::cout << "\nAssignment complete after " << (passNumber - 1) << " pass(es)\n";
    }

    // STEP 4: Generate output for all assigned members
    // Determine if they kept their slip (SAME) or got a new one (NEW)
    for (const auto &entry : mMemberAssignment) {
        const Member *member = entry.first;
        const std::string &slipId = entry.second;

        // Skip permanent members - already added to output in phase 1
        if (member->isPermanent()) {
            continue;
        }

        // Determine status: SAME if kept current slip, NEW otherwise
        Assignment::Status status = Assignment::Status::NEW;
        if (member->currentSlip().has_value() && member->currentSlip().value() == slipId) {
            status = Assignment::Status::SAME;
        }
        
        // Add length difference comment if ignoring length
        Slip *assignedSlip = findSlipById(slipId);
        std::string lengthComment = "";
        if (assignedSlip) {
            lengthComment = generateLengthComment(assignedSlip, member->boatDimensions());
        }

        assignments.emplace_back(member->id(), slipId, status, member->boatDimensions(), lengthComment);
    }

    // STEP 5: Generate output for all unassigned members
    // These members couldn't be assigned due to:
    // - Boat too large for all slips
    // - All suitable slips occupied by permanent or higher-priority members
    // - Evicted and no alternative slip found
    for (const auto &member : assignableMembers) {
        if (!isMemberAssigned(member)) {
            std::string comment = generateUnassignedComment(member);
            assignments.emplace_back(member->id(), "", Assignment::Status::UNASSIGNED, member->boatDimensions(), comment);
        }
    }
}

// Find a slip by its ID.
// Returns pointer to slip if found, nullptr otherwise.
Slip *AssignmentEngine::findSlipById(const std::string &slipId) const {
    for (const auto &slip : mSlips) {
        if (slip.id() == slipId) {
            return const_cast<Slip *>(&slip);
        }
    }
    return nullptr;
}

// Find a member by their ID.
// Returns pointer to member if found, nullptr otherwise.
Member *AssignmentEngine::findMemberById(const std::string &memberId) {
    for (auto &member : mMembers) {
        if (member.id() == memberId) {
            return &member;
        }
    }
    return nullptr;
}

// Assign a member to a slip.
// Updates both the slip occupancy map (slip -> member) and
// member assignment map (member -> slip) to maintain bidirectional tracking.
void AssignmentEngine::assignMemberToSlip(const Member *member, const std::string &slipId) {
    mSlipOccupant[slipId] = member;
    mMemberAssignment[member] = slipId;
}

// Unassign a member from their current slip.
// Removes them from both tracking maps, freeing up the slip for others.
// This is used during eviction - the member will be reconsidered for
// assignment in subsequent iterations.
void AssignmentEngine::unassignMember(const Member *member) {
    auto it = mMemberAssignment.find(member);
    if (it != mMemberAssignment.end()) {
        mSlipOccupant.erase(it->second);
        mMemberAssignment.erase(it);
    }
}

// Check if a member has been assigned to a slip.
// Returns true if member is currently assigned, false otherwise.
bool AssignmentEngine::isMemberAssigned(const Member *member) const {
    return mMemberAssignment.find(member) != mMemberAssignment.end();
}

// Generate a diagnostic comment explaining why a member wasn't assigned.
// Provides specific reasons to help understand assignment failures.
std::string AssignmentEngine::generateUnassignedComment(const Member *member) const {
    // Check if member had a current slip
    bool hadCurrentSlip = member->currentSlip().has_value();
    
    // Check if any slip can fit the boat
    bool anySlipFits = false;
    int fittingSlipCount = 0;
    
    for (const auto &slip : mSlips)
    {
        if (slipFits(&slip, member->boatDimensions()))
        {
            anySlipFits = true;
            fittingSlipCount++;
        }
    }
    
    if (!anySlipFits)
    {
        if (hadCurrentSlip)
        {
            return "Evicted - boat too large for all available slips";
        }
        
        return "Boat too large for all available slips";
    }
    
    // Boat fits in some slips, check current slip status
    if (hadCurrentSlip)
    {
        const std::string &currentSlipId = member->currentSlip().value();
        Slip *currentSlip = findSlipById(currentSlipId);
        
        if (!currentSlip)
        {
            return "Evicted - previous slip no longer exists";
        }
        
        if (!slipFits(currentSlip, member->boatDimensions()))
        {
            if (fittingSlipCount > 0)
            {
                return "Evicted - boat doesn't fit previous slip, all " + std::to_string(fittingSlipCount) + " suitable slips taken";
            }
            
            return "Evicted - boat doesn't fit previous slip";
        }
        
        // Check who occupies the current slip
        auto occupantIt = mSlipOccupant.find(currentSlipId);
        
        if (occupantIt != mSlipOccupant.end())
        {
            const Member *occupant = occupantIt->second;
            
            if (occupant->isPermanent())
            {
                return "Evicted - previous slip taken by permanent member, all " + std::to_string(fittingSlipCount) + " suitable slips taken";
            }
            
            return "Evicted - outranked by higher priority member(s), all " + std::to_string(fittingSlipCount) + " suitable slips taken";
        }
    }
    
    // Never had a slip, or lost it and no alternatives
    return "All " + std::to_string(fittingSlipCount) + " suitable slips taken by higher priority members";
}

// Find the best available slip for a boat.
//
// "Best" is defined as the smallest slip (by area) that can fit the boat.
// This minimizes wasted space and helps ensure larger slips remain available
// for larger boats.
//
// Parameters:
//   boatDimensions - dimensions of the boat to fit
//   excludeSlipId - slip to exclude from search (typically the boat's current slip)
//
// Returns:
//   Pointer to best fitting slip, or nullptr if no suitable slip exists
//
// Note: This function doesn't check occupancy - it returns the best slip
// regardless of whether it's occupied. The caller is responsible for checking
// occupancy and handling eviction if needed.
Slip *AssignmentEngine::findBestAvailableSlip(const Dimensions &boatDimensions, const std::string &excludeSlipId) {
    Slip *bestSlip = nullptr;
    int minArea = std::numeric_limits<int>::max();

    for (auto &slip : mSlips) {
        // Skip the excluded slip (typically the boat's current slip)
        if (slip.id() == excludeSlipId) {
            continue;
        }

        // Skip slips that are too small for the boat
        if (!slipFits(&slip, boatDimensions)) {
            continue;
        }

        // Calculate slip area (length × width)
        int area = slip.maxDimensions().lengthInches() * slip.maxDimensions().widthInches();

        // Track the smallest slip that fits
        if (area < minArea) {
            minArea = area;
            bestSlip = &slip;
        }
    }

    return bestSlip;
}

// Check if a boat fits in a slip, considering the ignore-length flag.
bool AssignmentEngine::slipFits(const Slip *slip, const Dimensions &boatDimensions) const {
    if (mIgnoreLength) {
        return slip->fitsWidthOnly(boatDimensions);
    }
    return slip->fits(boatDimensions);
}

// Generate length difference comment when ignoring length.
std::string AssignmentEngine::generateLengthComment(const Slip *slip, const Dimensions &boatDimensions) const {
    if (!mIgnoreLength) {
        return "";
    }
    
    int diffInches = slip->lengthDifference(boatDimensions);
    if (diffInches == 0) {
        return "";
    }
    
    int feet = std::abs(diffInches) / 12;
    int inches = std::abs(diffInches) % 12;
    std::string lengthStr;
    
    if (feet > 0 && inches > 0) {
        lengthStr = std::to_string(feet) + "' " + std::to_string(inches) + "\"";
    } else if (feet > 0) {
        lengthStr = std::to_string(feet) + "'";
    } else {
        lengthStr = std::to_string(inches) + "\"";
    }
    
    if (diffInches > 0) {
        return "NOTE: boat is " + lengthStr + " longer than slip";
    }
    return "NOTE: boat is " + lengthStr + " shorter than slip";
}

// Print summary statistics for verbose mode.
void AssignmentEngine::printStatistics(const std::vector<Assignment> &assignments) const {
    int permanentCount = 0;
    int sameCount = 0;
    int newCount = 0;
    int unassignedCount = 0;
    
    for (const auto &assignment : assignments) {
        switch (assignment.status()) {
            case Assignment::Status::PERMANENT:
                permanentCount++;
                break;
            case Assignment::Status::SAME:
                sameCount++;
                break;
            case Assignment::Status::NEW:
                newCount++;
                break;
            case Assignment::Status::UNASSIGNED:
                unassignedCount++;
                break;
        }
    }
    
    int totalPlaced = permanentCount + sameCount + newCount;
    
    // Find empty slips
    std::vector<const Slip *> emptySlips;
    for (const auto &slip : mSlips) {
        if (mSlipOccupant.find(slip.id()) == mSlipOccupant.end()) {
            emptySlips.push_back(&slip);
        }
    }
    
    std::cout << "\n===== SUMMARY STATISTICS =====\n";
    std::cout << "Permanent assignments: " << permanentCount << "\n";
    std::cout << "Boats in same slip:    " << sameCount << "\n";
    std::cout << "New assignments:       " << newCount << "\n";
    std::cout << "Total boats placed:    " << totalPlaced << "\n";
    std::cout << "Unassigned boats:      " << unassignedCount << "\n";
    std::cout << "\n";
    std::cout << "Total slips:           " << mSlips.size() << "\n";
    std::cout << "Occupied slips:        " << mSlipOccupant.size() << "\n";
    std::cout << "Empty slips:           " << emptySlips.size() << "\n";
    
    if (!emptySlips.empty()) {
        std::cout << "\nEmpty slip list:\n";
        for (const auto *slip : emptySlips) {
            int lengthFt = slip->maxDimensions().lengthInches() / 12;
            int lengthIn = slip->maxDimensions().lengthInches() % 12;
            int widthFt = slip->maxDimensions().widthInches() / 12;
            int widthIn = slip->maxDimensions().widthInches() % 12;
            
            std::cout << "  " << slip->id() << ": " << lengthFt << "' ";
            if (lengthIn > 0) {
                std::cout << lengthIn << "\" ";
            }
            std::cout << "x " << widthFt << "' ";
            if (widthIn > 0) {
                std::cout << widthIn << "\"";
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n";
}
