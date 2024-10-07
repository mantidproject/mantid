// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid {
namespace Crystal {
/** FindUBUsingIndexedPeaks : Algorithm to calculate a UB matrix,
    given a list of peaks that have already been indexed by some means.

    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17
  */
class MANTID_CRYSTAL_DLL FindUBUsingIndexedPeaks final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "FindUBUsingIndexedPeaks"; };

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "FindUBUsingFFT", "FindUBUsingLatticeParameters"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\UBMatrix"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the UB matrix from a peaks workspace, containing indexed "
           "peaks.";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
  void logLattice(const Geometry::OrientedLattice &o_lattice, const int &ModDim);
  int getModulationDimension(Kernel::V3D &mnp);
  bool isPeakIndexed(const Geometry::IPeak &peak);
};

} // namespace Crystal
} // namespace Mantid
