// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Algorithms {

/** VesuvioL1ThetaResolution

  Calculates the resolution function for L1 and Theta.
*/
class MANTID_ALGORITHMS_DLL VesuvioL1ThetaResolution final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void loadInstrument();

  void calculateDetector(const Mantid::Geometry::IDetector &detector,
                         const std::function<double()> &flatRandomVariateGen, std::vector<double> &l1Values,
                         std::vector<double> &thetaValues);
  Mantid::API::MatrixWorkspace_sptr processDistribution(Mantid::API::MatrixWorkspace_sptr ws, const double binWidth);

  Mantid::API::MatrixWorkspace_sptr m_instWorkspace;
  Mantid::Geometry::IComponent_const_sptr m_sample;
  Mantid::API::MatrixWorkspace_sptr m_outputWorkspace;
  Mantid::API::MatrixWorkspace_sptr m_l1DistributionWs;
  Mantid::API::MatrixWorkspace_sptr m_thetaDistributionWs;
};

} // namespace Algorithms
} // namespace Mantid
