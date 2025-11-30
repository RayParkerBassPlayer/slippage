#ifndef ASSIGNMENT_ENGINE_H
#define ASSIGNMENT_ENGINE_H

#include "member.h"
#include "slip.h"
#include "assignment.h"
#include <vector>
#include <map>

class AssignmentEngine {
public:
    AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips);
    
    std::vector<Assignment> assign();

private:
    std::vector<Member> mMembers;
    std::vector<Slip> mSlips;
    std::map<std::string, const Member *> mSlipOccupant;
    std::map<const Member *, std::string> mMemberAssignment;
    
    void assignPermanentMembers(std::vector<Assignment> &assignments);
    void assignRemainingMembers(std::vector<Assignment> &assignments);
    
    Slip *findSlipById(const std::string &slipId);
    Slip *findBestAvailableSlip(const Dimensions &boatDimensions, const std::string &excludeSlipId = "");
    Member *findMemberById(const std::string &memberId);
    void assignMemberToSlip(const Member *member, const std::string &slipId);
    void unassignMember(const Member *member);
    bool isMemberAssigned(const Member *member) const;
};

#endif
