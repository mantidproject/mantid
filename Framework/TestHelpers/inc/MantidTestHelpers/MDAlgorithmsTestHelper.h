// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY ONLY be included in the MDalgorithms package
 *********************************************************************************/
#pragma once

#include "MantidDataObjects/MDEventFactory.h"

namespace Mantid {
namespace MDAlgorithms {

namespace MDAlgorithmsTestHelper {

DataObjects::MDEventWorkspace3Lean::sptr makeFileBackedMDEW(const std::string &wsName, bool fileBacked,
                                                            long numEvents = 10000,
                                                            Kernel::SpecialCoordinateSystem coord = Kernel::None);

DataObjects::MDEventWorkspace3Lean::sptr
makeFileBackedMDEWwithMDFrame(const std::string &wsName, bool fileBacked, const Mantid::Geometry::MDFrame &frame,
                              long numEvents = 10000, Kernel::SpecialCoordinateSystem coord = Kernel::None);

} // namespace MDAlgorithmsTestHelper
} // namespace MDAlgorithms
} // namespace Mantid
