// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RebinByTimeBase.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Types::Core::DateAndTime;

/**
 Helper method to transform a MantidVector containing absolute times in
 nanoseconds to relative times in seconds given an offset.
 */
class ConvertToRelativeTime
    : public std::unary_function<const MantidVec::value_type &,
                                 MantidVec::value_type> {
private:
  double m_offSet;

public:
  explicit ConvertToRelativeTime(const DateAndTime &offSet)
      : m_offSet(static_cast<double>(offSet.totalNanoseconds()) * 1e-9) {}
  MantidVec::value_type operator()(const MantidVec::value_type &absTNanoSec) {
    return (absTNanoSec * 1e-9) - m_offSet;
  }
};

/** Initialize the algorithm's properties.
 */
void RebinByTimeBase::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "An input workspace containing TOF events.");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Params", boost::make_shared<RebinParamsValidator>()),
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. Optionally\n"
                  "this can be followed by a comma and more widths and last "
                  "boundary pairs. Values are in seconds since run start.");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

/**
 * Execute the algorithm.
 */
void RebinByTimeBase::exec() {
  using Mantid::DataObjects::EventWorkspace;
  IEventWorkspace_sptr inWS = getProperty("InputWorkspace");

  if (!boost::dynamic_pointer_cast<EventWorkspace>(inWS)) {
    const std::string algName = this->name();
    throw std::invalid_argument(
        algName + " Algorithm requires an EventWorkspace as an input.");
  }

  // retrieve the properties
  const std::vector<double> inParams = getProperty("Params");
  std::vector<double> rebinningParams;

  // workspace independent determination of length
  const auto histnumber = static_cast<int>(inWS->getNumberHistograms());

  const auto nanoSecondsInASecond = static_cast<uint64_t>(1e9);
  const DateAndTime runStartTime = inWS->run().startTime();
  // The validator only passes parameters with size 1, or 3xn.

  double tStep = 0;
  if (inParams.size() >= 3) {
    // Use the start of the run to offset the times provided by the user. pulse
    // time of the events are absolute.
    const DateAndTime startTime = runStartTime + inParams[0];
    const DateAndTime endTime = runStartTime + inParams[2];
    // Rebinning params in nanoseconds.
    rebinningParams.push_back(
        static_cast<double>(startTime.totalNanoseconds()));
    tStep = inParams[1] * nanoSecondsInASecond;
    rebinningParams.push_back(tStep);
    rebinningParams.push_back(static_cast<double>(endTime.totalNanoseconds()));
  } else if (inParams.size() == 1) {
    const uint64_t xmin = getMinX(inWS);
    const uint64_t xmax = getMaxX(inWS);

    rebinningParams.push_back(static_cast<double>(xmin));
    tStep = inParams[0] * nanoSecondsInASecond;
    rebinningParams.push_back(tStep);
    rebinningParams.push_back(static_cast<double>(xmax));
  }

  // Validate the timestep.
  if (tStep <= 0) {
    throw std::invalid_argument(
        "Cannot have a timestep less than or equal to zero.");
  }

  // Initialize progress reporting.
  Progress prog(this, 0.0, 1.0, histnumber);

  MantidVecPtr XValues_new;
  // create new X axis, with absolute times in seconds.
  static_cast<void>(VectorHelper::createAxisFromRebinParams(
      rebinningParams, XValues_new.access()));

  ConvertToRelativeTime transformToRelativeT(runStartTime);

  // Transform the output into relative times in seconds.
  MantidVec OutXValues_scaled(XValues_new->size());
  std::transform(XValues_new->begin(), XValues_new->end(),
                 OutXValues_scaled.begin(), transformToRelativeT);

  MatrixWorkspace_sptr outputWS = DataObjects::create<DataObjects::Workspace2D>(
      *inWS, histnumber, HistogramData::BinEdges(*XValues_new));

  // Copy all the axes
  for (int i = 1; i < inWS->axes(); i++) {
    outputWS->replaceAxis(
        i, std::unique_ptr<Axis>(inWS->getAxis(i)->clone(outputWS.get())));
    outputWS->getAxis(i)->unit() = inWS->getAxis(i)->unit();
  }

  // X-unit is relative time since the start of the run.
  outputWS->getAxis(0)->unit() = boost::make_shared<Units::Time>();

  // Copy the units over too.
  for (int i = 1; i < outputWS->axes(); ++i) {
    outputWS->getAxis(i)->unit() = inWS->getAxis(i)->unit();
  }
  outputWS->setYUnit(inWS->YUnit());
  outputWS->setYUnitLabel(inWS->YUnitLabel());

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);

  // Go through all the histograms and set the data
  doHistogramming(inWS, outputWS, XValues_new, OutXValues_scaled, prog);
}

} // namespace Algorithms
} // namespace Mantid
