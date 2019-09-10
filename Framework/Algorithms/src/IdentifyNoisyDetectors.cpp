// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/IdentifyNoisyDetectors.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/CompositeValidator.h"
#include <numeric>

namespace Mantid {
namespace Algorithms {
using namespace Kernel;
using namespace API;
using namespace HistogramData;
using namespace DataObjects;

DECLARE_ALGORITHM(IdentifyNoisyDetectors)

void IdentifyNoisyDetectors::init() {
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<WorkspaceUnitValidator>("TOF");
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  wsVal->add<InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "InputWorkspace", "", Direction::Input /*,wsVal*/));
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "OutputWorkspace", "", Direction::Output));
  declareProperty("RangeLower", 2000.0, "The lower integration range");
  declareProperty("RangeUpper", 19000.0, "The upper integration range");
}

void IdentifyNoisyDetectors::exec() {
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  const auto nHist = static_cast<int>(inputWS->getNumberHistograms());

  const double rangeLower = getProperty("RangeLower");
  const double rangeUpper = getProperty("RangeUpper");
  const double steps = rangeUpper - rangeLower;

  if (0 == nHist)
    throw std::runtime_error(
        "Cannot run this algorithm on an input workspace without any spectra. "
        "It does not seem to make sense and the calculations done here will "
        "will cause a division by zero.");

  Progress progress(this, 0.0, 1.0, (nHist * 7) + 6);

  // Create the output workspace a single value for each spectra.
  MatrixWorkspace_sptr outputWs;
  outputWs = create<MatrixWorkspace>(*inputWS, Points(1));

  MatrixWorkspace_sptr stdDevWs;
  stdDevWs = create<MatrixWorkspace>(*outputWs);

  progress.report("Integrating...");

  IAlgorithm_sptr integ = createChildAlgorithm("Integration");
  integ->initialize();
  integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWs);
  integ->setProperty<double>("RangeLower", rangeLower);
  integ->setProperty<double>("RangeUpper", rangeUpper);
  integ->execute();

  MatrixWorkspace_sptr int1 = integ->getProperty("OutputWorkspace");

  progress.report("Power...");

  IAlgorithm_sptr power = createChildAlgorithm("Power");
  power->initialize();
  power->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWs);
  power->setProperty<double>("Exponent", 2.0);
  power->execute();

  MatrixWorkspace_sptr power_tmp = power->getProperty("OutputWorkspace");

  progress.report("Integrating...");

  // integrate again
  integ = createChildAlgorithm("Integration");
  integ->initialize();
  integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", power_tmp);
  integ->setProperty<double>("RangeLower", rangeLower);
  integ->setProperty<double>("RangeUpper", rangeUpper);
  integ->execute();

  MatrixWorkspace_sptr int2 = integ->getProperty("OutputWorkspace");

  progress.report("Creating single valued workspace...");

  IAlgorithm_sptr csvw = createChildAlgorithm("CreateSingleValuedWorkspace");
  csvw->initialize();
  csvw->setProperty<double>("DataValue", steps);
  csvw->execute();

  MatrixWorkspace_sptr stepsWs = csvw->getProperty("OutputWorkspace");

  progress.report("Dividing...");

  IAlgorithm_sptr divide = createChildAlgorithm("Divide");
  divide->initialize();
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", int1);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", stepsWs);
  divide->execute();

  int1 = divide->getProperty("OutputWorkspace");

  progress.report("Dividing...");

  divide = createChildAlgorithm("Divide");
  divide->initialize();
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", int2);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", stepsWs);
  divide->execute();

  int2 = divide->getProperty("OutputWorkspace");

  for (int i = 0; i < nHist; i++) {
    outputWs->setHistogram(i, Points{0.0}, Counts{1.0});
    stdDevWs->setSharedX(i, outputWs->sharedX(i));
    stdDevWs->mutableY(i)[0] = sqrt(int2->y(i)[0] - std::pow(int1->y(i)[0], 2));

    progress.report();
  }

  getStdDev(progress, outputWs, stdDevWs);
  getStdDev(progress, outputWs, stdDevWs);
  getStdDev(progress, outputWs, stdDevWs);

  setProperty("OutputWorkspace", outputWs);
}

/**
 * Main work portion of algorithm. Calculates mean of standard deviation,
 * ignoring
 * the detectors marked as "bad", then determines if any of the detectors are
 * "bad".
 * @param progress :: progress indicator
 * @param valid :: eventual output workspace, holding 0 for bad and 1 for good
 * @param values :: stddeviations of each spectra (I think)
 */
void IdentifyNoisyDetectors::getStdDev(API::Progress &progress,
                                       MatrixWorkspace_sptr valid,
                                       MatrixWorkspace_sptr values) {
  const auto nhist = static_cast<int>(valid->getNumberHistograms());
  int count = 0;
  double mean = 0.0;
  double mean2 = 0.0;

  for (int i = 0; i < nhist; i++) {
    if (valid->y(i)[0] > 0) {
      mean += values->y(i)[0];
      mean2 += std::pow(values->y(i)[0], 2);
      count++;
    }

    progress.report();
  }

  if (0 == count) {
    // all values are zero, no need to loop
    return;
  }

  mean = mean / count;
  double stddev = sqrt((mean2 / count) - std::pow(mean, 2));

  double upper = mean + 3 * stddev;
  double lower = mean - 3 * stddev;
  double min = mean * 0.0001;

  Counts counts{0.0};
  for (int i = 0; i < nhist; i++) {
    double value = values->y(i)[0];
    if (value > upper) {
      valid->setCounts(i, counts);
    } else if (value < lower) {
      valid->setCounts(i, counts);
    } else if (value < min) {
      valid->setCounts(i, counts);
    }

    progress.report("Calculating StdDev...");
  }
}

} // namespace Algorithms
} // namespace Mantid