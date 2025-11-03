// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace Algorithms {
/** Smooths the data of the input workspace by making each point the mean
   average of itself and
    one or more points lying symmetrically either side of it.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
   </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> NPoints - The number of points to average over. Must be at least 3 (the
   default). If
         an even number is given, it will be incremented by 1 to make it
   odd.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 23/10/2008
*/
class MANTID_ALGORITHMS_DLL SmoothData final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SmoothData"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Smooths out statistical fluctuations in a workspace's data."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SmoothNeighbours"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Smoothing"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  int validateSpectrumInGroup(size_t wi);
  // This map does not need to be ordered, just a lookup for udet
  /// type alias for the storage of the UDET-group mapping
  using udet2groupmap = std::map<detid_t, int>;
  std::vector<int> udet2group;
  API::MatrixWorkspace_const_sptr inputWorkspace;
};

HistogramData::Histogram smooth(const HistogramData::Histogram &histogram, unsigned int const npts);

} // namespace Algorithms
} // namespace Mantid
