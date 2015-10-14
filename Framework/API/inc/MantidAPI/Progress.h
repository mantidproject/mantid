#ifndef MANTID_API_PROGRESS_H_
#define MANTID_API_PROGRESS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ProgressBase.h"

namespace Mantid {
namespace API {
class Algorithm;

/**
 Helper class for reporting progress from algorithms.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 06/02/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL Progress : public Mantid::Kernel::ProgressBase {
public:
  Progress();
  Progress(Algorithm *alg, double start, double end, int numSteps);
  Progress(Algorithm *alg, double start, double end, int64_t numSteps);
  Progress(Algorithm *alg, double start, double end, size_t numSteps);
  virtual ~Progress();

  void doReport(const std::string &msg = "");
  bool hasCancellationBeenRequested() const;

private:
  /// Owning algorithm
  Algorithm *const m_alg;
  Progress &operator=(const Progress &);
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROGRESS_H_*/
