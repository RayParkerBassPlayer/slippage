#include "member.h"

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
