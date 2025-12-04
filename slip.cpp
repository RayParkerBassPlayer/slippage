#include "slip.h"

Slip::Slip(const std::string &slipId, int feetLength, int inchesLength, int feetWidth, int inchesWidth)
    : mId(slipId), mMaxDimensions(feetLength, inchesLength, feetWidth, inchesWidth){
}

bool Slip::fits(const Dimensions &boatDimensions) const{
    return boatDimensions.fitsIn(mMaxDimensions);
}

bool Slip::fitsWidthOnly(const Dimensions &boatDimensions) const{
    return boatDimensions.fitsInWidthOnly(mMaxDimensions);
}

int Slip::lengthDifference(const Dimensions &boatDimensions) const{
    return boatDimensions.lengthDifferenceInches(mMaxDimensions);
}
