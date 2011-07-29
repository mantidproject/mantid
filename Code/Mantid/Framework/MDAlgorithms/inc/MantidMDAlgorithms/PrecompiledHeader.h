#ifndef MANTID_MDALGORITHMS_PRECOMPILEDHEADER_H_
#define MANTID_MDALGORITHMS_PRECOMPILEDHEADER_H_

#include "MantidAPI/Algorithm.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidKernel/Exception.h"

//MG: Keep this for the time being but it should go !
template <class T>
bool isNaN(T val){
   volatile T buf=val;
    return (val!=buf);
}

#endif // MANTID_MDALGORITHMS_PRECOMPILEDHEA