#include "assignment_engine.h"
#include <algorithm>
#include <limits>
#include <iostream>

AssignmentEngine::AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips)
    : mMembers(std::move(members)), mSlips(std::move(slips)), mVerbose(false), mIgnoreLength(false), mPricePerSqFt(0.0) {
}

// Main assignment algorithm entry point.
// Strategy: Two-phase assignment process
// Phase 1: Lock in permanent member assignments (cannot be evicted)
// Phase 2: Iteratively assign non-permanent members with eviction support
std::vector<Assignment> AssignmentEngine::assign() {
    std::vector<Assignment> assignments;
    
    assignPermanentMembers(assignments);
    processYearOffMembers(assignments);
    assignRemainingMembers(assignments);
    
    // Final pass: upgrade SAME status to PERMANENT
    for (auto &assignment : assignments) {
        if (assignment.status() == Assignment::Status::SAME) {
            assignment.upgradeToPermament();
        }
    }
    
    if (mVerbose) {
        printStatistics(assignments);
    }
    
    return assignments;
}

// Phase 1: Assign permanent members to their designated slips.
// 
// Permanent members have guaranteed assignments that cannot be evicted by
// anyone, regardless of priority. This is the first phase to ensure these
// critical assignments are locked in before processing other members.
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
        // Skip non-permanent members - handled in later phases
        if (member.dockStatus() != Member::DockStatus::PERMANENT) {
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
                }
                else {
                    comment = lengthComment;
                }
            }
            
            // Add tight fit note if boat is within 6 inches of slip width
            std::string widthNote = generateWidthMarginNote(slip, member.boatDimensions());
            if (!widthNote.empty()) {
                if (!comment.empty()) {
                    comment += "; " + widthNote;
                }
                else {
                    comment = widthNote;
                }
            }

            assignments.emplace_back(member.id(), slipId, 
                                    Assignment::Status::PERMANENT, 
                                    member.boatDimensions(),
                                    slip->maxDimensions(), member.dockStatus(),
                                    comment, mPricePerSqFt);
            
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

// Phase 2: Process year-off members - they don't get slip assignments.
void AssignmentEngine::processYearOffMembers(std::vector<Assignment> &assignments) {
    if (mVerbose)
    {
        std::cout << "\n===== PHASE 2: Year-Off Members =====\n";
    }
    
    for (auto &member : mMembers) {
        if (member.dockStatus() != Member::DockStatus::YEAR_OFF) {
            continue;
        }
        
        Dimensions emptyDimensions(0, 0, 0, 0);
        std::string previousSlip = member.currentSlip().value_or("");
        
        // Year-off members get no slip assignment
        assignments.emplace_back(member.id(), "", 
                                Assignment::Status::UNASSIGNED, 
                                member.boatDimensions(),
                                emptyDimensions, member.dockStatus(),
                                "Year off - not assigned", mPricePerSqFt);
        
        if (mVerbose)
        {
            std::cout << "  Member " << member.id() << " (YEAR-OFF)";
            if (!previousSlip.empty()) {
                std::cout << " - previous slip: " << previousSlip;
            }
            std::cout << "\n";
        }
    }
}

// Phase 3+: Assign members by dock status priority with iterative eviction support.
//
// This is the core assignment algorithm that handles priority-based assignment
// with eviction and reassignment. The algorithm runs iteratively until no more
// changes occur, ensuring evicted members are reconsidered for other slips.
//
// Algorithm overview:
// 1. Process members in dock status priority order: WAITING_LIST, TEMPORARY, UNASSIGNED
// 2. Within each status, sort by member ID (lower = higher priority)
// 3. Process each unassigned member in priority order
// 4. Try to assign them to their preferred slip or find best alternative
// 5. If slip is occupied by lower-priority member, evict them
// 6. Repeat until no evictions occur (stable state reached)
// 7. Add all assigned members to output
// 8. Add all unassigned members to output with UNASSIGNED status
void AssignmentEngine::assignRemainingMembers(std::vector<Assignment> &assignments) {
    // Process each dock status in priority order
    Member::DockStatus statusOrder[] = {
        Member::DockStatus::WAITING_LIST,
        Member::DockStatus::TEMPORARY,
        Member::DockStatus::UNASSIGNED
    };
    
    int phaseNumber = 3;
    
    for (Member::DockStatus currentStatus : statusOrder) {
        // Build list of members with this dock status
        std::vector<Member *> assignableMembers;

        for (auto &member : mMembers) {
            if (member.dockStatus() == currentStatus) {
                assignableMembers.push_back(&member);
            }
        }
        
        if (assignableMembers.empty()) {
            continue;
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
            std::cout << "\n===== PHASE " << phaseNumber << ": " 
                      << Member::dockStatusToString(currentStatus) 
                      << " Members =====\n";
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
                // They've found their slip and won't be evicted by same or lower priority
                if (isMemberAssigned(member)) {
                    continue;
                }
                
                // Determine if this member can evict others
                bool canEvict = canMemberEvict(member);

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
                        // Evict them if we can (based on dock status priority)
                        else if (canEvict && canEvictMember(member, occupantIt->second)) {
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
                    Slip *bestSlip = findBestAvailableSlip(member->boatDimensions(), member, excludeSlip);

                    if (bestSlip) {
                        auto occupantIt = mSlipOccupant.find(bestSlip->id());

                        // Case 1: Slip is available (not occupied) - take it
                        if (occupantIt == mSlipOccupant.end()) {
                            assignedSlipId = bestSlip->id();
                        }
                        // Case 2: Slip is occupied, try to evict if we have higher priority
                        else if (canEvict && canEvictMember(member, occupantIt->second)) {
                            unassignMember(occupantIt->second);
                            assignedSlipId = bestSlip->id();
                            changesMade = true;
                        }
                        // Case 3: Slip occupied by higher priority - cannot take it
                    }
                }

                // STEP 3: Assign member to slip if one was found
                if (!assignedSlipId.empty()) {
                    assignMemberToSlip(member, assignedSlipId);
                    
                    if (mVerbose) {
                        std::cout << "  Member " << member->id() << " -> Slip " << assignedSlipId;
                        
                        if (member->currentSlip().has_value() && member->currentSlip().value() == assignedSlipId) {
                            std::cout << " (keeping current)";
                        }
                        else {
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
        // End of iterative loop - stable assignment state reached for this status
        
        if (mVerbose)
        {
            std::cout << "\nPhase " << phaseNumber << " complete after " << (passNumber - 1) << " pass(es)\n";
        }
        
        phaseNumber++;
    }

    // STEP 4: Generate output for all assigned members
    // Determine if they kept their slip (SAME) or got a new one (NEW)
    for (const auto &entry : mMemberAssignment) {
        const Member *member = entry.first;
        const std::string &slipId = entry.second;

        // Skip permanent and year-off members - already added to output in phases 1 and 2
        if (member->dockStatus() == Member::DockStatus::PERMANENT || 
            member->dockStatus() == Member::DockStatus::YEAR_OFF) {
            continue;
        }

        // Determine status: SAME if kept current slip, NEW otherwise
        Assignment::Status status = Assignment::Status::NEW;
        if (member->currentSlip().has_value() && member->currentSlip().value() == slipId) {
            status = Assignment::Status::SAME;
        }
        
        // Add length difference comment if ignoring length
        Slip *assignedSlip = findSlipById(slipId);
        std::string comment = "";
        if (assignedSlip) {
            std::string lengthComment = generateLengthComment(assignedSlip, member->boatDimensions());
            if (!lengthComment.empty()) {
                comment = lengthComment;
            }
            
            // Add tight fit note if boat is within 6 inches of slip width
            std::string widthNote = generateWidthMarginNote(assignedSlip, member->boatDimensions());
            if (!widthNote.empty()) {
                if (!comment.empty()) {
                    comment += "; " + widthNote;
                }
                else {
                    comment = widthNote;
                }
            }
        }

        assignments.emplace_back(member->id(), slipId, status, 
                                member->boatDimensions(), 
                                assignedSlip->maxDimensions(), member->dockStatus(),
                                comment, mPricePerSqFt);
    }

    // STEP 5: Generate output for all unassigned members (not permanent or year-off)
    // These members couldn't be assigned due to:
    // - Boat too large for all slips
    // - All suitable slips occupied by higher-priority members
    // - Evicted and no alternative slip found
    for (auto &member : mMembers) {
        if (member.dockStatus() != Member::DockStatus::PERMANENT && 
            member.dockStatus() != Member::DockStatus::YEAR_OFF &&
            !isMemberAssigned(&member)) {
            std::string comment = generateUnassignedComment(&member);
            Dimensions emptyDimensions(0, 0, 0, 0);
            assignments.emplace_back(member.id(), "", Assignment::Status::UNASSIGNED, 
                                    member.boatDimensions(), emptyDimensions, member.dockStatus(),
                                    comment, mPricePerSqFt);
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

// Check if a member can evict others based on their dock status.
// Returns true if the member can potentially evict someone from a slip.
// Note: This doesn't prevent them from taking empty slips.
bool AssignmentEngine::canMemberEvict(const Member *member) const {
    // UNASSIGNED members have lowest priority and cannot evict anyone
    // (they're looking for their first assignment)
    return member->dockStatus() != Member::DockStatus::UNASSIGNED;
}

// Determine if evictingMember can evict occupant based on dock status and member ID.
bool AssignmentEngine::canEvictMember(const Member *evictingMember, const Member *occupant) const {
    // Permanent members cannot be evicted
    if (occupant->dockStatus() == Member::DockStatus::PERMANENT) {
        return false;
    }
    
    // Year-off members shouldn't be in slips, but if they are, they can be evicted
    if (occupant->dockStatus() == Member::DockStatus::YEAR_OFF) {
        return true;
    }
    
    int evictorPriority = getDockStatusPriority(evictingMember->dockStatus());
    int occupantPriority = getDockStatusPriority(occupant->dockStatus());
    
    // Higher dock status priority wins
    if (evictorPriority < occupantPriority) {
        return true;
    }
    
    // Same dock status: lower member ID wins
    if (evictorPriority == occupantPriority && *evictingMember < *occupant) {
        return true;
    }
    
    return false;
}

// Get numeric priority for dock status (lower = higher priority).
int AssignmentEngine::getDockStatusPriority(Member::DockStatus status) const {
    switch (status) {
        case Member::DockStatus::PERMANENT:
            return 0;  // Highest priority (cannot be evicted)
        case Member::DockStatus::WAITING_LIST:
            return 1;
        case Member::DockStatus::TEMPORARY:
            return 2;
        case Member::DockStatus::UNASSIGNED:
            return 3;  // Lowest priority
        case Member::DockStatus::YEAR_OFF:
            return 4;  // Should not be in slips
    }
    return 999;
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
    if (hadCurrentSlip) {
        const std::string &currentSlipId = member->currentSlip().value();
        Slip *currentSlip = findSlipById(currentSlipId);
        
        if (!currentSlip) {
            return "Evicted - previous slip no longer exists";
        }
        
        // Check who occupies the current slip
        // Note: We don't check if boat fits - if they had the slip, they keep it regardless
        // The only reason for eviction is being bumped by another member
        auto occupantIt = mSlipOccupant.find(currentSlipId);
        
        if (occupantIt != mSlipOccupant.end()) {
            const Member *occupant = occupantIt->second;
            
            if (occupant->dockStatus() == Member::DockStatus::PERMANENT) {
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
// "Best" is defined based on mode:
// - Normal mode: smallest slip by area that can fit the boat
// - Ignore-length mode: slip with minimum length overhang, then by smallest area
//
// This minimizes wasted space and helps ensure larger slips remain available
// for larger boats. In ignore-length mode, it also minimizes boat overhang.
//
// Parameters:
//   boatDimensions - dimensions of the boat to fit
//   requestingMember - the member requesting the slip (for priority checking)
//   excludeSlipId - slip to exclude from search (typically the boat's current slip)
//
// Returns:
//   Pointer to best fitting slip that is either empty or can be taken via eviction
//   Returns nullptr if no suitable slip exists
Slip *AssignmentEngine::findBestAvailableSlip(const Dimensions &boatDimensions, const Member *requestingMember, const std::string &excludeSlipId) {
    Slip *bestSlip = nullptr;
    int minOverhang = std::numeric_limits<int>::max();
    int minArea = std::numeric_limits<int>::max();
    int maxWidthMargin = -1;

    for (auto &slip : mSlips) {
        // Skip the excluded slip (typically the boat's current slip)
        if (slip.id() == excludeSlipId) {
            continue;
        }

        // Skip slips that are too small for the boat
        if (!slipFits(&slip, boatDimensions)) {
            continue;
        }
        
        // Skip slips occupied by members we cannot evict
        auto occupantIt = mSlipOccupant.find(slip.id());
        if (occupantIt != mSlipOccupant.end()) {
            if (!canEvictMember(requestingMember, occupantIt->second)) {
                continue;
            }
        }

        // Calculate slip area (length × width)
        int area = slip.maxDimensions().lengthInches() * slip.maxDimensions().widthInches();
        
        // Calculate width margin (how much extra width boat has)
        int widthMargin = slip.maxDimensions().widthInches() - boatDimensions.widthInches();
        
        // In ignore-length mode, prioritize minimum overhang, then minimum area, then max width margin
        if (mIgnoreLength) {
            // Positive overhang means boat is longer than slip
            int overhang = std::max(0, slip.lengthDifference(boatDimensions));
            
            // Prefer slip with less overhang
            if (overhang < minOverhang) {
                minOverhang = overhang;
                minArea = area;
                maxWidthMargin = widthMargin;
                bestSlip = &slip;
            }
            // If overhang is the same, prefer smaller area
            else if (overhang == minOverhang && area < minArea) {
                minArea = area;
                maxWidthMargin = widthMargin;
                bestSlip = &slip;
            }
            // If overhang and area are the same, prefer max width margin
            else if (overhang == minOverhang && area == minArea && widthMargin > maxWidthMargin) {
                maxWidthMargin = widthMargin;
                bestSlip = &slip;
            }
        }
        // In normal mode, prefer smallest slip by area, then max width margin as tie-breaker
        else {
            // Prefer smaller slip by area
            if (area < minArea) {
                minArea = area;
                maxWidthMargin = widthMargin;
                bestSlip = &slip;
            }
            // If area is the same, prefer max width margin
            else if (area == minArea && widthMargin > maxWidthMargin) {
                maxWidthMargin = widthMargin;
                bestSlip = &slip;
            }
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
    }
    else if (feet > 0) {
        lengthStr = std::to_string(feet) + "'";
    }
    else {
        lengthStr = std::to_string(inches) + "\"";
    }
    
    if (diffInches > 0) {
        return "NOTE: boat is " + lengthStr + " longer than slip";
    }
    return "NOTE: boat is " + lengthStr + " shorter than slip";
}

// Generate width margin note if boat is less than 6 inches narrower than slip.
std::string AssignmentEngine::generateWidthMarginNote(const Slip *slip, const Dimensions &boatDimensions) const {
    int widthMargin = slip->maxDimensions().widthInches() - boatDimensions.widthInches();
    
    if (widthMargin >= 0 && widthMargin < 6) {
        return "TIGHT FIT";
    }
    
    return "";
}

// Print summary statistics for verbose mode.
void AssignmentEngine::printStatistics(const std::vector<Assignment> &assignments) const {
    int permanentCount = 0;
    int sameCount = 0;
    int newCount = 0;
    int unassignedCount = 0;
    int upgradedCount = 0;
    
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
        if (assignment.upgraded()) {
            upgradedCount++;
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
    if (upgradedCount > 0) {
        std::cout << "Members upgraded:      " << upgradedCount << "\n";
    }
    else {
        std::cout << "Boats in same slip:    " << sameCount << "\n";
    }
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
