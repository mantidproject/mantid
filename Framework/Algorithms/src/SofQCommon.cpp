// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SofQCommon.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"

namespace Mantid::Algorithms {
/** The procedure analyses emode and efixed properties provided to the algorithm
 *and identify the energy analysis mode and the way the properties are defined
 *@param workspace     :: input workspace which may or may not have incident
 *energy property (Ei) attached to it as the run log
 *@param hostAlgorithm :: the pointer to SofQ algorithm hosting the base class.
 *This algorithm expects to have EMode and EFixed properties attached to it.
 */
void SofQCommon::initCachedValues(const API::MatrixWorkspace &workspace, API::Algorithm *const hostAlgorithm) {
  // Retrieve the emode & efixed properties
  const std::string emode = hostAlgorithm->getProperty("EMode");
  // Convert back to an integer representation
  m_emode = 0;
  if (emode == "Direct")
    m_emode = 1;
  else if (emode == "Indirect")
    m_emode = 2;
  m_efixed = hostAlgorithm->getProperty("EFixed");

  // Check whether they should have supplied an EFixed value
  if (m_emode == 1) // Direct
  {
    // If GetEi was run then it will have been stored in the workspace, if not
    // the user will need to enter one
    if (m_efixed == 0.0) {
      if (workspace.run().hasProperty("Ei")) {
        Kernel::Property *p = workspace.run().getProperty("Ei");
        auto *eiProp = dynamic_cast<Kernel::PropertyWithValue<double> *>(p);
        if (!eiProp)
          throw std::runtime_error("Input workspace contains Ei but its "
                                   "property type is not a double.");
        m_efixed = (*eiProp)();
      } else {
        throw std::invalid_argument("Input workspace does not contain an "
                                    "EFixed value. Please provide one or run "
                                    "GetEi.");
      }
    } else {
      m_efixedGiven = true;
    }
  } else {
    if (m_efixed != 0.0) {
      m_efixedGiven = true;
    }
  }
}

/**
 * Return the efixed for this detector. In Direct mode this has to be property
 set up earlier and in Indirect mode it may be the property of a component
                                        if not specified globally for the
 instrument.
 * @param det A pointer to a detector object
 * @return The value of efixed
 */
double SofQCommon::getEFixed(const Geometry::IDetector &det) const {
  double efixed(0.0);
  if (m_emode == 1) // Direct
  {
    efixed = m_efixed;
  } else // Indirect
  {
    if (m_efixedGiven)
      efixed = m_efixed; // user provided a value
    else {
      const std::vector<double> param = det.getNumberParameter("EFixed");
      if (param.empty())
        throw std::runtime_error("Cannot find EFixed parameter for component \"" + det.getName() +
                                 "\". This is required in indirect mode. Please check the IDF "
                                 "contains these values.");
      efixed = param[0];
    }
  }
  return efixed;
}

/**
 * Calculate the Q value
 * @param deltaE The energy transfer in meV
 * @param twoTheta The scattering angle in radians
 * @param det A pointer to the corresponding detector, can be nullptr
 *        for direct emode.
 * @return The momentum transfer in A-1
 */
double SofQCommon::q(const double deltaE, const double twoTheta, const Geometry::IDetector *det) const {
  if (m_emode == 1) {
    return directQ(deltaE, twoTheta);
  }
  return indirectQ(deltaE, twoTheta, det);
}

/**
 * Return a pair of (minimum Q, maximum Q) for given workspace.
 * @param ws a workspace
 * @param minE minimum energy transfer in ws
 * @param maxE maximum energy transfer in ws
 * @return a pair containing global minimun and maximum Q
 */
std::pair<double, double> SofQCommon::qBinHints(const API::MatrixWorkspace &ws, const double minE,
                                                const double maxE) const {
  if (m_emode == 1) {
    return qBinHintsDirect(ws, minE, maxE);
  }
  return qBinHintsIndirect(ws, minE, maxE);
}

/**
 * Calculate the Q value for a direct instrument
 * @param deltaE The energy change
 * @param twoTheta The value of the scattering angle
 * @return The value of Q
 */
double SofQCommon::directQ(const double deltaE, const double twoTheta) const {
  using Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq;
  const double ki = std::sqrt(m_efixed / E_mev_toNeutronWavenumberSq);
  const double kf = std::sqrt((m_efixed - deltaE) / E_mev_toNeutronWavenumberSq);
  return std::sqrt(ki * ki + kf * kf - 2. * ki * kf * std::cos(twoTheta));
}

/**
 * Calculate the Q value for an  indirect instrument
 * @param deltaE The energy change
 * @param twoTheta The value of the scattering angle
 * @param det A pointer to the corresponding Detector
 * @return The value of Q
 */
double SofQCommon::indirectQ(const double deltaE, const double twoTheta, const Geometry::IDetector *det) const {
  using Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq;
  if (!det) {
    throw std::runtime_error("indirectQ: det is nullptr.");
  }
  const auto efixed = getEFixed(*det);
  const double ki = std::sqrt((efixed + deltaE) / E_mev_toNeutronWavenumberSq);
  const double kf = std::sqrt(efixed / E_mev_toNeutronWavenumberSq);
  return std::sqrt(ki * ki + kf * kf - 2. * ki * kf * std::cos(twoTheta));
}

/**
 * Return a pair of (minimum Q, maximum Q) for given
 * direct geometry workspace.
 * @param ws a workspace
 * @param minE minimum energy transfer in ws
 * @param maxE maximum energy transfer in ws
 * @return a pair containing global minimun and maximum Q
 */
std::pair<double, double> SofQCommon::qBinHintsDirect(const API::MatrixWorkspace &ws, const double minE,
                                                      const double maxE) const {
  using namespace Mantid::PhysicalConstants;
  if (maxE > m_efixed) {
    throw std::invalid_argument("Cannot compute Q binning range: maximum "
                                "energy transfer is greater than the incident "
                                "energy.");
  }
  auto minTwoTheta = std::numeric_limits<double>::max();
  auto maxTwoTheta = std::numeric_limits<double>::lowest();
  const auto &spectrumInfo = ws.spectrumInfo();
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto twoTheta = spectrumInfo.twoTheta(i);
    minTwoTheta = std::min(minTwoTheta, twoTheta);
    maxTwoTheta = std::max(maxTwoTheta, twoTheta);
  }
  if (minTwoTheta == std::numeric_limits<double>::max()) {
    throw std::runtime_error("Could not determine Q binning: workspace does "
                             "not contain usable spectra.");
  }
  std::array<double, 4> qArr;
  qArr[0] = directQ(minE, minTwoTheta);
  qArr[1] = directQ(minE, maxTwoTheta);
  qArr[2] = directQ(maxE, minTwoTheta);
  qArr[3] = directQ(maxE, maxTwoTheta);
  const auto minmaxQ = std::minmax_element(qArr.cbegin(), qArr.cend());
  return std::make_pair(*minmaxQ.first, *minmaxQ.second);
}

/**
 * Return a pair of (minimum Q, maximum Q) for given
 * indirect geometry workspace. Estimates the Q range from all detectors.
 * If workspace contains grouped detectors/not all detectors are linked
 * to a spectrum, the returned interval may be larger than actually needed.
 * @param ws a workspace
 * @param minE minimum energy transfer in ws
 * @param maxE maximum energy transfer in ws
 * @return a pair containing global minimun and maximum Q
 */
std::pair<double, double> SofQCommon::qBinHintsIndirect(const API::MatrixWorkspace &ws, const double minE,
                                                        const double maxE) const {
  using namespace Mantid::PhysicalConstants;
  auto minQ = std::numeric_limits<double>::max();
  auto maxQ = std::numeric_limits<double>::lowest();
  const auto &detectorInfo = ws.detectorInfo();
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMasked(i) || detectorInfo.isMonitor(i)) {
      continue;
    }
    const auto twoTheta = detectorInfo.twoTheta(i);
    const auto &det = detectorInfo.detector(i);
    const auto Q1 = indirectQ(minE, twoTheta, &det);
    const auto Q2 = indirectQ(maxE, twoTheta, &det);
    if (!std::isfinite(Q1) || !std::isfinite(Q2)) {
      throw std::invalid_argument("Cannot compute Q binning range: non-finite "
                                  "Q found for detector ID " +
                                  std::to_string(detectorInfo.detectorIDs()[i]));
    }
    const auto minmaxQ = std::minmax(Q1, Q2);
    minQ = std::min(minQ, minmaxQ.first);
    maxQ = std::max(maxQ, minmaxQ.second);
  }
  if (minQ == std::numeric_limits<double>::max()) {
    throw std::runtime_error("Could not determine Q binning: workspace does "
                             "not contain usable spectra.");
  }
  return std::make_pair(minQ, maxQ);
}
} // namespace Mantid::Algorithms
