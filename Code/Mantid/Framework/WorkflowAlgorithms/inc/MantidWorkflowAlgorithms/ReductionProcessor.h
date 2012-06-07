#ifndef MANTID_WORKFLOWALGORITHM_REDUCTION_PROCESSOR_H_
#define MANTID_WORKFLOWALGORITHM_REDUCTION_PROCESSOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include <vector>

namespace Mantid
{
namespace WorkflowAlgorithms
{
/** DataProcessorAlgorithm : TODO: DESCRIPTION

   @date 2012-04-04

   Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

   File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport ReductionProcessor: public API::DataProcessorAlgorithm
{
public:
  /// Algorithm's name
  virtual const std::string name() const { return "ReductionProcessor"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHM_REDUCTION_PROCESSOR_H_ */
