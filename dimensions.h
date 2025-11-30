#ifndef DIMENSIONS_H
#define DIMENSIONS_H

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

#endif
