// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;
} // namespace API

namespace Algorithms {

/** TimeAtSampleStrategyIndirect : Determine Time At Sample for an indirect
  instrument setup.
*/
class DLLExport TimeAtSampleStrategyIndirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyIndirect(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Workspace to operate on
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ws;
  const API::SpectrumInfo &m_spectrumInfo;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_ */
