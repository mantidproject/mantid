// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/V3D.h"
#include <iomanip>

namespace Mantid {
using Kernel::V3D;

namespace Algorithms {

/**
 * Construct the statistics object. Look up the environment component names
 * from the supplied sample
 * @param detectorID ID of the detector that is the end points of the tracks
 * that this object is measuring statistics on
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 */
MCInteractionStatistics::MCInteractionStatistics(const detid_t detectorID, const API::Sample &sample)
    : m_detectorID(detectorID) {
  const Geometry::SampleEnvironment *env = nullptr;
  try {
    env = &sample.getEnvironment();
    assert(env);
    if (env->nelements() == 0) {
      throw std::invalid_argument("MCInteractionStatistics() - Sample enviroment has zero components.");
    }
  } catch (std::runtime_error &) {
    // swallow this as no defined environment from getEnvironment
  }

  if (env) {
    for (size_t i = 0; i < env->nelements(); i++) {
      m_envScatterPoints.push_back({env->getComponent(i).id(), 0, 0});
    }
  }
}

/**
 * Update the scatter point counts
 * @param componentIndex Index of the sample/environment component where
 * the sample is -1
 * @param pointUsed Whether the generated point is actually used in the
 * simulation. If no valid tracks before and after scatter are generated
 * the point will be discarded
 */
void MCInteractionStatistics::UpdateScatterPointCounts(int componentIndex, bool pointUsed) {
  ScatterPointStat *StatToUpdate;
  if (componentIndex == -1) {
    StatToUpdate = &m_sampleScatterPoints;
  } else {
    StatToUpdate = &m_envScatterPoints[componentIndex];
  }
  if (pointUsed) {
    StatToUpdate->usedPointCount++;
  } else {
    StatToUpdate->generatedPointCount++;
  }
}

/**
 * Update the scattering angle statistics
 * @param toStart Vector from scatter point to point on beam profile where
 * @param scatteredDirec Vector from scatter point to detector
 */
void MCInteractionStatistics::UpdateScatterAngleStats(V3D toStart, V3D scatteredDirec) {
  double scatterAngleDegrees = scatteredDirec.angle(-toStart) * 180. / M_PI;
  double delta = scatterAngleDegrees - m_scatterAngleMean;
  int totalScatterPoints = m_sampleScatterPoints.usedPointCount;
  for (const auto &stat : m_envScatterPoints) {
    totalScatterPoints += stat.usedPointCount;
  }
  m_scatterAngleMean += delta / totalScatterPoints;
  m_scatterAngleM2 += delta * (scatterAngleDegrees - m_scatterAngleMean);
  m_scatterAngleSD = sqrt(m_scatterAngleM2 / totalScatterPoints);
}

/**
 * Log a debug string summarising which parts of the environment
 * the simulated scatter points occurred in
 */
std::string MCInteractionStatistics::generateScatterPointStats() {

  std::stringstream scatterPointSummary;
  scatterPointSummary << std::fixed;
  scatterPointSummary << std::setprecision(2);

  scatterPointSummary << "Scatter point statistics" << std::endl;
  scatterPointSummary << "========================" << std::endl;

  scatterPointSummary << "Detector ID: " << m_detectorID << std::endl;

  int totalScatterPointsGenerated = m_sampleScatterPoints.generatedPointCount;
  int totalScatterPointsUsed = m_sampleScatterPoints.usedPointCount;
  for (const auto &stat : m_envScatterPoints) {
    totalScatterPointsGenerated += stat.generatedPointCount;
    totalScatterPointsUsed += stat.usedPointCount;
  }

  scatterPointSummary << "Total scatter points generated: " << totalScatterPointsGenerated << std::endl;
  scatterPointSummary << "Total scatter points used: " << totalScatterPointsUsed << std::endl;

  if (m_envScatterPoints.size() > 0) {
    double percentage = static_cast<double>(m_sampleScatterPoints.usedPointCount) / totalScatterPointsUsed * 100;
    scatterPointSummary << "Sample: " << m_sampleScatterPoints.usedPointCount << " (" << percentage << "%)"
                        << std::endl;

    for (std::vector<int>::size_type i = 0; i < m_envScatterPoints.size(); i++) {
      percentage = static_cast<double>(m_envScatterPoints[i].usedPointCount) / totalScatterPointsUsed * 100;
      scatterPointSummary << "Environment part " << i << " (" << m_envScatterPoints[i].name
                          << "): " << m_envScatterPoints[i].usedPointCount << " (" << percentage << "%)" << std::endl;
    }
  }
  scatterPointSummary << "Scattering angle mean (degrees)=" << m_scatterAngleMean << std::endl;
  scatterPointSummary << "Scattering angle sd (degrees)=" << m_scatterAngleSD << std::endl;

  return scatterPointSummary.str();
}

} // namespace Algorithms
} // namespace Mantid
