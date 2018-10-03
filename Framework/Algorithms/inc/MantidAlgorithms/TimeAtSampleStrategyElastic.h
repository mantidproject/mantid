// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;
} // namespace API
namespace Algorithms {

/** TimeAtSampleStrategyElastic : Time at sample stragegy for elastic scattering
 */
class DLLExport TimeAtSampleStrategyElastic
    : public Mantid::Algorithms::TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyElastic(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ws;
  const API::SpectrumInfo &m_spectrumInfo;
  const Kernel::V3D m_beamDir;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_ */
