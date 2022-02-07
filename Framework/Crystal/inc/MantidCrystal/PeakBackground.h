// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidKernel/V3D.h"
#include <boost/function.hpp>

namespace Mantid {
namespace Geometry {
class IPeak;
}
namespace Crystal {

/** PeakBackground : Extension of HardThresholdBackground to consider regions of
the image as background if they are outside
the peaks radius limits (no mater what their theshold is). For pixels inside the
radius, they must also be above the threshold value.
*/

class MANTID_CRYSTAL_DLL PeakBackground : public HardThresholdBackground {
private:
  /// Peak workspace containing peaks of interest
  Mantid::API::IPeaksWorkspace_const_sptr m_peaksWS;
  /// Radius estimate
  double m_radiusEstimate;
  /// MD coordinates to use
  Mantid::Kernel::SpecialCoordinateSystem m_mdCoordinates;
  /// Pointer to member function used for coordinate determination.
  boost::function<Mantid::Kernel::V3D(const Mantid::Geometry::IPeak *)> m_coordFunction;

public:
  /// Constructor
  PeakBackground(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, const double &radiusEstimate,
                 const double &thresholdSignal, const Mantid::API::MDNormalization normalisation,
                 const Mantid::Kernel::SpecialCoordinateSystem coordinates);

  /// Overriden is background function
  bool isBackground(Mantid::API::IMDIterator *iterator) const override;

  /// Overriden configure iterator function.
  void configureIterator(Mantid::API::IMDIterator *const iterator) const override;

  /// Virutal constructor
  PeakBackground *clone() const override;
};

} // namespace Crystal
} // namespace Mantid
