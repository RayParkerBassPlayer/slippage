#include "models.h"

Dimensions::Dimensions(int feetLength, int inchesLength, int feetWidth, int inchesWidth)
    : mLengthInches(feetLength * 12 + inchesLength),
      mWidthInches(feetWidth * 12 + inchesWidth) {
}

bool Dimensions::fitsIn(const Dimensions &container) const {
    return mLengthInches <= container.mLengthInches && 
           mWidthInches <= container.mWidthInches;
}

Slip::Slip(const std::string &slipId, int feetLength, int inchesLength, int feetWidth, int inchesWidth)
    : mId(slipId), mMaxDimensions(feetLength, inchesLength, feetWidth, inchesWidth) {
}

bool Slip::fits(const Dimensions &boatDimensions) const {
    return boatDimensions.fitsIn(mMaxDimensions);
}

Member::Member(const std::string &memberId, int boatFeetLength, int boatInchesLength,
               int boatFeetWidth, int boatInchesWidth,
               const std::optional<std::string> &currentSlip, bool permanent)
    : mId(memberId),
      mBoatDimensions(boatFeetLength, boatInchesLength, boatFeetWidth, boatInchesWidth),
      mCurrentSlip(currentSlip),
      mIsPermanent(permanent) {
}

bool Member::operator<(const Member &other) const {
    return mId < other.mId;
}

bool Member::operator>(const Member &other) const {
    return mId > other.mId;
}

bool Member::operator==(const Member &other) const {
    return mId == other.mId;
}

Assignment::Assignment(const std::string &memberId, const std::string &slipId,
                       Status status, const Dimensions &dimensions)
    : mMemberId(memberId), mSlipId(slipId), mStatus(status), mBoatDimensions(dimensions) {
}

bool Assignment::assigned() const {
    return !mSlipId.empty();
}

std::string Assignment::statusToString(Status status) {
    switch (status) {
        case Status::PERMANENT:
            return "PERMANENT";
        case Status::SAME:
            return "SAME";
        case Status::NEW:
            return "NEW";
        case Status::EVICTED:
            return "EVICTED";
    }
    return "UNKNOWN";
}
