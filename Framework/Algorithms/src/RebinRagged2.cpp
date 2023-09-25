// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RebinRagged2.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RebinRagged)

using namespace API;
using namespace Kernel;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RebinRagged::name() const { return "RebinRagged"; }

/// Algorithm's version for identification. @see Algorithm::version
int RebinRagged::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RebinRagged::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RebinRagged::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RebinRagged::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input), "input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output), "output workspace");

  declareProperty(std::make_unique<ArrayProperty<double>>("XMin"), "minimum x values with NaN meaning no minimum");
  declareProperty(std::make_unique<ArrayProperty<double>>("XMax"), "maximum x values with NaN meaning no maximum");
  declareProperty(std::make_unique<ArrayProperty<double>>("Delta"), "step parameter for rebin");
  declareProperty("PreserveEvents", true, "False converts event workspaces to histograms");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RebinRagged::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  bool PreserveEvents = getProperty("PreserveEvents");

  // Rebinning in-place
  bool inPlace = (inputWS == outputWS);

  // workspace independent determination of length
  const auto histnumber = static_cast<int>(inputWS->getNumberHistograms());

  std::vector<double> xmins = getProperty("XMin");
  std::vector<double> xmaxs = getProperty("XMax");
  std::vector<double> deltas = getProperty("Delta");

  xmins.resize(histnumber, xmins[0]);
  xmaxs.resize(histnumber, xmaxs[0]);
  deltas.resize(histnumber, deltas[0]);

  // Now, determine if the input workspace is an EventWorkspace
  EventWorkspace_const_sptr eventInputWS = std::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventInputWS) {

    if (PreserveEvents) {
      if (!inPlace) {
        outputWS = inputWS->clone();
      }
      auto eventOutputWS = std::dynamic_pointer_cast<EventWorkspace>(outputWS);

      for (int i = 0; i < histnumber; i++) {
        auto xmin = xmins[i];
        auto xmax = xmaxs[i];
        const auto delta = deltas[i];

        const auto inX = eventInputWS->x(i);
        if (std::isnan(xmin))
          xmin = inX.front();
        if (std::isnan(xmax))
          xmax = inX.back();

        HistogramData::BinEdges XValues_new(0);
        static_cast<void>(VectorHelper::createAxisFromRebinParams({xmin, delta, xmax}, XValues_new.mutableRawData()));
        EventList &el = eventOutputWS->getSpectrum(i);
        el.setHistogram(XValues_new);
      }
    } else {
      throw Kernel::Exception::NotImplementedError("TODO: event to histogram");
    }
  } else {
    throw Kernel::Exception::NotImplementedError("TODO: histgmra");
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
