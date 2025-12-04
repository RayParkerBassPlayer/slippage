#include "assignment.hpp"
#include <algorithm>
#include <cmath>

Assignment::Assignment(const std::string &memberId, const std::string &slipId,
                       Status status, const Dimensions &boatDimensions, 
                       const Dimensions &slipDimensions, Member::DockStatus dockStatus,
                       const std::string &comment, double pricePerSqFt, bool upgraded)
    : mMemberId(memberId), mSlipId(slipId), mStatus(status), 
      mBoatDimensions(boatDimensions), mSlipDimensions(slipDimensions), 
      mComment(comment), mPrice(0.0), mUpgraded(upgraded), mDockStatus(dockStatus){
    
    if (pricePerSqFt > 0.0 && status != Status::UNASSIGNED){
        // Calculate square footage (convert from square inches to square feet)
        double boatSqFt = (boatDimensions.lengthInches() * boatDimensions.widthInches()) / 144.0;
        double slipSqFt = (slipDimensions.lengthInches() * slipDimensions.widthInches()) / 144.0;
        
        // Use the larger of boat or slip
        double billableSqFt = std::max(boatSqFt, slipSqFt);
        
        // Calculate price and round to 2 decimal places
        mPrice = std::round(billableSqFt * pricePerSqFt * 100.0) / 100.0;
    }
}

bool Assignment::assigned() const{
    return !mSlipId.empty();
}

std::string Assignment::statusToString(Status status){
    switch (status){
        case Status::PERMANENT:
            return "PERMANENT";
        case Status::SAME:
            return "SAME";
        case Status::NEW:
            return "NEW";
        case Status::UNASSIGNED:
            return "UNASSIGNED";
    }
    return "UNKNOWN";
}
