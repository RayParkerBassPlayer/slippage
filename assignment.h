#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include "dimensions.h"
#include "member.h"
#include <string>

class Assignment {
public:
    enum class Status {
        PERMANENT,
        SAME,
        NEW,
        UNASSIGNED
    };

private:
    std::string mMemberId;
    std::string mSlipId;
    Status mStatus;
    Dimensions mBoatDimensions;
    Dimensions mSlipDimensions;
    std::string mComment;
    double mPrice;
    bool mUpgraded;
    Member::DockStatus mDockStatus;

public:
    Assignment(const std::string &memberId, const std::string &slipId, 
               Status status, const Dimensions &boatDimensions, 
               const Dimensions &slipDimensions, Member::DockStatus dockStatus,
               const std::string &comment = "", double pricePerSqFt = 0.0, bool upgraded = false);
    
    const std::string& memberId() const { return mMemberId; }
    const std::string& slipId() const { return mSlipId; }
    Status status() const { return mStatus; }
    const Dimensions& boatDimensions() const { return mBoatDimensions; }
    const Dimensions& slipDimensions() const { return mSlipDimensions; }
    const std::string& comment() const { return mComment; }
    double price() const { return mPrice; }
    bool upgraded() const { return mUpgraded; }
    Member::DockStatus dockStatus() const { return mDockStatus; }
    
    void upgradeToPermament() {
        if (mStatus != Status::PERMANENT) {
            mStatus = Status::PERMANENT;
            mUpgraded = true;
        }
    }
    
    bool assigned() const;
    
    static std::string statusToString(Status status);
};

#endif
