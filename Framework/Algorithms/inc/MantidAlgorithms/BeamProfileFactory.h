// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL BeamProfileFactory {
public:
  static std::unique_ptr<IBeamProfile> createBeamProfile(Geometry::ComponentInfo const &componentInfo,
                                                         API::Sample const &sample);
};

} // namespace Algorithms
} // namespace Mantid
