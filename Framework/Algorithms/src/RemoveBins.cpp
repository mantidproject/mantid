// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveBins.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RemoveBins)

RemoveBins::RemoveBins()
    : API::Algorithm(), m_inputWorkspace(), m_spectrumInfo(nullptr),
      m_startX(DBL_MAX), m_endX(-DBL_MAX), m_rangeUnit(), m_interpolate(false) {
}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void RemoveBins::init() {
  auto wsValidator = boost::make_shared<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name of the output workspace.");

  auto mustHaveValue = boost::make_shared<MandatoryValidator<double>>();
  declareProperty("XMin", Mantid::EMPTY_DBL(), mustHaveValue,
                  "The lower bound of the region to be removed.");
  declareProperty("XMax", Mantid::EMPTY_DBL(), mustHaveValue,
                  "The upper bound of the region to be removed.");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();

  // remove some known units that will not work
  units.erase(std::remove(units.begin(), units.end(), "Empty"), units.end());
  units.erase(std::remove(units.begin(), units.end(), "Label"), units.end());
  units.erase(std::remove(units.begin(), units.end(), "Time"), units.end());
  units.erase(std::remove(units.begin(), units.end(), "Degrees"), units.end());

  // add a default do nothing value
  units.insert(units.begin(), "AsInput");
  declareProperty("RangeUnit", "AsInput",
                  boost::make_shared<StringListValidator>(units),
                  "The unit in which XMin/XMax are being given. If not given, "
                  "it will peak the unit from the Input workspace X unit.");

  std::vector<std::string> propOptions{"None", "Linear"};
  declareProperty("Interpolation", "None",
                  boost::make_shared<StringListValidator>(propOptions),
                  "Whether mid-axis bins should be interpolated linearly "
                  "(\"Linear\") or set to zero (\"None\"). Note: Used when the "
                  "region to be removed is within a bin. Linear scales the "
                  "value in that bin by the proportion of it that is outside "
                  "the region to be removed and none sets it to zero");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "If set, will remove data only in the given spectrum of the "
                  "workspace. Otherwise, all spectra will be acted upon.");
}

/** Checks cross property validation
 *   @returns a map of PropertyName->ErrorMessage
 */
std::map<std::string, std::string> RemoveBins::validateInputs() {
  std::map<std::string, std::string> result;
  const std::string rangeUnit = getProperty("RangeUnit");

  // Get input workspace
  m_inputWorkspace = getProperty("InputWorkspace");

  // If that was OK, then we can get their values
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX) {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    result["XMax"] = failure;
  }

  // If WorkspaceIndex has been set it must be valid
  const int index = getProperty("WorkspaceIndex");
  if (!isEmpty(index) &&
      index >= static_cast<int>(m_inputWorkspace->getNumberHistograms())) {
    std::stringstream failureMsg;
    failureMsg << "The value of WorkspaceIndex provided (" << index
               << ") is larger than the size of this workspace ("
               << m_inputWorkspace->getNumberHistograms() << ")";
    g_log.error(failureMsg.str());
    result["WorkspaceIndex"] = failureMsg.str();
  }

  const std::string interpolation = getProperty("Interpolation");
  m_interpolate = (interpolation == "Linear");

  const bool unitChange = (rangeUnit != "AsInput");
  if (unitChange) {
    std::string errorString = "";
    if (m_inputWorkspace->axes() == 0)
      errorString =
          "A single valued workspace has no unit, which is required for "
          "this algorithm";

    Kernel::Unit_const_sptr unit = m_inputWorkspace->getAxis(0)->unit();
    // If m_unitID is empty it means that the workspace must have units, which
    // can be anything
    if (unit && (!boost::dynamic_pointer_cast<const Kernel::Unit>(unit))) {
      errorString =
          "The workspace must have units if the RangeUnit is not \"AsInput\"";
    }
    if (!errorString.empty()) {
      g_log.error() << "InputWorkspace: " << errorString << "\n";
      result["InputWorkspace"] = errorString;
    }
  }
  return result;
}

/** Executes the algorithm
 *
 */
void RemoveBins::exec() {
  // If the X range has been given in a different unit, or if the workspace
  // isn't square, then we will need
  // to calculate the bin indices to cut out each time.
  const std::string rangeUnit = getProperty("RangeUnit");
  const bool unitChange =
      (rangeUnit != "AsInput" &&
       rangeUnit != m_inputWorkspace->getAxis(0)->unit()->unitID());
  if (unitChange)
    m_rangeUnit = UnitFactory::Instance().create(rangeUnit);
  const bool commonBins = m_inputWorkspace->isCommonBins();
  const int index = getProperty("WorkspaceIndex");
  const bool singleSpectrum = !isEmpty(index);
  const bool recalcRange = (unitChange || !commonBins);

  // If the above evaluates to false, and the range given is at the edge of the
  // workspace, then we can just call
  // CropWorkspace as a ChildAlgorithm and we're done.
  auto &X0 = m_inputWorkspace->x(0);
  if (!singleSpectrum && !recalcRange &&
      (m_startX <= X0.front() || m_endX >= X0.back())) {
    double start, end;
    if (m_startX <= X0.front()) {
      start = m_endX;
      end = X0.back();
    } else {
      start = X0.front();
      end = m_startX;
    }

    try {
      this->crop(start, end);
      return;
    } catch (...) {
    } // If this fails for any reason, just carry on and do it the other way
  }

  m_spectrumInfo = &m_inputWorkspace->spectrumInfo();

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (m_inputWorkspace !=
      outputWS) // Create the output workspace only if not the same as input
  {
    outputWS = WorkspaceFactory::Instance().create(m_inputWorkspace);
  }

  // Loop over the spectra
  int start = 0, end = 0;
  const auto blockSize = static_cast<int>(m_inputWorkspace->x(0).size());
  const auto numHists =
      static_cast<int>(m_inputWorkspace->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numHists);
  for (int i = 0; i < numHists; ++i) {
    outputWS->setHistogram(i, m_inputWorkspace->histogram(i));

    // If just operating on a single spectrum and this isn't it, go to next
    // iteration
    if (singleSpectrum && (i != index))
      continue;

    double startX(m_startX), endX(m_endX);
    // Calculate the X limits for this spectrum, if necessary
    if (unitChange) {
      this->transformRangeUnit(i, startX, endX);
    }

    auto &X = m_inputWorkspace->x(i);
    auto &outY = outputWS->mutableY(i);
    auto &outE = outputWS->mutableE(i);
    // Calculate the bin indices corresponding to the X range, if necessary
    if (recalcRange || singleSpectrum || !i) {
      start = this->findIndex(startX, X);
      end = this->findIndex(endX, X);
    }

    if (start == 0 || end == blockSize) {
      // Remove bins from either end
      this->RemoveFromEnds(start, end, outY, outE);
    } else {
      // Remove bins from middle
      const double startFrac = (X[start] - startX) / (X[start] - X[start - 1]);
      const double endFrac = (endX - X[end - 1]) / (X[end] - X[end - 1]);
      this->RemoveFromMiddle(start - 1, end, startFrac, endFrac, outY, outE);
    }
    prog.report();
  } // Loop over spectra

  // Assign to the output workspace property
  setProperty("OutputWorkspace", outputWS);
  m_inputWorkspace.reset();
}

/// Calls CropWorkspace as a Child Algorithm to remove bins from the start or
/// end of a square workspace
void RemoveBins::crop(const double &start, const double &end) {
  IAlgorithm_sptr childAlg = createChildAlgorithm("CropWorkspace");
  childAlg->setProperty<MatrixWorkspace_sptr>(
      "InputWorkspace",
      boost::const_pointer_cast<MatrixWorkspace>(m_inputWorkspace));
  childAlg->setProperty<double>("XMin", start);
  childAlg->setProperty<double>("XMax", end);
  childAlg->executeAsChildAlg();

  // Only get to here if successful
  // Assign the result to the output workspace property
  MatrixWorkspace_sptr outputWS = childAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWS);
}

/** Convert the X range given into the unit of the input workspace
 *  @param index ::  The current workspace index
 *  @param startX :: Returns the start of the range in the workspace's unit
 *  @param endX ::   Returns the end of the range in the workspace's unit
 */
void RemoveBins::transformRangeUnit(const int index, double &startX,
                                    double &endX) {
  const Kernel::Unit_sptr inputUnit = m_inputWorkspace->getAxis(0)->unit();
  // First check for a 'quick' conversion
  double factor, power;
  if (m_rangeUnit->quickConversion(*inputUnit, factor, power)) {
    startX = factor * std::pow(m_startX, power);
    endX = factor * std::pow(m_endX, power);
  } else {
    double l1, l2, theta;
    this->calculateDetectorPosition(index, l1, l2, theta);
    std::vector<double> endPoints;
    endPoints.push_back(startX);
    endPoints.push_back(endX);
    std::vector<double> emptyVec;
    m_rangeUnit->toTOF(endPoints, emptyVec, l1, l2, theta, 0, 0.0, 0.0);
    inputUnit->fromTOF(endPoints, emptyVec, l1, l2, theta, 0, 0.0, 0.0);
    startX = endPoints.front();
    endX = endPoints.back();
  }

  if (startX > endX) {
    const double temp = startX;
    startX = endX;
    endX = temp;
  }

  g_log.debug() << "For index " << index << ", X range given corresponds to "
                << startX << "-" << endX << " in workspace's unit\n";
}

/** Retrieves the detector postion for a given spectrum
 *  @param index ::    The workspace index of the spectrum
 *  @param l1 ::       Returns the source-sample distance
 *  @param l2 ::       Returns the sample-detector distance
 *  @param twoTheta :: Returns the detector's scattering angle
 */
void RemoveBins::calculateDetectorPosition(const int index, double &l1,
                                           double &l2, double &twoTheta) {
  l1 = m_spectrumInfo->l1();
  l2 = m_spectrumInfo->l2(index);
  if (m_spectrumInfo->isMonitor(index))
    twoTheta = 0.0;
  else
    twoTheta = m_spectrumInfo->twoTheta(index);

  g_log.debug() << "Detector for index " << index << " has L1+L2=" << l1 + l2
                << " & 2theta= " << twoTheta << '\n';
}

/** Finds the index in an ordered vector which follows the given value
 *  @param value :: The value to search for
 *  @param vec ::   The vector to search
 *  @return The index (will give vec.size()+1 if the value is past the end of
 * the vector)
 */
int RemoveBins::findIndex(const double &value, const HistogramX &vec) {
  auto pos = std::lower_bound(vec.cbegin(), vec.cend(), value);
  return static_cast<int>(std::distance(vec.cbegin(), pos));
}

/** Zeroes data (Y/E) at the end of a spectrum
 *  @param start :: The index to start zeroing at
 *  @param end ::   The index to end zeroing at
 *  @param Y ::     The data vector
 *  @param E ::     The error vector
 */
void RemoveBins::RemoveFromEnds(int start, int end, HistogramY &Y,
                                HistogramE &E) {
  if (start)
    --start;
  auto size = static_cast<int>(Y.size());
  if (end > size)
    end = size;
  for (int j = start; j < end; ++j) {
    Y[j] = 0.0;
    E[j] = 0.0;
  }
}

/** Removes bins in the middle of the data (Y/E).
 *  According to the value of the Interpolation property, they are either zeroed
 * or the gap is interpolated linearly.
 *  If the former, the edge bins will be scaled according to how much of them
 * falls within the range being removed.
 *  @param start ::     The first index to remove
 *  @param end ::       The last index to remove
 *  @param startFrac :: The fraction of the first bin that's outside the range
 * being zeroed
 *  @param endFrac ::   The fraction of the last bin that's outside the range
 * being zeroed
 *  @param Y ::         The data vector
 *  @param E ::         The error vector
 */
void RemoveBins::RemoveFromMiddle(const int &start, const int &end,
                                  const double &startFrac,
                                  const double &endFrac, HistogramY &Y,
                                  HistogramE &E) {
  // Remove bins from middle
  double valPrev = 0;
  double valNext = 0;
  double errPrev = 0;
  double errNext = 0;

  // Values for interpolation
  if (m_interpolate) {
    valPrev = Y[start - 1];
    valNext = Y[end];
    errPrev = E[start - 1];
    errNext = E[end];
  }

  const double m =
      (valNext - valPrev) / (1.0 * (end - start) + 2.0); // Gradient
  const double c = valPrev;                              // Intercept

  double aveE = (errPrev + errNext) / 2; // Cheat: will do properly later

  for (int j = start; j < end; ++j) {
    if (!m_interpolate && j == start) {
      Y[j] *= startFrac;
      E[j] *= startFrac;
    } else if (!m_interpolate && j == end - 1) {
      Y[j] *= endFrac;
      E[j] *= endFrac;
    } else {
      Y[j] = m * (j - start + 1) + c;
      E[j] = aveE;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
