#ifndef MANTID_KERNEL_EQUALBINSCHECKER_H_
#define MANTID_KERNEL_EQUALBINSCHECKER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** EqualBinsChecker : Checks for evenly spaced bins

  This class works on x data from a workspace and checks that the bins
  are equally spaced. The tolerance thresholds for rejecting the bins
  and warning the user are both adjustable.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL EqualBinsChecker {
public:
  /// Type of bin to compare others to
  enum class ReferenceBin { Average, First };
  /// Type of errors to check
  enum class ErrorType { Cumulative, Individual };
  EqualBinsChecker(const MantidVec &xData, const double errorLevel,
                   const double warningLevel = -1);
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

#endif /* MANTID_KERNEL_EQUALBINSCHECKER_H_ */