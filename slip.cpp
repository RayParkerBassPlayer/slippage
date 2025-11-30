#include "slip.h"

Slip::Slip(const std::string &slipId, int feetLength, int inchesLength, int feetWidth, int inchesWidth)
    : mId(slipId), mMaxDimensions(feetLength, inchesLength, feetWidth, inchesWidth) {
}

bool Slip::fits(const Dimensions &boatDimensions) const {
    return boatDimensions.fitsIn(mMaxDimensions);
}
