#ifndef MANTID_API_MUTLIPERIODGROUPALGORITHM_H_
#define MANTID_API_MUTLIPERIODGROUPALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MultiPeriodGroupWorker.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace API {

/** MutliPeriodGroupAlgorithm : Abstract algorithm. Algorithms that need special
processing for Mutli-Period group workspaces should inherit from this
algorithm rather than from Algorithm directly. This algorithm processes
workspaces in each group input in a pair-wise fashion to give a group workspace
output.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MultiPeriodGroupAlgorithm : public Algorithm {
public:
  MultiPeriodGroupAlgorithm();
  virtual ~MultiPeriodGroupAlgorithm();

private:
  /// Overriden from Algorithm base
  virtual bool checkGroups();
  /// Overriden from Algorithm base.
  virtual bool processGroups();
  /// Method to provide the name for the input workspace property.
  virtual std::string fetchInputPropertyName() const = 0;
  /// Method to indicate that a non-standard property is taken as the input, so
  /// will be specified via fetchInputPropertyName.
  virtual bool useCustomInputPropertyName() const { return false; }

  /// Convenience typdef for workspace names.
  typedef MultiPeriodGroupWorker::VecWSGroupType VecWSGroupType;
  /// multi period group workspaces.
  VecWSGroupType m_multiPeriodGroups;
  /// Multiperiod group worker.
  boost::scoped_ptr<MultiPeriodGroupWorker> m_worker;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MUTLIPERIODGROUPALGORITHM_H_ */
