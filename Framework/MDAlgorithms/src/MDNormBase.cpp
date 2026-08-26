// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/MDNormBase.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::MDAlgorithms {

namespace {
// function to  compare two intersections (h,k,l,Momentum) by Momentum
bool compareMomentum(const std::array<double, 4> &v1, const std::array<double, 4> &v2) { return (v1[3] < v2[3]); }
const std::string LOG_CHARGE_NAME("proton_charge");
} // namespace

MDNormBase::MDNormBase()
    : m_hmin(0.0f), m_hmax(0.0f), m_kmin(0.0f), m_kmax(0.0f), m_lmin(0.0f), m_lmax(0.0f), m_dEmin(0.f), m_dEmax(0.f),
      m_Ei(0.), m_ki(0.), m_kfmin(0.), m_kfmax(0.), m_hIntegrated(true), m_kIntegrated(true), m_lIntegrated(true),
      m_dEIntegrated(true), m_rubw(3, 3), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_eIdx(-1), m_hX(), m_kX(), m_lX(),
      m_eX(), m_samplePos(), m_beamDir() {}

/**
 * Checks the normalization workspace against the indices of the original
 * dimensions.
 * If not found, the corresponding dimension is integrated
 * @param otherDimValues Values from non-HKL dimensions
 * @param skipNormalization [InOut] Sets the flag true if normalization values
 * are outside of original inputs
 * @return Affine trasform matrix
 */
Kernel::Matrix<coord_t> MDNormBase::findIntergratedDimensions(const std::vector<coord_t> &otherDimValues,
                                                              bool &skipNormalization) {
  // Get indices of the original dimensions in the output workspace,
  // and if not found, the corresponding dimension is integrated
  Kernel::Matrix<coord_t> affineMat = m_normWS->getTransformFromOriginal(0)->makeAffineMatrix();

  const size_t nrm1 = affineMat.numRows() - 1;
  const size_t ncm1 = affineMat.numCols() - 1;
  const size_t lastCol = m_diffraction ? 3 : 4;
  for (size_t row = 0; row < nrm1; row++) // affine matrix, ignore last row
  {
    const auto dimen = m_normWS->getDimension(row);
    const auto dimMin(dimen->getMinimum()), dimMax(dimen->getMaximum());
    if (affineMat[row][0] == 1.) {
      m_hIntegrated = false;
      m_hIdx = row;
      m_hmin = std::max(m_hmin, dimMin);
      m_hmax = std::min(m_hmax, dimMax);
      if (m_hmin > dimMax || m_hmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][1] == 1.) {
      m_kIntegrated = false;
      m_kIdx = row;
      m_kmin = std::max(m_kmin, dimMin);
      m_kmax = std::min(m_kmax, dimMax);
      if (m_kmin > dimMax || m_kmax < dimMin) {
        skipNormalization = true;
      }
    }
    if (affineMat[row][2] == 1.) {
      m_lIntegrated = false;
      m_lIdx = row;
      m_lmin = std::max(m_lmin, dimMin);
      m_lmax = std::min(m_lmax, dimMax);
      if (m_lmin > dimMax || m_lmax < dimMin) {
        skipNormalization = true;
      }
    }

    if (!m_diffraction && affineMat[row][3] == 1.) {
      m_dEIntegrated = false;
      m_eIdx = row;
      m_dEmin = std::max(m_dEmin, dimMin);
      m_dEmax = std::min(m_dEmax, dimMax);
      if (m_dEmin > dimMax || m_dEmax < dimMin) {
        skipNormalization = true;
      }
    }
    for (size_t col = lastCol; col < ncm1; col++) // affine matrix, ignore last column
    {
      if (affineMat[row][col] == 1.) {
        double val = otherDimValues.at(col - 3);
        if (val > dimMax || val < dimMin) {
          skipNormalization = true;
        }
      }
    }
  }

  return affineMat;
}

/**
 * Stores the X values from each H,K,L,E dimension as member variables
 * Energy dimension is transformed to final wavevector.
 */
void MDNormBase::cacheDimensionXValues() {
  constexpr double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass * PhysicalConstants::meV * 1e-20 /
                               (PhysicalConstants::h * PhysicalConstants::h);
  if (!m_hIntegrated) {
    auto &hDim = *m_normWS->getDimension(m_hIdx);
    m_hX.resize(hDim.getNBoundaries());
    for (size_t i = 0; i < m_hX.size(); ++i) {
      m_hX[i] = hDim.getX(i);
    }
  }
  if (!m_kIntegrated) {
    auto &kDim = *m_normWS->getDimension(m_kIdx);
    m_kX.resize(kDim.getNBoundaries());
    for (size_t i = 0; i < m_kX.size(); ++i) {
      m_kX[i] = kDim.getX(i);
    }
  }
  if (!m_lIntegrated) {
    auto &lDim = *m_normWS->getDimension(m_lIdx);
    m_lX.resize(lDim.getNBoundaries());
    for (size_t i = 0; i < m_lX.size(); ++i) {
      m_lX[i] = lDim.getX(i);
    }
  }
  if (!m_dEIntegrated) {
    // NOTE: store k final instead
    auto &eDim = *m_normWS->getDimension(m_eIdx);
    m_eX.resize(eDim.getNBoundaries());
    for (size_t i = 0; i < m_eX.size(); ++i) {
      double temp = m_Ei - eDim.getX(i);
      temp = std::max(temp, 0.);
      m_eX[i] = std::sqrt(energyToK * temp);
    }
  }
}

/**
 * Computed the normalization for the input workspace. Results are stored in
 * m_normWS
 * @param otherValues non HKLE dimensions
 * @param affineTrans affine matrix
 * @param expInfoIndex current experiment info index
 */
void MDNormBase::calculateNormalization(const std::vector<coord_t> &otherValues,
                                        const Kernel::Matrix<coord_t> &affineTrans, uint16_t expInfoIndex) {
  using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
  const auto &currentExptInfo = *(m_inputWS->getExperimentInfo(expInfoIndex));
  const auto &spectrumInfo = currentExptInfo.spectrumInfo();
  auto *rubwLog = dynamic_cast<VectorDoubleProperty *>(currentExptInfo.getLog("RUBW_MATRIX"));
  if (!rubwLog) {
    throw std::runtime_error("Wokspace does not contain a log entry for the RUBW matrix."
                             "Cannot continue.");
  } else {
    Kernel::DblMatrix rubwValue((*rubwLog)()); // includes the 2*pi factor but not goniometer for now :)
    m_rubw = currentExptInfo.run().getGoniometerMatrix() * rubwValue;
    m_rubw.Invert();
  }
  const double protonCharge = currentExptInfo.run().getProtonCharge();

  calculateNormInner(spectrumInfo, protonCharge, otherValues, affineTrans);
}

/**
 * Computes the normalization for the input workspace for the case of a continous rotation
 * @param otherValues non HKLE dimensions
 * @param affineTrans affine matrix
 * @param expInfoIndex current experiment info index
 */
void MDNormBase::calculateNormContinuous(const std::vector<coord_t> &otherValues,
                                         const Kernel::Matrix<coord_t> &affineTrans, uint16_t expInfoIndex) {
  using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
  const auto &currentExptInfo = *(m_inputWS->getExperimentInfo(expInfoIndex));
  const auto &spectrumInfo = currentExptInfo.spectrumInfo();
  auto *rubwLog = dynamic_cast<VectorDoubleProperty *>(currentExptInfo.getLog("RUBW_MATRIX"));
  Kernel::DblMatrix rubwValue((*rubwLog)()); // includes the 2*pi factor but not goniometer for now :)

  // MDEventWS was created with the "useLogTimes" option: should be only a single expInfo, but
  // gonios vary with time - we now coarse-bin it to compute the normalisation.
  const Run &run = currentExptInfo.run();
  if (!run.hasProperty(LOG_CHARGE_NAME)) {
    throw std::runtime_error("Wokspace does not contain the proton charge log. Cannot continue.");
  }

  double progressStart = 0.3 + 0.7 * expInfoIndex / m_numExptInfos;
  double progressEnd = 0.3 + 0.7 * (expInfoIndex + 1) / m_numExptInfos;
  double normfac = run.hasProperty("NormalizationFactor")
                       ? (*dynamic_cast<Kernel::PropertyWithValue<double> *>(run.getProperty("NormalizationFactor")))()
                       : 1.0;
  std::istringstream tosplit;
  tosplit.str((*dynamic_cast<PropertyWithValue<std::string> *>(run.getProperty("useLogTimes")))());
  std::vector<TimeSeriesProperty<double> *> logs;
  std::vector<size_t> movingGonioIndex;
  const TimeSeriesProperty<double> *protonlog = run.getTimeSeriesProperty<double>(LOG_CHARGE_NAME);
  std::vector<double> protonCharge = protonlog->valuesAsVector();
  std::vector<Types::Core::DateAndTime> protonTimes = protonlog->timesAsVector();
  Geometry::Goniometer gonio(run.getGoniometer());

  for (std::string name; std::getline(tosplit, name, ',');) {
    auto *log = run.getTimeSeriesProperty<double>(name);
    logs.push_back(log);
    if ((log->maxValue() - log->minValue()) > STATIONARYANGLIM) { // Assume gonio logs in degrees
      movingGonioIndex.push_back(logs.size() - 1);
    }
  }

  // Convert from picoCoulomb to uA.hr for SNS data
  if (protonlog->units().find("picoCoulomb") != std::string::npos) {
    normfac *= 3600.e6;
  }

  if (movingGonioIndex.size() == 1) {
    // If we only have a single moving gonio, bin all its values to GONIOBINSTEP degree bins and
    // run inner loop on each binned angle
    const TimeROI &timeroi = run.getTimeROI();
    const auto &gonioAxLog = logs[movingGonioIndex[0]];
    std::vector<double> filteredVals = gonioAxLog->filteredValuesAsVector(&timeroi);
    const auto &[min, max] = std::minmax_element(filteredVals.begin(), filteredVals.end());
    std::vector<double> gonioCharge(static_cast<int>((*max - *min) / GONIOBINSTEP) + 1, 0.0);
    for (size_t n = 0; n < protonCharge.size(); n++) {
      double logval = gonioAxLog->getSingleValue(protonTimes[n]);
      if (std::isnan(logval) || logval > *max || logval < *min) {
        continue;
      }
      auto idx = static_cast<size_t>(floor((logval - *min) / GONIOBINSTEP));
      gonioCharge[idx] += protonCharge[n];
    }
    m_progress->resetNumSteps(static_cast<int64_t>(gonioCharge.size()), progressStart, progressEnd);
    for (size_t n = 0; n < gonioCharge.size(); n++) {
      if (gonioCharge[n] < MINPROTONCHARGE) {
        continue;
      }
      auto nn = static_cast<double>(n);
      gonio.setRotationAngle(movingGonioIndex[0], nn * GONIOBINSTEP + *min);
      m_rubw = gonio.getR() * rubwValue;
      m_rubw.Invert();
      calculateNormInner(spectrumInfo, gonioCharge[n] / normfac, otherValues, affineTrans);
      m_progress->report();
    }
  } else {
    // Otherwise run inner loop over small bins of proton charge in time
    double chargeSum = 0.0;
    size_t i0 = 0;
    bool skipIter = false;
    m_progress->resetNumSteps(static_cast<int64_t>(protonCharge.size()), progressStart, progressEnd);
    for (size_t n = 0; n < protonCharge.size(); n++) {
      chargeSum += protonCharge[n];
      if (chargeSum > CHARGEBINSIZE) {
        size_t mid = static_cast<int>(floor(static_cast<double>(n - i0) / 2.));
        skipIter = false;
        for (size_t gAx = 0; gAx < gonio.getNumberAxes(); gAx++) {
          double logval = logs[gAx]->getSingleValue(protonTimes[mid]);
          if (std::isnan(logval)) {
            skipIter = true;
            continue;
          }
          gonio.setRotationAngle(gAx, logval);
        }
        if (!skipIter) {
          m_rubw = gonio.getR() * rubwValue;
          m_rubw.Invert();
          calculateNormInner(spectrumInfo, chargeSum / normfac, otherValues, affineTrans);
        }
        chargeSum = 0;
        i0 = n;
      }
      m_progress->report();
    }
  }
  if (m_numExptInfos > 1) {
    m_progress->resetNumSteps(m_numExptInfos - expInfoIndex, progressStart, 1.0);
  }
}

void MDNormBase::calculateNormInner(const API::SpectrumInfo &spectrumInfo, const double protonCharge,
                                    const std::vector<coord_t> &otherValues,
                                    const Kernel::Matrix<coord_t> &affineTrans) {
  constexpr double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass * PhysicalConstants::meV * 1e-20 /
                               (PhysicalConstants::h * PhysicalConstants::h);
  // Mapping
  const auto ndets = static_cast<int64_t>(spectrumInfo.size());
  bool haveSA = false;
  API::MatrixWorkspace_const_sptr integrFlux, solidAngleWS = getProperty("SolidAngleWorkspace");
  detid2index_map fluxDetToIdx, solidAngDetToIdx;
  if (m_diffraction) {
    integrFlux = getProperty("FluxWorkspace"); // FluxWorkspace is mandatory for diffraction
    integrFlux->getXMinMax(m_kfmin, m_kfmax);
    fluxDetToIdx = integrFlux->getDetectorIDToWorkspaceIndexMap();
  }
  if (solidAngleWS != nullptr) {
    haveSA = true;
    solidAngDetToIdx = solidAngleWS->getDetectorIDToWorkspaceIndexMap();
  }

  const size_t vmdDims = 4;

  PRAGMA_OMP(parallel for)
  for (int64_t i = 0; i < ndets; i++) {
    PARALLEL_START_INTERRUPT_REGION

    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i)) {
      continue;
    }
    const auto &detector = spectrumInfo.detector(i);
    double theta = detector.getTwoTheta(m_samplePos, m_beamDir);
    double phi = detector.getPhi();
    // If the detector is a group, this should be the ID of the first detector
    const auto detID = detector.getID();

    // Intersections
    std::vector<std::array<double, 4>> intersections;
    std::vector<coord_t> pos, posNew;
    this->calculateIntersections(intersections, theta, phi);
    if (intersections.empty())
      continue;

    // Get solid angle for this contribution
    double solid = protonCharge;
    if (haveSA) {
      solid = solidAngleWS->y(solidAngDetToIdx.find(detID)->second)[0] * protonCharge;
    }

    // -- calculate integrals for the intersection --
    // momentum values at intersections
    std::vector<double> yValues;
    if (m_diffraction) {
      // get the flux spetrum number
      size_t wsIdx = fluxDetToIdx.find(detID)->second;
      // copy momenta to xValues
      std::vector<double> xValues(intersections.size());
      yValues.resize(intersections.size());
      auto x = xValues.begin();
      for (auto it = intersections.begin(); it != intersections.end(); ++it, ++x) {
        *x = (*it)[3];
      }
      // calculate integrals at momenta from xValues by interpolating between
      // points in spectrum sp
      // of workspace integrFlux. The result is stored in yValues
      calcIntegralsForIntersections(xValues, *integrFlux, wsIdx, yValues);
    }

    // Compute final position in HKL
    // pre-allocate for efficiency and copy non-hkl dim values into place
    pos.resize(vmdDims + otherValues.size() + 1);
    std::copy(otherValues.begin(), otherValues.end(), pos.begin() + vmdDims);
    pos.emplace_back(1.f);
    for (auto it = intersections.begin() + 1; it != intersections.end(); ++it) {
      const auto &curIntSec = *it;
      const auto &prevIntSec = *(it - 1);
      // the full vector isn't used so compute only what is necessary
      double delta = (curIntSec[3] * curIntSec[3] - prevIntSec[3] * prevIntSec[3]) / energyToK;
      if (delta < 1e-10)
        continue; // Assume zero contribution if difference is small

      // Average between two intersections for final position
      std::transform(curIntSec.data(), curIntSec.data() + vmdDims, prevIntSec.data(), pos.begin(),
                     [](const double rhs, const double lhs) { return static_cast<coord_t>(0.5 * (rhs + lhs)); });

      // transform kf to energy transfer
      if (!m_diffraction) {
        pos[3] = static_cast<coord_t>(m_Ei - pos[3] * pos[3] / energyToK);
      }
      affineTrans.multiplyPoint(pos, posNew);
      size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
      if (linIndex == static_cast<size_t>(-1))
        continue;

      if (m_diffraction) {
        // index of the current intersection
        auto k = static_cast<size_t>(std::distance(intersections.begin(), it));
        delta = (yValues[k] - yValues[k - 1]);
      }
      // signal = delta * solid = integral between two consecutive intersections * solid angle
      Mantid::Kernel::AtomicOp(m_signalArray[linIndex], delta * solid, std::plus<signal_t>());
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * Linearly interpolate between the points in integrFlux at xValues and save the
 * results in yValues.
 * @param xValues :: X-values at which to interpolate
 * @param integrFlux :: A workspace with the spectra to interpolate
 * @param sp :: A workspace index for a spectrum in integrFlux to interpolate.
 * @param yValues :: A vector to save the results.
 */
void MDNormBase::calcIntegralsForIntersections(const std::vector<double> &xValues,
                                               const API::MatrixWorkspace &integrFlux, size_t sp,
                                               std::vector<double> &yValues) const {
  assert(xValues.size() == yValues.size());

  // the x-data from the workspace
  const auto &xData = integrFlux.x(sp);
  const double xStart = xData.front();
  const double xEnd = xData.back();

  // the values in integrFlux are expected to be integrals of a non-negative
  // function
  // ie they must make a non-decreasing function
  const auto &yData = integrFlux.y(sp);
  size_t spSize = yData.size();

  const double yMin = 0.0;
  const double yMax = yData.back();

  size_t nData = xValues.size();
  // all integrals below xStart must be 0
  if (xValues[nData - 1] < xStart) {
    std::fill(yValues.begin(), yValues.end(), yMin);
    return;
  }

  // all integrals above xEnd must be equal tp yMax
  if (xValues[0] > xEnd) {
    std::fill(yValues.begin(), yValues.end(), yMax);
    return;
  }

  size_t i = 0;
  // integrals below xStart must be 0
  while (i < nData - 1 && xValues[i] < xStart) {
    yValues[i] = yMin;
    i++;
  }
  size_t j = 0;
  for (; i < nData; i++) {
    // integrals above xEnd must be equal tp yMax
    if (j >= spSize - 1) {
      yValues[i] = yMax;
    } else {
      double xi = xValues[i];
      while (j < spSize - 1 && xi > xData[j])
        j++;
      // if x falls onto an interpolation point return the corresponding y
      if (xi == xData[j]) {
        yValues[i] = yData[j];
      } else if (j == spSize - 1) {
        // if we get above xEnd it's yMax
        yValues[i] = yMax;
      } else if (j > 0) {
        // interpolate between the consecutive points
        double x0 = xData[j - 1];
        double x1 = xData[j];
        double y0 = yData[j - 1];
        double y1 = yData[j];
        yValues[i] = y0 + (y1 - y0) * (xi - x0) / (x1 - x0);
      } else // j == 0
      {
        yValues[i] = yMin;
      }
    }
  }
}

/**
 * Calculate the points of intersection for the given detector with cuboid
 * surrounding the
 * detector position in HKL
 * @param intersections A list of intersections in HKL space
 * @param theta Polar angle with detector
 * @param phi Azimuthal angle with detector
 */
void MDNormBase::calculateIntersections(std::vector<std::array<double, 4>> &intersections, const double theta,
                                        const double phi) {
  V3D qin, qout;
  if (m_diffraction) {
    qout = V3D(-sin(theta) * cos(phi), -sin(theta) * sin(phi), 1. - cos(theta));
    qin = qout;
  } else {
    qout = V3D(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    qin = V3D(0., 0., m_ki);
  }

  qout = m_rubw * qout;
  qin = m_rubw * qin;
  if (m_convention == "Crystallography") {
    qout *= -1;
    qin *= -1;
  }
  double hStart = qin.X() - qout.X() * m_kfmin, hEnd = qin.X() - qout.X() * m_kfmax;
  double kStart = qin.Y() - qout.Y() * m_kfmin, kEnd = qin.Y() - qout.Y() * m_kfmax;
  double lStart = qin.Z() - qout.Z() * m_kfmin, lEnd = qin.Z() - qout.Z() * m_kfmax;
  double eps = 1e-10;
  auto hNBins = m_hX.size();
  auto kNBins = m_kX.size();
  auto lNBins = m_lX.size();
  auto eNBins = m_eX.size();
  intersections.clear();
  intersections.reserve(hNBins + kNBins + lNBins + eNBins + 8); // 8 is 3*(min,max for each Q component)+kfmin+kfmax

  // calculate intersections with planes perpendicular to h
  if (fabs(hStart - hEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (hEnd - hStart);
    double fk = (kEnd - kStart) / (hEnd - hStart);
    double fl = (lEnd - lStart) / (hEnd - hStart);
    if (!m_hIntegrated) {
      for (size_t i = 0; i < hNBins; i++) {
        double hi = m_hX[i];
        if ((hi >= m_hmin) && (hi <= m_hmax) && ((hStart - hi) * (hEnd - hi) < 0)) {
          // if hi is between hStart and hEnd, then ki and li will be between
          // kStart, kEnd and lStart, lEnd and momi will be between m_kfmin and
          // m_kfmax
          double ki = fk * (hi - hStart) + kStart;
          double li = fl * (hi - hStart) + lStart;
          if ((ki >= m_kmin) && (ki <= m_kmax) && (li >= m_lmin) && (li <= m_lmax)) {
            double momi = fmom * (hi - hStart) + m_kfmin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }
    double momhMin = fmom * (m_hmin - hStart) + m_kfmin;
    if ((momhMin - m_kfmin) * (momhMin - m_kfmax) < 0) // m_kfmin>m_kfmax
    {
      // khmin and lhmin
      double khmin = fk * (m_hmin - hStart) + kStart;
      double lhmin = fl * (m_hmin - hStart) + lStart;
      if ((khmin >= m_kmin) && (khmin <= m_kmax) && (lhmin >= m_lmin) && (lhmin <= m_lmax)) {
        intersections.push_back({{m_hmin, khmin, lhmin, momhMin}});
      }
    }
    double momhMax = fmom * (m_hmax - hStart) + m_kfmin;
    if ((momhMax - m_kfmin) * (momhMax - m_kfmax) <= 0) {
      // khmax and lhmax
      double khmax = fk * (m_hmax - hStart) + kStart;
      double lhmax = fl * (m_hmax - hStart) + lStart;
      if ((khmax >= m_kmin) && (khmax <= m_kmax) && (lhmax >= m_lmin) && (lhmax <= m_lmax)) {
        intersections.push_back({{m_hmax, khmax, lhmax, momhMax}});
      }
    }
  }

  // calculate intersections with planes perpendicular to k
  if (fabs(kStart - kEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (kEnd - kStart);
    double fh = (hEnd - hStart) / (kEnd - kStart);
    double fl = (lEnd - lStart) / (kEnd - kStart);
    if (!m_kIntegrated) {
      for (size_t i = 0; i < kNBins; i++) {
        double ki = m_kX[i];
        if ((ki >= m_kmin) && (ki <= m_kmax) && ((kStart - ki) * (kEnd - ki) < 0)) {
          // if ki is between kStart and kEnd, then hi and li will be between
          // hStart, hEnd and lStart, lEnd and momi will be between m_kfmin and
          // m_kfmax
          double hi = fh * (ki - kStart) + hStart;
          double li = fl * (ki - kStart) + lStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (li >= m_lmin) && (li <= m_lmax)) {
            double momi = fmom * (ki - kStart) + m_kfmin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }
    double momkMin = fmom * (m_kmin - kStart) + m_kfmin;
    if ((momkMin - m_kfmin) * (momkMin - m_kfmax) < 0) {
      // hkmin and lkmin
      double hkmin = fh * (m_kmin - kStart) + hStart;
      double lkmin = fl * (m_kmin - kStart) + lStart;
      if ((hkmin >= m_hmin) && (hkmin <= m_hmax) && (lkmin >= m_lmin) && (lkmin <= m_lmax)) {
        intersections.push_back({{hkmin, m_kmin, lkmin, momkMin}});
      }
    }
    double momkMax = fmom * (m_kmax - kStart) + m_kfmin;
    if ((momkMax - m_kfmin) * (momkMax - m_kfmax) <= 0) {
      // hkmax and lkmax
      double hkmax = fh * (m_kmax - kStart) + hStart;
      double lkmax = fl * (m_kmax - kStart) + lStart;
      if ((hkmax >= m_hmin) && (hkmax <= m_hmax) && (lkmax >= m_lmin) && (lkmax <= m_lmax)) {
        intersections.push_back({{hkmax, m_kmax, lkmax, momkMax}});
      }
    }
  }

  // calculate intersections with planes perpendicular to l
  if (fabs(lStart - lEnd) > eps) {
    double fmom = (m_kfmax - m_kfmin) / (lEnd - lStart);
    double fh = (hEnd - hStart) / (lEnd - lStart);
    double fk = (kEnd - kStart) / (lEnd - lStart);
    if (!m_lIntegrated) {
      for (size_t i = 0; i < lNBins; i++) {
        double li = m_lX[i];
        if ((li >= m_lmin) && (li <= m_lmax) && ((lStart - li) * (lEnd - li) < 0)) {
          // if li is between lStart and lEnd, then hi and ki will be between
          // hStart, hEnd and kStart, kEnd
          double hi = fh * (li - lStart) + hStart;
          double ki = fk * (li - lStart) + kStart;
          if ((hi >= m_hmin) && (hi <= m_hmax) && (ki >= m_kmin) && (ki <= m_kmax)) {
            double momi = fmom * (li - lStart) + m_kfmin;
            intersections.push_back({{hi, ki, li, momi}});
          }
        }
      }
    }
    double momlMin = fmom * (m_lmin - lStart) + m_kfmin;
    if ((momlMin - m_kfmin) * (momlMin - m_kfmax) <= 0) {
      // hlmin and klmin
      double hlmin = fh * (m_lmin - lStart) + hStart;
      double klmin = fk * (m_lmin - lStart) + kStart;
      if ((hlmin >= m_hmin) && (hlmin <= m_hmax) && (klmin >= m_kmin) && (klmin <= m_kmax)) {
        intersections.push_back({{hlmin, klmin, m_lmin, momlMin}});
      }
    }
    double momlMax = fmom * (m_lmax - lStart) + m_kfmin;
    if ((momlMax - m_kfmin) * (momlMax - m_kfmax) < 0) {
      // hlmax and klmax
      double hlmax = fh * (m_lmax - lStart) + hStart;
      double klmax = fk * (m_lmax - lStart) + kStart;
      if ((hlmax >= m_hmin) && (hlmax <= m_hmax) && (klmax >= m_kmin) && (klmax <= m_kmax)) {
        intersections.push_back({{hlmax, klmax, m_lmax, momlMax}});
      }
    }
  }

  // intersections with dE
  if (!m_dEIntegrated) {
    for (size_t i = 0; i < eNBins; i++) {
      double kfi = m_eX[i];
      if ((kfi - m_kfmin) * (kfi - m_kfmax) <= 0) {
        double h = qin.X() - qout.X() * kfi;
        double k = qin.Y() - qout.Y() * kfi;
        double l = qin.Z() - qout.Z() * kfi;
        if ((h >= m_hmin) && (h <= m_hmax) && (k >= m_kmin) && (k <= m_kmax) && (l >= m_lmin) && (l <= m_lmax)) {
          intersections.push_back({{h, k, l, kfi}});
        }
      }
    }
  }

  // endpoints
  if ((hStart >= m_hmin) && (hStart <= m_hmax) && (kStart >= m_kmin) && (kStart <= m_kmax) && (lStart >= m_lmin) &&
      (lStart <= m_lmax)) {
    intersections.push_back({{hStart, kStart, lStart, m_kfmin}});
  }
  if ((hEnd >= m_hmin) && (hEnd <= m_hmax) && (kEnd >= m_kmin) && (kEnd <= m_kmax) && (lEnd >= m_lmin) &&
      (lEnd <= m_lmax)) {
    intersections.push_back({{hEnd, kEnd, lEnd, m_kfmax}});
  }

  // sort intersections by final momentum
  std::stable_sort(intersections.begin(), intersections.end(), compareMomentum);
}

} // namespace Mantid::MDAlgorithms
