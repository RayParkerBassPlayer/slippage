#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "../assignment_engine.h"
#include "../member.h"
#include "../slip.h"
#include "../assignment.h"

TEST_CASE("Basic slip assignment", "[assignment]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::UNASSIGNED);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
}

TEST_CASE("Member keeps current slip (auto-upgraded)", "[assignment]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S2"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S2");
    REQUIRE(assignments[0].status() == Assignment::Status::PERMANENT);
    REQUIRE(assignments[0].upgraded() == true);
}

TEST_CASE("Permanent member assignment", "[assignment][permanent]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::PERMANENT);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::PERMANENT);
}

TEST_CASE("Higher priority member evicts lower priority from current slip", "[assignment][eviction]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m1Assigned = false, m2Unassigned = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S1") {
            m1Assigned = true;
        }
        if (assignment.memberId() == "M2" && assignment.status() == Assignment::Status::UNASSIGNED) {
            m2Unassigned = true;
        }
    }
    
    REQUIRE(m1Assigned);
    REQUIRE(m2Unassigned);
}

TEST_CASE("Higher priority member evicts lower priority, lower priority gets nothing", "[assignment][eviction]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m1Assigned = false, m3Unassigned = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S1") {
            m1Assigned = true;
        }
        if (assignment.memberId() == "M3" && assignment.status() == Assignment::Status::UNASSIGNED) {
            m3Unassigned = true;
        }
    }
    
    REQUIRE(m1Assigned);
    REQUIRE(m3Unassigned);
}

TEST_CASE("Permanent members cannot be evicted", "[assignment][permanent][eviction]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::PERMANENT);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m2Permanent = false, m1Unassigned = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M2" && assignment.status() == Assignment::Status::PERMANENT) {
            m2Permanent = true;
        }
        if (assignment.memberId() == "M1" && assignment.status() == Assignment::Status::UNASSIGNED) {
            m1Unassigned = true;
        }
    }
    
    REQUIRE(m2Permanent);
    REQUIRE(m1Unassigned);
}

TEST_CASE("Multiple members with eviction and reassignment", "[assignment][eviction]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    slips.emplace_back("S3", 22, 0, 11, 0);
    
    std::vector<Member> members;
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S2"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 3);
    
    bool m1HasS1 = false, m2HasS2 = false, m3HasS3 = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S1") {
            m1HasS1 = true;
        }
        if (assignment.memberId() == "M2" && assignment.slipId() == "S2") {
            m2HasS2 = true;
        }
        if (assignment.memberId() == "M3" && assignment.slipId() == "S3") {
            m3HasS3 = true;
        }
    }
    
    REQUIRE(m1HasS1);
    REQUIRE(m2HasS2);
    REQUIRE(m3HasS3);
}

TEST_CASE("Boat too large for all slips", "[assignment][no_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 12, 0, std::nullopt, Member::DockStatus::UNASSIGNED);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::UNASSIGNED);
}

TEST_CASE("Smallest fitting slip is chosen", "[assignment][best_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 15, 0);
    slips.emplace_back("S2", 20, 0, 10, 0);
    slips.emplace_back("S3", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::UNASSIGNED);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S2");
}

TEST_CASE("Complex scenario with permanent, eviction, and new assignments", "[assignment][complex]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    slips.emplace_back("S3", 30, 0, 15, 0);
    slips.emplace_back("S4", 22, 0, 11, 0);
    
    std::vector<Member> members;
    members.emplace_back("M5", 18, 0, 8, 0, std::optional<std::string>("S2"), Member::DockStatus::PERMANENT);
    members.emplace_back("M4", 18, 0, 8, 0, std::optional<std::string>("S3"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S3"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    bool m5Found = false, m1Found = false, m2Found = false, m3Found = false;
    
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M5") {
            REQUIRE(assignment.slipId() == "S2");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            m5Found = true;
        }
        else if (assignment.memberId() == "M1") {
            REQUIRE(assignment.slipId() == "S1");
            REQUIRE(assignment.status() == Assignment::Status::NEW);
            m1Found = true;
        }
        else if (assignment.memberId() == "M2") {
            REQUIRE(assignment.slipId() == "S3");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == true);
            m2Found = true;
        }
        else if (assignment.memberId() == "M3") {
            m3Found = true;
        }
    }
    
    REQUIRE(m5Found);
    REQUIRE(m1Found);
    REQUIRE(m2Found);
    REQUIRE(m3Found);
}

TEST_CASE("Evicted member finds alternative slip", "[assignment][eviction][reassignment]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m1HasS1 = false, m2HasS2 = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S1") {
            m1HasS1 = true;
        }
        if (assignment.memberId() == "M2" && assignment.slipId() == "S2") {
            m2HasS2 = true;
        }
    }
    
    REQUIRE(m1HasS1);
    REQUIRE(m2HasS2);
}

TEST_CASE("Lower priority member keeps small slip that higher priority cannot fit", "[assignment][size_protection]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 15, 0, 8, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 14, 0, 7, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 22, 0, 10, 0, std::optional<std::string>("S2"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m1HasS2 = false, m2HasS1 = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S2") {
            m1HasS2 = true;
        }
        if (assignment.memberId() == "M2" && assignment.slipId() == "S1") {
            m2HasS1 = true;
        }
    }
    
    REQUIRE(m1HasS2);
    REQUIRE(m2HasS1);
}

TEST_CASE("Member marked UNASSIGNED when boat too large for all slips", "[assignment][unassigned]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 12, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "");
    REQUIRE(assignments[0].status() == Assignment::Status::UNASSIGNED);
}

TEST_CASE("Evicted member marked UNASSIGNED when no alternative slip available", "[assignment][unassigned][eviction]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    bool m1Assigned = false, m2Unassigned = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1" && assignment.slipId() == "S1") {
            m1Assigned = true;
        }
        if (assignment.memberId() == "M2" && assignment.status() == Assignment::Status::UNASSIGNED) {
            m2Unassigned = true;
        }
    }
    
    REQUIRE(m1Assigned);
    REQUIRE(m2Unassigned);
}

TEST_CASE("Ignore length: boat too long but fits width-wise", "[assignment][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
    REQUIRE(assignments[0].comment() == "NOTE: boat is 5' longer than slip");
}

TEST_CASE("Ignore length: boat shorter than slip", "[assignment][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 25, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 20, 6, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
    REQUIRE(assignments[0].comment() == "NOTE: boat is 4' 6\" shorter than slip");
}

TEST_CASE("Ignore length: boat too wide still fails", "[assignment][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 12, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::UNASSIGNED);
}

TEST_CASE("Ignore length: permanent member gets length comment", "[assignment][ignore_length][permanent]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 22, 3, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::PERMANENT);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::PERMANENT);
    REQUIRE(assignments[0].comment() == "NOTE: boat is 2' 3\" longer than slip");
}

TEST_CASE("Ignore length: exact length match has no comment", "[assignment][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 20, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].comment().empty());
}

TEST_CASE("Ignore length: best-fit minimizes overhang", "[assignment][ignore_length][best_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 15, 0);  // 0 overhang, large area
    slips.emplace_back("S2", 20, 0, 10, 0);  // 5' overhang, smallest area
    slips.emplace_back("S3", 25, 0, 12, 0);  // 0 overhang, medium area
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    // Should choose S3 (exact length match, smallest area with 0 overhang)
    REQUIRE(assignments[0].slipId() == "S3");
    REQUIRE(assignments[0].comment().empty());
}

TEST_CASE("Ignore length: doesn't affect width-only fits", "[assignment][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 15, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].comment() == "NOTE: boat is 15' shorter than slip");
}

TEST_CASE("Ignore length: overhang prioritization with varying overhangs", "[assignment][ignore_length][best_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);  // 10' overhang
    slips.emplace_back("S2", 25, 0, 10, 0);  // 5' overhang
    slips.emplace_back("S3", 28, 0, 10, 0);  // 2' overhang
    
    std::vector<Member> members;
    members.emplace_back("M1", 30, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    // Should choose S3 (minimum 2' overhang)
    REQUIRE(assignments[0].slipId() == "S3");
    REQUIRE(assignments[0].comment() == "NOTE: boat is 2' longer than slip");
}

// New tests for tight fit feature
TEST_CASE("Tight fit warning when boat is 5 inches narrower than slip", "[assignment][tight_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 5);  // 125" width
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].comment() == "TIGHT FIT");
}

TEST_CASE("No tight fit warning when boat is 6 inches narrower", "[assignment][tight_fit]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 6);  // 126" width
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].comment().empty());
}

TEST_CASE("Tight fit warning with ignore-length comment", "[assignment][tight_fit][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 3);  // 123" width
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width, 25' length
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].comment() == "NOTE: boat is 5' longer than slip; TIGHT FIT");
}

// Tests for width-prioritized best fit
TEST_CASE("Best fit prefers smallest area, width margin is tie-breaker", "[assignment][best_fit][width_priority]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);  // 240 sqft, 0" width margin
    slips.emplace_back("S2", 25, 0, 10, 6);  // 318.75 sqft, 6" width margin
    slips.emplace_back("S3", 30, 0, 11, 0);  // 396 sqft, 12" width margin
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    // Should choose S1 (smallest area) even though it has 0" width margin
    REQUIRE(assignments[0].slipId() == "S1");
}

TEST_CASE("Best fit: same width margin, prefer smaller area", "[assignment][best_fit][width_priority]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 30, 0, 11, 0);  // 396 sqft, 12" width margin
    slips.emplace_back("S2", 25, 0, 11, 0);  // 330 sqft, 12" width margin
    slips.emplace_back("S3", 20, 0, 11, 0);  // 264 sqft, 12" width margin
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    // Should choose S3 (smallest area with same 12" width margin)
    REQUIRE(assignments[0].slipId() == "S3");
}

TEST_CASE("Best fit with ignore-length: overhang prioritized, then area, then width margin", "[assignment][best_fit][width_priority][ignore_length]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 25, 0, 10, 6);  // 0' overhang, 318.75 sqft, 6" width margin
    slips.emplace_back("S2", 25, 0, 11, 0);  // 0' overhang, 330 sqft, 12" width margin
    slips.emplace_back("S3", 20, 0, 12, 0);  // 5' overhang, 288 sqft, 24" width margin
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 120" width, 25' length
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setIgnoreLength(true);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    // Should choose S1 (0' overhang takes priority, then smallest area among 0' overhang)
    REQUIRE(assignments[0].slipId() == "S1");
}

// Tests for price calculation
TEST_CASE("Price calculation: boat area larger than slip area", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 35, 0, 14, 0);  // 490 sqft, fits boat
    
    std::vector<Member> members;
    members.emplace_back("M1", 32, 0, 13, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 416 sqft
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setPricePerSqFt(2.50);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    // Price should be based on slip size (490 sqft * 2.50 = 1225.00)
    REQUIRE(assignments[0].price() == 1225.00);
}

TEST_CASE("Price calculation: slip larger than boat", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 40, 0, 15, 0);  // 600 sqft
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 250 sqft
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setPricePerSqFt(3.00);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    // Price should be based on slip size (600 sqft * 3.00 = 1800.00)
    REQUIRE(assignments[0].price() == 1800.00);
}

TEST_CASE("Price calculation: exact match", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 25, 0, 10, 0);  // 250 sqft
    
    std::vector<Member> members;
    members.emplace_back("M1", 25, 0, 10, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // 250 sqft
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setPricePerSqFt(2.75);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    // Price should be 250 sqft * 2.75 = 687.50
    REQUIRE(assignments[0].price() == 687.50);
}

TEST_CASE("Price calculation: unassigned members have zero price", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 35, 0, 15, 0, std::nullopt, Member::DockStatus::TEMPORARY);  // Too large
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setPricePerSqFt(2.50);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::UNASSIGNED);
    REQUIRE(assignments[0].price() == 0.0);
}

TEST_CASE("Price calculation: without price-per-sqft set", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].price() == 0.0);
}

TEST_CASE("Price calculation: rounding to 2 decimal places", "[assignment][price]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 23, 6, 9, 6);  // 23.5' x 9.5' = 223.25 sqft
    
    std::vector<Member> members;
    members.emplace_back("M1", 20, 0, 8, 6, std::nullopt, Member::DockStatus::TEMPORARY);  // 20' x 8.5' = 170 sqft
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    engine.setPricePerSqFt(2.75);
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].slipId() == "S1");
    // Price should be 223.25 * 2.75 = 613.9375, rounded to 613.94
    REQUIRE(assignments[0].price() == 613.94);
}

// Tests for upgrade-status feature
TEST_CASE("Upgrade status: SAME becomes PERMANENT", "[assignment][upgrade]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    members.emplace_back("M2", 22, 0, 10, 0, std::optional<std::string>("S2"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 2);
    
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M1") {
            REQUIRE(assignment.slipId() == "S1");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == true);
        }
        else if (assignment.memberId() == "M2") {
            REQUIRE(assignment.slipId() == "S2");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == true);
        }
    }
}

TEST_CASE("Upgrade status: NEW assignments not upgraded", "[assignment][upgrade]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
    REQUIRE(assignments[0].upgraded() == false);
}

TEST_CASE("Upgrade status: Already PERMANENT members not marked as upgraded", "[assignment][upgrade]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::PERMANENT);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::PERMANENT);
    REQUIRE(assignments[0].upgraded() == false);  // Was already permanent
}

TEST_CASE("Upgrade status: Always auto-upgrade SAME to PERMANENT", "[assignment][upgrade]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].status() == Assignment::Status::PERMANENT);
    REQUIRE(assignments[0].upgraded() == true);
}

TEST_CASE("Upgrade status: Mixed scenario", "[assignment][upgrade]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    slips.emplace_back("S3", 30, 0, 15, 0);
    
    std::vector<Member> members;
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), Member::DockStatus::TEMPORARY);  // Will keep S1 -> upgraded
    members.emplace_back("M3", 22, 0, 10, 0, std::optional<std::string>("S2"), Member::DockStatus::TEMPORARY);  // Will keep S2 -> upgraded
    members.emplace_back("M4", 28, 0, 14, 0, std::optional<std::string>("S3"), Member::DockStatus::PERMANENT);  // Already permanent -> not upgraded
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 3);
    
    bool m2Found = false, m3Found = false, m4Found = false;
    for (const auto &assignment : assignments) {
        if (assignment.memberId() == "M2") {
            REQUIRE(assignment.slipId() == "S1");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == true);
            m2Found = true;
        }
        else if (assignment.memberId() == "M3") {
            REQUIRE(assignment.slipId() == "S2");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == true);
            m3Found = true;
        }
        else if (assignment.memberId() == "M4") {
            REQUIRE(assignment.slipId() == "S3");
            REQUIRE(assignment.status() == Assignment::Status::PERMANENT);
            REQUIRE(assignment.upgraded() == false);
            m4Found = true;
        }
    }
    
    REQUIRE(m2Found);
    REQUIRE(m3Found);
    REQUIRE(m4Found);
}
