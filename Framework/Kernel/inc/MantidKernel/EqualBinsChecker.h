// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** EqualBinsChecker : Checks for evenly spaced bins

  This class works on x data from a workspace and checks that the bins
  are equally spaced. The tolerance thresholds for rejecting the bins
  and warning the user are both adjustable.
*/
class MANTID_KERNEL_DLL EqualBinsChecker {
public:
  /// Type of bin to compare others to
  enum class ReferenceBin { Average, First };
  /// Type of errors to check
  enum class ErrorType { Cumulative, Individual };
  EqualBinsChecker(const MantidVec &xData, const double errorLevel, const double warningLevel = -1);
  virtual ~EqualBinsChecker() = default;
  virtual std::string validate() const;
  virtual void setReferenceBin(const ReferenceBin &refBinType);
  virtual void setErrorType(const ErrorType &errorType);

protected:
  virtual double getReferenceDx() const;
  virtual double getDifference(const size_t bin, const double dx) const;

private:
  const MantidVec &m_xData;
  const double m_errorLevel;
  const bool m_warn;
  const double m_warningLevel;
  ReferenceBin m_refBinType;
  ErrorType m_errorType;
};

} // namespace Kernel
} // namespace Mantid
