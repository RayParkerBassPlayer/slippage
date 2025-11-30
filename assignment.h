#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include "dimensions.h"
#include <string>

class Assignment {
public:
    enum class Status {
        PERMANENT,
        SAME,
        NEW,
        UNASSIGNED
    };
    
    Assignment(const std::string &memberId, const std::string &slipId, 
               Status status, const Dimensions &dimensions, const std::string &comment = "");
    
    const std::string& memberId() const { return mMemberId; }
    const std::string& slipId() const { return mSlipId; }
    Status status() const { return mStatus; }
    const Dimensions& boatDimensions() const { return mBoatDimensions; }
    const std::string& comment() const { return mComment; }
    
    bool assigned() const;
    
    static std::string statusToString(Status status);

private:
    std::string mMemberId;
    std::string mSlipId;
    Status mStatus;
    Dimensions mBoatDimensions;
    std::string mComment;
};

#endif
