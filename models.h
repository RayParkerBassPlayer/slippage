#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <optional>

class Dimensions {
public:
    Dimensions(int feetLength, int inchesLength, int feetWidth, int inchesWidth);
    
    int lengthInches() const { return mLengthInches; }
    int widthInches() const { return mWidthInches; }
    
    bool fitsIn(const Dimensions &container) const;

private:
    int mLengthInches;
    int mWidthInches;
};

class Slip {
public:
    Slip(const std::string &slipId, int feetLength, int inchesLength, int feetWidth, int inchesWidth);
    
    const std::string& id() const { return mId; }
    const Dimensions& maxDimensions() const { return mMaxDimensions; }
    
    bool fits(const Dimensions &boatDimensions) const;

private:
    std::string mId;
    Dimensions mMaxDimensions;
};

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

class Assignment {
public:
    enum class Status {
        PERMANENT,
        SAME,
        NEW,
        EVICTED
    };
    
    Assignment(const std::string &memberId, const std::string &slipId, 
               Status status, const Dimensions &dimensions);
    
    const std::string& memberId() const { return mMemberId; }
    const std::string& slipId() const { return mSlipId; }
    Status status() const { return mStatus; }
    const Dimensions& boatDimensions() const { return mBoatDimensions; }
    
    bool assigned() const;
    
    static std::string statusToString(Status status);

private:
    std::string mMemberId;
    std::string mSlipId;
    Status mStatus;
    Dimensions mBoatDimensions;
};

#endif
