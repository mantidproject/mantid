/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY ONLY be included in the MDalgorithms package
 *********************************************************************************/
#ifndef MDALGORITHMSTESTHELPER_H
#define MDALGORITHMSTESTHELPER_H

#include "MantidDataObjects/MDEventFactory.h"

namespace Mantid {
namespace MDAlgorithms {

namespace MDAlgorithmsTestHelper {

DataObjects::MDEventWorkspace3Lean::sptr
makeFileBackedMDEW(const std::string &wsName, bool fileBacked,
                   long numEvents = 10000);

} // namespace
}
}

#endif
