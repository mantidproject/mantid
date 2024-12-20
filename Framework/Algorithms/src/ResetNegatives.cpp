// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ResetNegatives.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ResetNegatives)

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::IAlgorithm::name()
const std::string ResetNegatives::name() const { return "ResetNegatives"; }

/// @copydoc Mantid::API::IAlgorithm::version()
int ResetNegatives::version() const { return 1; }

/// @copydoc Mantid::API::IAlgorithm::category()
const std::string ResetNegatives::category() const { return "CorrectionFunctions\\SpecialCorrections"; }

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::init()
void ResetNegatives::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input), "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  declareProperty("AddMinimum", true, "Add the minimum value of the spectrum to bring it up to zero.");
  declareProperty("ResetValue", 0., "Reset negative values to this number (default=0)");
  setPropertySettings("ResetValue", std::make_unique<EnabledWhenProperty>("AddMinimum", IS_NOT_DEFAULT));
}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::exec()
void ResetNegatives::exec() {
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

  // get the minimum for each spectrum
  auto alg = createChildAlgorithm("Min", 0., .1);
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  alg->executeAsChildAlg();
  MatrixWorkspace_const_sptr minWS = alg->getProperty("OutputWorkspace");

  // determine if there is anything to do
  auto nHist = static_cast<int64_t>(minWS->getNumberHistograms());
  bool hasNegative = false;
  for (int64_t i = 0; i < nHist; i++) {
    if (minWS->y(i)[0] < 0.0) {
      hasNegative = true;
      break;
    }
  }

  // get out early if there is nothing to do
  if (!hasNegative) {
    g_log.information() << "No values are negative. Copying InputWorkspace to "
                           "OutputWorkspace\n";
    if (inputWS != outputWS) {
      auto clone = createChildAlgorithm("CloneWorkspace", .1, 1.);
      clone->setProperty<Workspace_sptr>("InputWorkspace", inputWS);
      clone->executeAsChildAlg();

      Workspace_sptr temp = clone->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(temp));
    }
    return;
  }

  // sort the event list to make it fast and thread safe
  DataObjects::EventWorkspace_const_sptr eventWS =
      std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWS);
  if (eventWS)
    eventWS->sortAll(DataObjects::TOF_SORT, nullptr);

  Progress prog(this, 0.1, 1.0, 2 * nHist);

  // generate output workspace - copy X and dY
  outputWS = API::WorkspaceFactory::Instance().create(inputWS);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < nHist; i++) {
    PARALLEL_START_INTERRUPT_REGION
    const auto index = static_cast<size_t>(i);
    outputWS->setHistogram(index, inputWS->histogram(index));
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // do the actual work
  if (this->getProperty("AddMinimum")) {
    this->pushMinimum(minWS, outputWS, prog);
  } else {
    this->changeNegatives(minWS, this->getProperty("ResetValue"), outputWS, prog);
  }

  setProperty("OutputWorkspace", outputWS);
}

namespace { // anonymous namespace
            /**
             * Convert -0. to 0.
             */
inline double fixZero(const double value) { return value != -0. ? value : 0.; }
} // namespace

/**
 * Add -1.*minValue on each spectra.
 *
 * @param minWS A workspace of minimum values for each spectra. This is
 *calculated in
 * the @see exec portion of the algorithm.
 * @param wksp The workspace to modify.
 * @param prog The progress.
 */
void ResetNegatives::pushMinimum(const MatrixWorkspace_const_sptr &minWS, const MatrixWorkspace_sptr &wksp,
                                 Progress &prog) {
  int64_t nHist = minWS->getNumberHistograms();
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp, *minWS))
  for (int64_t i = 0; i < nHist; i++) {
    PARALLEL_START_INTERRUPT_REGION
    double minValue = minWS->y(i)[0];
    if (minValue <= 0) {
      minValue *= -1.;
      auto &y = wksp->mutableY(i);
      std::transform(y.begin(), y.end(), y.begin(), [minValue](double value) { return fixZero(value + minValue); });
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * Search each spectrum for y-values that are less than zero and reset
 * them to the supplied value.
 *
 * @param minWS A workspace of minimum values for each spectra.
 * @param spectrumNegativeValues Reset negative values in the spectra to this
 * number.
 * @param wksp The workspace to modify.
 * @param prog The progress.
 */
void ResetNegatives::changeNegatives(const MatrixWorkspace_const_sptr &minWS, const double spectrumNegativeValues,
                                     const MatrixWorkspace_sptr &wksp, Progress &prog) {
  int64_t nHist = wksp->getNumberHistograms();
  PARALLEL_FOR_IF(Kernel::threadSafe(*minWS, *wksp))
  for (int64_t i = 0; i < nHist; i++) {
    PARALLEL_START_INTERRUPT_REGION
    if (minWS->y(i)[0] <= 0.) // quick check to see if there is a reason to bother
    {
      auto &y = wksp->mutableY(i);
      for (double &value : y) {
        if (value < 0.) {
          value = spectrumNegativeValues;
        } else
          value = fixZero(value);
      }
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

} // namespace Mantid::Algorithms
