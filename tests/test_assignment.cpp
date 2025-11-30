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
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, false);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S1");
    REQUIRE(assignments[0].status() == Assignment::Status::NEW);
}

TEST_CASE("Member keeps current slip", "[assignment]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    slips.emplace_back("S2", 25, 0, 12, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S2"), false);
    
    AssignmentEngine engine(std::move(members), std::move(slips));
    auto assignments = engine.assign();
    
    REQUIRE(assignments.size() == 1);
    REQUIRE(assignments[0].memberId() == "M1");
    REQUIRE(assignments[0].slipId() == "S2");
    REQUIRE(assignments[0].status() == Assignment::Status::SAME);
}

TEST_CASE("Permanent member assignment", "[assignment][permanent]") {
    std::vector<Slip> slips;
    slips.emplace_back("S1", 20, 0, 10, 0);
    
    std::vector<Member> members;
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), true);
    
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
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), true);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S2"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
    members.emplace_back("M1", 25, 0, 12, 0, std::nullopt, false);
    
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
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, false);
    
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
    members.emplace_back("M5", 18, 0, 8, 0, std::optional<std::string>("S2"), true);
    members.emplace_back("M4", 18, 0, 8, 0, std::optional<std::string>("S3"), false);
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S3"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::nullopt, false);
    members.emplace_back("M3", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
            REQUIRE(assignment.status() == Assignment::Status::SAME);
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
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
    members.emplace_back("M2", 14, 0, 7, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M1", 22, 0, 10, 0, std::optional<std::string>("S2"), false);
    
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
    members.emplace_back("M1", 25, 0, 12, 0, std::nullopt, false);
    
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
    members.emplace_back("M2", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    members.emplace_back("M1", 18, 0, 8, 0, std::optional<std::string>("S1"), false);
    
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
