// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidIndexing/SpectrumIndexSet.h"

namespace Mantid {

namespace Histogram {
class BinEdges;
}
namespace Algorithms {
/** Masks bins in a workspace. Bins falling within the range given (even
   partially) are
    masked, i.e. their data and error values are set to zero and the bin is
   added to the
    list of masked bins. This range is masked for all spectra in the workspace
   (though the
    workspace does not have to have common X values in all spectra).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name of the Workspace containing the masked bins.
   </LI>
    <LI> XMin - The value to start masking from.</LI>
    <LI> XMax - The value to end masking at.</LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 29/04/2009
 */
class MANTID_ALGORITHMS_DLL MaskBins : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MaskBins"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Marks bins in a workspace as being masked."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"MaskBinsIf", "MaskBinsFromTable", "MaskNonOverlappingBins"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();

  void findIndices(const HistogramData::BinEdges &X, MantidVec::difference_type &startBin,
                   MantidVec::difference_type &endBin);

  double m_startX{0.0};                ///< The range start point
  double m_endX{0.0};                  ///< The range end point
  Indexing::SpectrumIndexSet indexSet; ///< the list of Spectra (workspace index) to load
};

} // namespace Algorithms
} // namespace Mantid
