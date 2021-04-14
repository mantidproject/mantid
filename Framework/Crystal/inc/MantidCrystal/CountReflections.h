// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidCrystal/PeakStatisticsTools.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

/** CountReflections

  This algorithm takes a PeaksWorkspace and calculates statistics that are
  based on point group symmetry and do not depend on intensities. For those
  statistics look at SortHKL.
*/
class MANTID_CRYSTAL_DLL CountReflections : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PredictPeaks", "SortHKL"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  DataObjects::PeaksWorkspace_sptr getPeaksWorkspace(const DataObjects::PeaksWorkspace_sptr &templateWorkspace,
                                                     const PeakStatisticsTools::UniqueReflectionCollection &reflections,
                                                     const Geometry::PointGroup_sptr &pointGroup) const;
};

} // namespace Crystal
} // namespace Mantid
