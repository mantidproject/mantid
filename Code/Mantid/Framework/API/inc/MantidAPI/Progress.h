#ifndef MANTID_API_PROGRESS_H_
#define MANTID_API_PROGRESS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/ProgressBase.h"

namespace Mantid
{
namespace API
{
class Algorithm;

/** 
 Helper class for reporting progress from algorithms.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 06/02/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport Progress : public Mantid::Kernel::ProgressBase
{
public:
  Progress();
  Progress(Algorithm* alg,double start,double end, int numSteps);
  virtual ~Progress();

  void doReport(const std::string& msg = "");

private:
  /// Owning algorithm
  Algorithm* const m_alg;

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PROGRESS_H_*/
