#ifndef MANTID_ALGORITHMS_SOFQW_H_
#define MANTID_ALGORITHMS_SOFQW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/SofQCommon.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {
/** Converts a 2D workspace that has axes of energy transfer against spectrum
number to
one that gives intensity as a function of momentum transfer against energy.

Required Properties:
<UL>
<LI> InputWorkspace  - Reduced data in units of energy transfer. Must have
common bins. </LI>
<LI> OutputWorkspace - The name to use for the q-w workspace. </LI>
<LI> QAxisBinning    - The bin parameters to use for the q axis. </LI>
<LI> Emode           - The energy mode (direct or indirect geometry). </LI>
<LI> Efixed          - Value of fixed energy: EI (emode=1) or EF (emode=2)
(meV). </LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/02/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

struct SofQCommon;

class DLLExport SofQW : public API::DataProcessorAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SofQW"; }
  /// Summary of algorithms purpose
  const std::string summary() const override;

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SofQWCentre", "SofQWPolygon", "SofQWNormalisedPolygon", "Rebin2D"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\SofQW"; }
  /// Create the output workspace
  template <typename Workspace>
  static std::unique_ptr<Workspace> setUpOutputWorkspace(
      const API::MatrixWorkspace &inputWorkspace,
      const std::vector<double> &qbinParams, std::vector<double> &qAxis,
      const std::vector<double> &ebinParams, const SofQCommon &emodeProperties);
  /// Create the input properties on the given algorithm object
  static void createCommonInputProperties(API::Algorithm &alg);

private:
  /// Initialization code
  void init() override;
  /// Execution code
  void exec() override;
};

/** Creates the output workspace, setting the axes according to the input
 * binning parameters
 *  @tparam     Workspace The type of the workspace to create
 *  @param[in]  inputWorkspace The input workspace
 *  @param[in]  qbinParams The q-bin parameters from the user
 *  @param[out] qAxis The 'vertical' (q) axis defined by the given parameters
 *  @param[out] ebinParams The 'horizontal' (energy) axis parameters (optional)
 *  @param[in]  emodeProperties The initialized SofQCommon object corresponding
 *              to the input workspace and calling algorithm
 *  @return A pointer to the newly-created workspace
 */
template <typename Workspace>
std::unique_ptr<Workspace> SofQW::setUpOutputWorkspace(
    const API::MatrixWorkspace &inputWorkspace,
    const std::vector<double> &qbinParams, std::vector<double> &qAxis,
    const std::vector<double> &ebinParams, const SofQCommon &emodeProperties) {
  using Kernel::VectorHelper::createAxisFromRebinParams;
  // Create vector to hold the new X axis values
  HistogramData::BinEdges xAxis(0);
  double eMin{std::nan("")};
  double eMax{std::nan("")};
  if (ebinParams.empty()) {
    xAxis = inputWorkspace.binEdges(0);
  } else if (ebinParams.size() == 1) {
    inputWorkspace.getXMinMax(eMin, eMax);
    createAxisFromRebinParams(ebinParams, xAxis.mutableRawData(), true, true,
                              eMin, eMax);
  } else {
    createAxisFromRebinParams(ebinParams, xAxis.mutableRawData());
  }
  // Create a vector to temporarily hold the vertical ('y') axis and populate
  // that
  int yLength;
  if (qbinParams.size() == 1) {
    if (std::isnan(eMin)) {
      inputWorkspace.getXMinMax(eMin, eMax);
    }
    double qMin;
    double qMax;
    std::tie(qMin, qMax) =
        emodeProperties.qBinHints(inputWorkspace, eMin, eMax);
    yLength =
        createAxisFromRebinParams(qbinParams, qAxis, true, true, qMin, qMax);
  } else {
    yLength = createAxisFromRebinParams(qbinParams, qAxis);
  }

  // Create output workspace, bin edges are same as in inputWorkspace index 0
  auto outputWorkspace =
      DataObjects::create<Workspace>(inputWorkspace, yLength - 1, xAxis);

  // Create a binned numeric axis to replace the default vertical one
  API::Axis *const verticalAxis = new API::BinEdgeAxis(qAxis);
  outputWorkspace->replaceAxis(1, verticalAxis);

  // Set the axis units
  verticalAxis->unit() =
      Kernel::UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWorkspace->getAxis(0)->title() = "Energy transfer";

  outputWorkspace->setYUnit("");
  outputWorkspace->setYUnitLabel("Intensity");

  return outputWorkspace;
}

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOFQW_H_*/
