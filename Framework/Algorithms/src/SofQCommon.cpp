#include "MantidAlgorithms/SofQCommon.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {
/** The procedure analyses emode and efixed properties provided to the algorithm
 *and identify the energy analysis mode and the way the properties are defined
 *@param workspace     :: input workspace which may or may not have incident
 *energy property (Ei) attached to it as the run log
 *@param hostAlgorithm :: the pointer to SofQ algorithm hosting the base class.
 *This algorithm expects to have EMode and EFixed properties attached to it.
*/
void SofQCommon::initCachedValues(const API::MatrixWorkspace &workspace,
                                  API::Algorithm *const hostAlgorithm) {
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
        Kernel::PropertyWithValue<double> *eiProp =
            dynamic_cast<Kernel::PropertyWithValue<double> *>(p);
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
      std::vector<double> param = det.getNumberParameter("EFixed");
      if (param.empty())
        throw std::runtime_error(
            "Cannot find EFixed parameter for component \"" + det.getName() +
            "\". This is required in indirect mode. Please check the IDF "
            "contains these values.");
      efixed = param[0];
    }
  }
  return efixed;
}

/**
 * Return a pair of (minimum Q, maximum Q) for given workspace.
 * @param ws a workspace
 * @param minE minimum energy transfer in ws
 * @param maxE maximum energy transfer in ws
 * @return a pair containing global minimun and maximum Q
 */
std::pair<double, double> SofQCommon::qBinHints(const API::MatrixWorkspace &ws,
                                                const double minE,
                                                const double maxE) const {
  if (m_emode == 1) {
    return qBinHintsDirect(ws, minE, maxE);
  }
  return qBinHintsIndirect(ws, minE, maxE);
}

/**
 * Return a pair of (minimum Q, maximum Q) for given
 * direct geometry workspace.
 * @param ws a workspace
 * @param minE minimum energy transfer in ws
 * @param maxE maximum energy transfer in ws
 * @return a pair containing global minimun and maximum Q
 */
std::pair<double, double>
SofQCommon::qBinHintsDirect(const API::MatrixWorkspace &ws, const double minE,
                            const double maxE) const {
  using namespace Mantid::PhysicalConstants;
  auto minTheta = std::numeric_limits<double>::max();
  auto maxTheta = std::numeric_limits<double>::lowest();
  const auto &spectrumInfo = ws.spectrumInfo();
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto theta = spectrumInfo.twoTheta(i);
    if (theta < minTheta) {
      minTheta = theta;
    }
    if (theta > maxTheta) {
      maxTheta = theta;
    }
  }
  if (minTheta == std::numeric_limits<double>::max()) {
    throw std::runtime_error("Could not determine Q binning: workspace does "
                             "not contain usable spectra.");
  }
  const auto incidentKSq = m_efixed / E_mev_toNeutronWavenumberSq;
  const auto incidentK = std::sqrt(incidentKSq);
  const auto minEnergy = m_efixed - maxE;
  const auto maxEnergy = m_efixed - minE;
  const auto minKSq = minEnergy / E_mev_toNeutronWavenumberSq;
  const auto minK = std::sqrt(minKSq);
  const auto maxKSq = maxEnergy / E_mev_toNeutronWavenumberSq;
  const auto maxK = std::sqrt(maxKSq);
  std::array<double, 4> q;
  q[0] = std::sqrt(incidentKSq + minKSq -
                   2. * incidentK * minK * std::cos(minTheta));
  q[1] = std::sqrt(incidentKSq + minKSq -
                   2. * incidentK * minK * std::cos(maxTheta));
  q[2] = std::sqrt(incidentKSq + maxKSq -
                   2. * incidentK * maxK * std::cos(minTheta));
  q[3] = std::sqrt(incidentKSq + maxKSq -
                   2. * incidentK * maxK * std::cos(maxTheta));
  const auto minmaxQ = std::minmax_element(q.cbegin(), q.cend());
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
std::pair<double, double>
SofQCommon::qBinHintsIndirect(const API::MatrixWorkspace &ws, const double minE,
                              const double maxE) const {
  using namespace Mantid::PhysicalConstants;
  auto minQSq = std::numeric_limits<double>::max();
  auto maxQSq = std::numeric_limits<double>::lowest();
  const auto &detectorInfo = ws.detectorInfo();
  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMasked(i) || detectorInfo.isMonitor(i)) {
      continue;
    }
    const auto costheta = std::cos(detectorInfo.twoTheta(i));
    const auto eFixed = getEFixed(detectorInfo.detector(i));
    const auto minEnergy = eFixed + minE;
    const auto maxEnergy = eFixed + maxE;
    const auto minKSq = minEnergy / E_mev_toNeutronWavenumberSq;
    const auto minK = std::sqrt(minKSq);
    const auto maxKSq = maxEnergy / E_mev_toNeutronWavenumberSq;
    const auto maxK = std::sqrt(maxKSq);
    const auto finalKSq = eFixed / E_mev_toNeutronWavenumberSq;
    const auto finalK = std::sqrt(finalKSq);
    const auto QSq1 = minKSq + finalKSq - 2. * minK * finalK * costheta;
    const auto QSq2 = maxKSq + finalKSq - 2. * maxK * finalK * costheta;
    const auto minmaxQSq = std::minmax(QSq1, QSq2);
    if (minmaxQSq.first < minQSq) {
      minQSq = minmaxQSq.first;
    }
    if (minmaxQSq.second > maxQSq) {
      maxQSq = minmaxQSq.second;
    }
  }
  if (minQSq == std::numeric_limits<double>::max()) {
    throw std::runtime_error("Could not determine Q binning: workspace does "
                             "not contain usable spectra.");
  }
  return std::make_pair(std::sqrt(minQSq), std::sqrt(maxQSq));
}
}
}
