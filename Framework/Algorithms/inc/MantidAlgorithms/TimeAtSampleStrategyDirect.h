// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
}

namespace Algorithms {
/** TimeAtSampleStrategyDirect : Determine the Time at Sample corrections for a
  Direct Geometry instrument
*/
class DLLExport TimeAtSampleStrategyDirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyDirect(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws, double ei);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Cached L1, Ei dependent const shift
  double m_constShift;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_ */
