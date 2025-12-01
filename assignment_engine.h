#ifndef ASSIGNMENT_ENGINE_H
#define ASSIGNMENT_ENGINE_H

#include "member.h"
#include "slip.h"
#include "assignment.h"
#include <vector>
#include <map>

class AssignmentEngine {
    std::vector<Member> mMembers;
    std::vector<Slip> mSlips;
    std::map<std::string, const Member *> mSlipOccupant;
    std::map<const Member *, std::string> mMemberAssignment;
    bool mVerbose;
    bool mIgnoreLength;
    
    void assignPermanentMembers(std::vector<Assignment> &assignments);
    void assignRemainingMembers(std::vector<Assignment> &assignments);
    
    Slip *findSlipById(const std::string &slipId) const;
    Slip *findBestAvailableSlip(const Dimensions &boatDimensions, const std::string &excludeSlipId = "");
    Member *findMemberById(const std::string &memberId);
    void assignMemberToSlip(const Member *member, const std::string &slipId);
    void unassignMember(const Member *member);
    bool isMemberAssigned(const Member *member) const;
    std::string generateUnassignedComment(const Member *member) const;
    bool slipFits(const Slip *slip, const Dimensions &boatDimensions) const;
    std::string generateLengthComment(const Slip *slip, const Dimensions &boatDimensions) const;

public:
    AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips);
    
    void setVerbose(bool verbose) { mVerbose = verbose; }
    void setIgnoreLength(bool ignoreLength) { mIgnoreLength = ignoreLength; }
    std::vector<Assignment> assign();
};

#endif
