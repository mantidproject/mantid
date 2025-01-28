// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskBinsIf.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/MultiThreaded.h"

#include <muParser.h>

namespace {

/**
 * @brief Makes a new instance of the muparser
 * @param y : count
 * @param e : error
 * @param x : bin center
 * @param dx : bin center error
 * @param s : spectrum axis value
 * @param criterion : expression
 * @return muparser
 */
mu::Parser makeParser(double &y, double &e, double &x, double &dx, double &s, const std::string &criterion) {
  mu::Parser muParser;
  muParser.DefineVar("y", &y);
  muParser.DefineVar("e", &e);
  muParser.DefineVar("x", &x);
  muParser.DefineVar("dx", &dx);
  muParser.DefineVar("s", &s);
  muParser.SetExpr(criterion);
  return muParser;
}
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBinsIf)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaskBinsIf::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty("Criterion", "",
                  "Masking criterion as a muparser expression; y: bin count, "
                  "e: bin error, x: bin center, dx: bin center error, s: "
                  "spectrum axis value.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Validate the inputs.
 */
std::map<std::string, std::string> MaskBinsIf::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::string criterion = getPropertyValue("Criterion");
  if (criterion.empty()) {
    issues["Criterion"] = "The criterion expression provided is empty";
  } else {
    double y = 0., e = 0., x = 0., dx = 0., s = 0.;
    mu::Parser parser = makeParser(y, e, x, dx, s, criterion);
    try {
      parser.Eval();
    } catch (mu::Parser::exception_type &exception) {
      issues["Criterion"] = "Invalid expression given: " + exception.GetMsg();
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MaskBinsIf::exec() {
  const std::string criterion = getPropertyValue("Criterion");
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");
  if (inputWorkspace != outputWorkspace) {
    outputWorkspace = inputWorkspace->clone();
  }
  const auto verticalAxis = outputWorkspace->getAxis(1);
  const auto *numericAxis = dynamic_cast<NumericAxis *>(verticalAxis);
  const auto *spectrumAxis = dynamic_cast<SpectraAxis *>(verticalAxis);
  const bool spectrumOrNumeric = numericAxis || spectrumAxis;
  if (!spectrumOrNumeric) {
    throw std::runtime_error("Vertical axis must be NumericAxis or SpectraAxis");
  }
  const auto numberHistograms = static_cast<int64_t>(outputWorkspace->getNumberHistograms());
  auto progress = std::make_unique<Progress>(this, 0., 1., numberHistograms);
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*outputWorkspace))
  for (int64_t index = 0; index < numberHistograms; ++index) {
    PARALLEL_START_INTERRUPT_REGION
    double y, e, x, dx;
    double s = verticalAxis->getValue(index);
    mu::Parser parser = makeParser(y, e, x, dx, s, criterion);
    const auto &spectrum = outputWorkspace->histogram(index);
    const bool hasDx = outputWorkspace->hasDx(index);
    for (auto it = spectrum.begin(); it != spectrum.end(); ++it) {
      const auto bin = std::distance(spectrum.begin(), it);
      y = it->counts();
      x = it->center();
      e = it->countStandardDeviation();
      dx = hasDx ? it->centerError() : 0.;
      if (parser.Eval() != 0.) {
        outputWorkspace->maskBin(index, bin);
      }
    }
    progress->report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Mantid::Algorithms
