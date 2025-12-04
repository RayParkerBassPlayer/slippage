#ifndef MEMBER_H
#define MEMBER_H

#include "dimensions.hpp"
#include <string>
#include <optional>

class Member {
public:
    enum class DockStatus {
        PERMANENT,
        YEAR_OFF,
        WAITING_LIST,
        TEMPORARY,
        UNASSIGNED
    };

private:
    std::string mId;
    Dimensions mBoatDimensions;
    std::optional<std::string> mCurrentSlip;
    DockStatus mDockStatus;

public:
    Member(const std::string &memberId, int boatFeetLength, int boatInchesLength, 
           int boatFeetWidth, int boatInchesWidth, 
           const std::optional<std::string> &currentSlip, DockStatus dockStatus);
    
    const std::string &id() const{ return mId; }
    const Dimensions &boatDimensions() const{ return mBoatDimensions; }
    const std::optional<std::string> &currentSlip() const{ return mCurrentSlip; }
    DockStatus dockStatus() const{ return mDockStatus; }
    
    static DockStatus stringToDockStatus(const std::string &str);
    static std::string dockStatusToString(DockStatus status);
    
    bool operator<(const Member &other) const;
    bool operator>(const Member &other) const;
    bool operator==(const Member &other) const;
};

#endif
