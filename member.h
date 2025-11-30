#ifndef MEMBER_H
#define MEMBER_H

#include "dimensions.h"
#include <string>
#include <optional>

class Member {
public:
    Member(const std::string &memberId, int boatFeetLength, int boatInchesLength, 
           int boatFeetWidth, int boatInchesWidth, 
           const std::optional<std::string> &currentSlip, bool permanent);
    
    const std::string& id() const { return mId; }
    const Dimensions& boatDimensions() const { return mBoatDimensions; }
    const std::optional<std::string>& currentSlip() const { return mCurrentSlip; }
    bool isPermanent() const { return mIsPermanent; }
    
    bool operator<(const Member &other) const;
    bool operator>(const Member &other) const;
    bool operator==(const Member &other) const;

private:
    std::string mId;
    Dimensions mBoatDimensions;
    std::optional<std::string> mCurrentSlip;
    bool mIsPermanent;
};

#endif
