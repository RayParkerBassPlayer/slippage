#include "dimensions.h"

Dimensions::Dimensions(int feetLength, int inchesLength, int feetWidth, int inchesWidth)
    : mLengthInches(feetLength * 12 + inchesLength),
      mWidthInches(feetWidth * 12 + inchesWidth){
}

bool Dimensions::fitsIn(const Dimensions &container) const{
    return mLengthInches <= container.mLengthInches && 
           mWidthInches <= container.mWidthInches;
}

bool Dimensions::fitsInWidthOnly(const Dimensions &container) const{
    return mWidthInches <= container.mWidthInches;
}

int Dimensions::lengthDifferenceInches(const Dimensions &container) const{
    return mLengthInches - container.mLengthInches;
}
