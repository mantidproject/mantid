#ifndef MANTID_API_IMDHISTOWORKSPACE_FWD_H_
#define MANTID_API_IMDHISTOWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
/**
  This file provides forward declarations for Mantid::API::IMDHistoWorkspace

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/// forward declare of Mantid::API::IMDHistoWorkspace
class IMDHistoWorkspace;
/// shared pointer to Mantid::API::IMDHistoWorkspace
typedef boost::shared_ptr<IMDHistoWorkspace> IMDHistoWorkspace_sptr;
/// shared pointer to Mantid::API::IMDHistoWorkspace (const version)
typedef boost::shared_ptr<const IMDHistoWorkspace> IMDHistoWorkspace_const_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IMDHISTOWORKSPACE_FWD_H_ */
