
/**
 * libemulation-hal
 * Signal processing vector
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Implements a signal processing vector
 */

#ifndef _OEVECTOR_H
#define _OEVECTOR_H

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "OECommon.h"

class OEVector
{
public:
    OEVector();
    OEVector(OEInt size);
    OEVector(std::vector<float> data);
    
    float getValue(OEInt i);
    
    OEVector operator *(const float value);
    OEVector operator *(const OEVector& v);
    OEVector normalize();
    OEVector realIDFT();
    void print() const;
    static OEVector lanczosWindow(OEInt n, float fc);
    static OEVector chebyshevWindow(OEInt n, float sidelobeDb);
    
private:
    std::vector<float> data;
};

#endif
