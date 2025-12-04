#ifndef SLIP_H
#define SLIP_H

#include "dimensions.h"
#include <string>

class Slip {
    std::string mId;
    Dimensions mMaxDimensions;

public:
    Slip(const std::string &slipId, int feetLength, int inchesLength, int feetWidth, int inchesWidth);
    
    const std::string &id() const{ return mId; }
    const Dimensions &maxDimensions() const{ return mMaxDimensions; }
    
    bool fits(const Dimensions &boatDimensions) const;
    bool fitsWidthOnly(const Dimensions &boatDimensions) const;
    int lengthDifference(const Dimensions &boatDimensions) const;
};

#endif
