#ifndef DIMENSIONS_H
#define DIMENSIONS_H

class Dimensions {
    int mLengthInches;
    int mWidthInches;

public:
    Dimensions(int feetLength, int inchesLength, int feetWidth, int inchesWidth);
    
    int lengthInches() const{ return mLengthInches; }
    int widthInches() const{ return mWidthInches; }
    
    bool fitsIn(const Dimensions &container) const;
    bool fitsInWidthOnly(const Dimensions &container) const;
    int lengthDifferenceInches(const Dimensions &container) const;
};

#endif
