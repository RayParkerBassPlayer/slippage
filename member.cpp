#include "member.h"
#include <stdexcept>

Member::Member(const std::string &memberId, int boatFeetLength, int boatInchesLength,
               int boatFeetWidth, int boatInchesWidth,
               const std::optional<std::string> &currentSlip, DockStatus dockStatus)
    : mId(memberId),
      mBoatDimensions(boatFeetLength, boatInchesLength, boatFeetWidth, boatInchesWidth),
      mCurrentSlip(currentSlip),
      mDockStatus(dockStatus){
}

bool Member::operator<(const Member &other) const{
    return mId < other.mId;
}

bool Member::operator>(const Member &other) const{
    return mId > other.mId;
}

bool Member::operator==(const Member &other) const{
    return mId == other.mId;
}

Member::DockStatus Member::stringToDockStatus(const std::string &str){
    if (str == "permanent"){
        return DockStatus::PERMANENT;
    }
    else if (str == "year-off"){
        return DockStatus::YEAR_OFF;
    }
    else if (str == "waiting-list"){
        return DockStatus::WAITING_LIST;
    }
    else if (str == "temporary"){
        return DockStatus::TEMPORARY;
    }
    else if (str == "unassigned"){
        return DockStatus::UNASSIGNED;
    }
    else{
        throw std::invalid_argument("Invalid dock status: " + str);
    }
}

std::string Member::dockStatusToString(DockStatus status){
    switch (status){
        case DockStatus::PERMANENT:
            return "permanent";
        case DockStatus::YEAR_OFF:
            return "year-off";
        case DockStatus::WAITING_LIST:
            return "waiting-list";
        case DockStatus::TEMPORARY:
            return "temporary";
        case DockStatus::UNASSIGNED:
            return "unassigned";
    }
    return "unknown";
}
