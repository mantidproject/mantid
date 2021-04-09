// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"

#include <utility> // std::pair

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {

/** ConvertEmptyToTof :

 At the ILL the data is loaded in raw format : no units used. The X-axis
 represent the time channel number.
 This algorithm converts the channel number to time of flight
 */
class MANTID_ALGORITHMS_DLL ConvertEmptyToTof : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Converts the channel number to time of flight."; }

private:
  void init() override;
  void exec() override;

  void validateWorkspaceIndices(std::vector<int> &v);
  void validateChannelIndices(std::vector<int> &v);

  std::map<int, int> findElasticPeakPositions(const std::vector<int> &, const std::vector<int> &);

  void estimateFWHM(const Mantid::HistogramData::HistogramY &, double &, double &, double &, double &, double &);

  bool doFitGaussianPeak(int, double &, double &, double &, double, double);
  std::pair<int, double> findAverageEppAndEpTof(const std::map<int, int> &);

  double calculateTOF(double, double);
  bool areEqual(double, double, double);
  int roundUp(double);
  std::vector<double> makeTofAxis(int, double, size_t, double);
  void setTofInWS(const std::vector<double> &, const API::MatrixWorkspace_sptr &);

  DataObjects::Workspace2D_sptr m_inputWS;
  API::MatrixWorkspace_sptr m_outputWS;
  // Provide hint to compiler that this should be default initialized to nullptr
  const API::SpectrumInfo *m_spectrumInfo = nullptr;
};

} // namespace Algorithms
} // namespace Mantid
