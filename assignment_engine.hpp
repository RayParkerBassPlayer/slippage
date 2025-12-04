#ifndef ASSIGNMENT_ENGINE_H
#define ASSIGNMENT_ENGINE_H

#include "member.hpp"
#include "slip.hpp"
#include "assignment.hpp"
#include <vector>
#include <map>

class AssignmentEngine {
    std::vector<Member> mMembers;
    std::vector<Slip> mSlips;
    std::map<std::string, const Member *> mSlipOccupant;
    std::map<const Member *, std::string> mMemberAssignment;
    bool mVerbose;
    bool mIgnoreLength;
    double mPricePerSqFt;
    
    void assignPermanentMembers(std::vector<Assignment> &assignments);
    void processYearOffMembers(std::vector<Assignment> &assignments);
    void assignRemainingMembers(std::vector<Assignment> &assignments);
    
    bool canMemberEvict(const Member *member) const;
    bool canEvictMember(const Member *evictingMember, const Member *occupant) const;
    int getDockStatusPriority(Member::DockStatus status) const;
    
    Slip *findSlipById(const std::string &slipId) const;
    Slip *findBestAvailableSlip(const Dimensions &boatDimensions, const Member *requestingMember, const std::string &excludeSlipId = "");
    Member *findMemberById(const std::string &memberId);
    void assignMemberToSlip(const Member *member, const std::string &slipId);
    void unassignMember(const Member *member);
    bool isMemberAssigned(const Member *member) const;
    std::string generateUnassignedComment(const Member *member) const;
    bool slipFits(const Slip *slip, const Dimensions &boatDimensions) const;
    std::string generateLengthComment(const Slip *slip, const Dimensions &boatDimensions) const;
    std::string generateWidthMarginNote(const Slip *slip, const Dimensions &boatDimensions) const;
    void printStatistics(const std::vector<Assignment> &assignments) const;

public:
    AssignmentEngine(std::vector<Member> members, std::vector<Slip> slips);
    
    void setVerbose(bool verbose){ mVerbose = verbose; }
    void setIgnoreLength(bool ignoreLength){ mIgnoreLength = ignoreLength; }
    void setPricePerSqFt(double pricePerSqFt){ mPricePerSqFt = pricePerSqFt; }
    std::vector<Assignment> assign();
};

#endif
