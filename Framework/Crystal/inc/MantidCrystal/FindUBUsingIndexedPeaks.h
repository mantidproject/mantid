// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_H_
#define MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** FindUBUsingIndexedPeaks : Algorithm to calculate a UB matrix,
    given a list of peaks that have already been indexed by some means.

    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17
  */
class DLLExport FindUBUsingIndexedPeaks : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "FindUBUsingIndexedPeaks"; };

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "FindUBUsingFFT", "FindUBUsingLatticeParameters",
            "FindUBUsingMinMaxD"};
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
  void logLattice(Geometry::OrientedLattice &o_lattice, int &ModDim);
  int getModulationDimension(Kernel::V3D &mnp);
  bool isPeakIndexed(DataObjects::Peak &peak);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS */
