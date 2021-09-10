// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"

#include <list>

namespace Mantid {
namespace Algorithms {

/** SumOverlappingTubes : Converts workspaces containing an instrument with PSD
  tubes into a workspace with counts as a function of height and scattering
  angle. Currently works for the D2B instrument geometry.
*/
class MANTID_ALGORITHMS_DLL SumOverlappingTubes : public API::Algorithm {
public:
  const std::string name() const override { return "SumOverlappingTubes"; }
  const std::string category() const override { return "ILL\\Diffraction"; }
  const std::string summary() const override {
    return "Takes workspaces containing an instrument with PSD and tubes, and "
           "converts to a workspace with counts as a function of height and "
           "scattering angle. Detector scans with overlapping tubes are "
           "supported.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SumSpectra"}; }

private:
  void init() override;
  void exec() override;

  std::list<API::MatrixWorkspace_sptr> m_workspaceList;

  size_t m_numHistograms;
  size_t m_numPoints;

  double m_startScatteringAngle;
  double m_endScatteringAngle;
  double m_stepScatteringAngle;

  double m_startHeight;
  double m_endHeight;
  std::vector<double> m_heightAxis;
  std::string m_outputType;

  void getInputParameters();
  void getScatteringAngleBinning();
  void getHeightAxis(const std::string &componentName);
  std::vector<std::vector<double>> performBinning(API::MatrixWorkspace_sptr &outputWS);

  double distanceFromAngle(const int angleIndex, const double angle) const;

  int m_mirrorDetectors; /// holds the sign flipper for 2theta

  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid
