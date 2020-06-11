// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Logger.h"

namespace Mantid {

namespace Kernel {
class V3D;
} // namespace Kernel

namespace Algorithms {

struct ScatterPointStat {
  std::string name;
  int generatedPointCount;
  int usedPointCount;
};

/**
  Stores statistics relating to the tracks generated in MCInteractionVolume
  for a specific detector.
*/
class MANTID_ALGORITHMS_DLL MCInteractionStatistics {
public:
  MCInteractionStatistics(detid_t detectorID, const API::Sample &sample);
  std::string generateScatterPointStats();
  void UpdateScatterPointCounts(int componentIndex, bool pointUsed);
  void UpdateScatterAngleStats(Kernel::V3D toStart, Kernel::V3D scatteredDirec);

private:
  detid_t m_detectorID;
  ScatterPointStat m_sampleScatterPoints = {"Sample", 0, 0};
  std::vector<ScatterPointStat> m_envScatterPoints;
  double m_scatterAngleMean = 0;
  double m_scatterAngleM2 = 0;
  double m_scatterAngleSD = 0;
};

} // namespace Algorithms
} // namespace Mantid
