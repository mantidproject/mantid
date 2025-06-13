// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/UnwrapMonitor.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(UnwrapMonitor)

using namespace Kernel;
using namespace API;

/// Default constructor
UnwrapMonitor::UnwrapMonitor()
    : m_conversionConstant(0.), m_inputWS(), m_LRef(0.), m_Tmin(0.), m_Tmax(0.), m_XSize(0) {}

/// Initialisation method
void UnwrapMonitor::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "A workspace with x values in units of TOF and y values in counts");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the algorithm");

  auto validator = std::make_shared<BoundedValidator<double>>();
  validator->setLower(0.01);
  declareProperty("LRef", 0.0, validator, "The length of the reference flight path (in metres)");

  declareProperty("JoinWavelength", 0.0, Direction::Output);

  // Calculate and set the constant factor for the conversion to wavelength
  const double TOFisinMicroseconds = 1e6;
  const double toAngstroms = 1e10;
  m_conversionConstant = (PhysicalConstants::h * toAngstroms) / (PhysicalConstants::NeutronMass * TOFisinMicroseconds);
}

/** Executes the algorithm
 *  @throw std::runtime_error if the workspace is invalid or a child algorithm
 *fails
 *  @throw Kernel::Exception::InstrumentDefinitionError if detector, source or
 *sample positions cannot be calculated
 *
 */
void UnwrapMonitor::exec() {
  // Get the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Get the number of spectra in this workspace
  const auto numberOfSpectra = static_cast<int>(m_inputWS->getNumberHistograms());
  g_log.debug() << "Number of spectra in input workspace: " << numberOfSpectra << '\n';

  // Get the "reference" flightpath (currently passed in as a property)
  m_LRef = getProperty("LRef");
  // Get the min & max frame values
  m_Tmin = m_inputWS->x(0).front();
  m_Tmax = m_inputWS->x(0).back();
  g_log.debug() << "Frame range in microseconds is: " << m_Tmin << " - " << m_Tmax << '\n';
  m_XSize = m_inputWS->x(0).size();

  // Need a new workspace. Will just be used temporarily until the data is
  // rebinned.
  MatrixWorkspace_sptr tempWS = WorkspaceFactory::Instance().create(m_inputWS, numberOfSpectra, m_XSize, m_XSize - 1);
  // Set the correct X unit on the output workspace
  tempWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

  // This will be used later to store the maximum number of bin BOUNDARIES for
  // the rebinning
  size_t max_bins = 0;

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const double L1 = spectrumInfo.l1();

  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numberOfSpectra);
  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i) {
    if (!spectrumInfo.hasDetectors(i)) {
      // If the detector flightpath is missing, zero the data
      g_log.debug() << "Detector information for workspace index " << i << " is not available.\n";
      tempWS->mutableX(i) = 0.0;
      tempWS->mutableY(i) = 0.0;
      tempWS->mutableE(i) = 0.0;
      continue;
    }

    // Getting the unwrapped data is a three step process.
    // 1. We ge the unwrapped version of the x data.
    // 2. Then we get the unwrapped version of the y and e data for
    //    which we require the x data from the previous step
    // 3. Finally we need to set the newly unwrapped data on the
    //    histogram

    // Unwrap the x data. Returns the bin ranges that end up being used
    const double Ld = L1 + spectrumInfo.l2(i);
    std::vector<double> newX;
    const std::vector<int> rangeBounds = this->unwrapX(newX, i, Ld);

    // Unwrap the y & e data according to the ranges found above
    std::vector<double> newY;
    std::vector<double> newE;
    this->unwrapYandE(tempWS, i, rangeBounds, newY, newE);

    // Now set the new X, Y and E values on the Histogram
    auto histogram = tempWS->histogram(i);
    tempWS->setHistogram(i, Mantid::HistogramData::BinEdges(std::move(newX)),
                         Mantid::HistogramData::Counts(std::move(newY)),
                         Mantid::HistogramData::CountStandardDeviations(std::move(newE)));

    // Get the maximum number of bins (excluding monitors) for the rebinning
    // below
    if (!spectrumInfo.isMonitor(i)) {
      const size_t XLen = tempWS->x(i).size();
      if (XLen > max_bins)
        max_bins = XLen;
    }
    m_progress->report();
  } // loop over spectra

  // Only rebin if more that just a single monitor spectrum in the input WS
  if (numberOfSpectra > 1) {
    // Calculate the minimum and maximum possible wavelengths for the rebinning
    const double minLambda = (m_conversionConstant * m_Tmin) / m_LRef;
    const double maxLambda = (m_conversionConstant * m_Tmax) / m_LRef;
    // Rebin the data into common wavelength bins
    MatrixWorkspace_sptr outputWS = this->rebin(tempWS, minLambda, maxLambda, max_bins - 1);

    g_log.debug() << "Rebinned workspace has " << outputWS->getNumberHistograms() << " histograms of "
                  << outputWS->blocksize() << " bins each\n";
    setProperty("OutputWorkspace", outputWS);
  } else
    setProperty("OutputWorkspace", tempWS);

  m_inputWS.reset();
}

/** Unwraps an X array, converting the units to wavelength along the way.
 *  @param newX ::   A reference to a container which stores our unwrapped x
 * data.
 *  @param spectrum :: The workspace index
 *  @param Ld ::       The flightpath for the detector related to this spectrum
 *  @return A 3-element vector containing the bins at which the upper and lower
 * ranges start & end
 */
const std::vector<int> UnwrapMonitor::unwrapX(std::vector<double> &newX, const int &spectrum, const double &Ld) {
  // Create and initalise the vector that will store the bin ranges, and will be
  // returned
  // Elements are: 0 - Lower range start, 1 - Lower range end, 2 - Upper range
  // start
  std::vector<int> binRange(3, -1);

  // Calculate cut-off times
  const double T1 = m_Tmax - (m_Tmin * (1 - (Ld / m_LRef)));
  const double T2 = m_Tmax * (Ld / m_LRef);

  // Create a temporary vector to store the lower range of the unwrapped
  // histograms
  std::vector<double> tempX_L;
  tempX_L.reserve(m_XSize); // Doing this possible gives a small efficiency increase
  // Create a vector for the upper range. Make it a reference to the output
  // histogram to save an assignment later
  newX.clear();
  newX.reserve(m_XSize);

  // Get a reference to the input x data
  const auto &xdata = m_inputWS->x(spectrum);
  // Loop over histogram, selecting bins in appropriate ranges.
  // At the moment, the data in the bin in which a cut-off sits is excluded.
  for (unsigned int bin = 0; bin < m_XSize; ++bin) {
    // This is the time-of-flight value under consideration in the current
    // iteration of the loop
    const double tof = xdata[bin];
    // First deal with bins where m_Tmin < tof < T2
    if (tof < T2) {
      const double wavelength = (m_conversionConstant * tof) / Ld;
      tempX_L.emplace_back(wavelength);
      // Record the bins that fall in this range for copying over the data &
      // errors
      if (binRange[0] == -1)
        binRange[0] = bin;
      binRange[1] = bin;
    }
    // Now do the bins where T1 < tof < m_Tmax
    else if (tof > T1) {
      const double velocity = Ld / (tof - m_Tmax + m_Tmin);
      const double wavelength = m_conversionConstant / velocity;
      newX.emplace_back(wavelength);
      // Remove the duplicate boundary bin
      if (tof == m_Tmax && Kernel::withinAbsoluteDifference(wavelength, tempX_L.front(), 1.0e-5))
        newX.pop_back();
      // Record the bins that fall in this range for copying over the data &
      // errors
      if (binRange[2] == -1)
        binRange[2] = bin;
    }
  } // loop over X values

  // Deal with the (rare) case that a detector (e.g. downstream monitor) is at a
  // longer flightpath than m_LRef
  if (Ld > m_LRef) {
    std::pair<int, int> binLimits = this->handleFrameOverlapped(xdata, Ld, tempX_L);
    binRange[0] = binLimits.first;
    binRange[1] = binLimits.second;
  }

  // Record the point at which the unwrapped sections are joined, first time
  // through only
  Property const *join = getProperty("JoinWavelength");
  if (join->isDefault()) {
    g_log.information() << "Joining wavelength: " << tempX_L.front() << " Angstrom\n";
    setProperty("JoinWavelength", tempX_L.front());
  }

  // Append first vector to back of second
  newX.insert(newX.end(), tempX_L.begin(), tempX_L.end());

  return binRange;
}

/** Deals with the (rare) case where the flightpath is longer than the reference
 *  Note that in this case both T1 & T2 will be greater than Tmax
 */
std::pair<int, int> UnwrapMonitor::handleFrameOverlapped(const Mantid::HistogramData::HistogramX &xdata,
                                                         const double &Ld, std::vector<double> &tempX) {
  // Calculate the interval to exclude
  const double Dt = (m_Tmax - m_Tmin) * (1 - (m_LRef / Ld));
  // This gives us new minimum & maximum tof values
  const double minT = m_Tmin + Dt;
  const double maxT = m_Tmax - Dt;
  // Can have situation where Ld is so much larger than Lref that everything
  // would be excluded.
  // This is an invalid input - warn and leave spectrum unwrapped
  if (minT > maxT) {
    g_log.warning() << "Invalid input, Ld (" << Ld << ") >> Lref (" << m_LRef
                    << "). Current spectrum will not be unwrapped.\n";
    return std::make_pair(0, static_cast<int>(xdata.size() - 1));
  }

  int min = 0, max = static_cast<int>(xdata.size());
  for (unsigned int j = 0; j < m_XSize; ++j) {
    const double T = xdata[j];
    if (T < minT) {
      min = j + 1;
      tempX.erase(tempX.begin());
    } else if (T > maxT) {
      tempX.erase(tempX.end() - (max - j), tempX.end());
      max = j - 1;
      break;
    }
  }
  return std::make_pair(min, max);
}

/** Unwraps the Y & E vectors of a spectrum according to the ranges found in
 * unwrapX.
 *  @param tempWS ::      A pointer to the temporary workspace in which the
 * results are being stored
 *  @param spectrum ::    The workspace index
 *  @param rangeBounds :: The upper and lower ranges for the unwrapping
 *  @param newY :: A reference to a container which stores our unwrapped y data.
 *  @param newE :: A reference to a container which stores our unwrapped e data.
 */
void UnwrapMonitor::unwrapYandE(const API::MatrixWorkspace_sptr &tempWS, const int &spectrum,
                                const std::vector<int> &rangeBounds, std::vector<double> &newY,
                                std::vector<double> &newE) {
  // Copy over the relevant ranges of Y & E data
  std::vector<double> &Y = newY;
  std::vector<double> &E = newE;
  // Get references to the input data
  const auto &YIn = m_inputWS->y(spectrum);
  const auto &EIn = m_inputWS->e(spectrum);
  if (rangeBounds[2] != -1) {
    // Copy in the upper range
    Y.assign(YIn.begin() + rangeBounds[2], YIn.end());
    E.assign(EIn.begin() + rangeBounds[2], EIn.end());
    // Propagate masking, if necessary
    if (m_inputWS->hasMaskedBins(spectrum)) {
      const MatrixWorkspace::MaskList &inputMasks = m_inputWS->maskedBins(spectrum);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it) {
        if (static_cast<int>((*it).first) >= rangeBounds[2])
          tempWS->flagMasked(spectrum, (*it).first - rangeBounds[2], (*it).second);
      }
    }
  } else {
    // Y & E are references to existing vector. Assign above clears them, so
    // need to explicitly here
    Y.clear();
    E.clear();
  }
  if (rangeBounds[0] != -1 && rangeBounds[1] > 0) {
    // Now append the lower range
    auto YStart = YIn.begin();
    auto EStart = EIn.begin();
    Y.insert(Y.end(), YStart + rangeBounds[0], YStart + rangeBounds[1]);
    E.insert(E.end(), EStart + rangeBounds[0], EStart + rangeBounds[1]);
    // Propagate masking, if necessary
    if (m_inputWS->hasMaskedBins(spectrum)) {
      const MatrixWorkspace::MaskList &inputMasks = m_inputWS->maskedBins(spectrum);
      for (const auto &inputMask : inputMasks) {
        const auto maskIndex = static_cast<int>(inputMask.first);
        if (maskIndex >= rangeBounds[0] && maskIndex < rangeBounds[1])
          tempWS->flagMasked(spectrum, maskIndex - rangeBounds[0], inputMask.second);
      }
    }
  }
}

/** Rebins the data into common bins of wavelength.
 *  @param workspace :: The input workspace to the rebinning
 *  @param min ::       The lower limit in X for the rebinning
 *  @param max ::       The upper limit in X for the rebinning
 *  @param numBins ::   The number of bins into which to rebin
 *  @return A pointer to the workspace containing the rebinned data
 *  @throw std::runtime_error If the Rebin child algorithm fails
 */
API::MatrixWorkspace_sptr UnwrapMonitor::rebin(const API::MatrixWorkspace_sptr &workspace, const double &min,
                                               const double &max, const size_t &numBins) {
  // Calculate the width of a bin
  const double step = (max - min) / static_cast<double>(numBins);

  // Create a Rebin child algorithm
  auto childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  childAlg->setPropertyValue("OutputWorkspace", "Anonymous");

  // Construct the vector that holds the rebin parameters and set the property
  std::vector<double> paramArray = {min, step, max};
  childAlg->setProperty<std::vector<double>>("Params", paramArray);
  g_log.debug() << "Rebinning unwrapped data into " << numBins << " bins of width " << step
                << " Angstroms, running from " << min << " to " << max << '\n';

  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Mantid::Algorithms
