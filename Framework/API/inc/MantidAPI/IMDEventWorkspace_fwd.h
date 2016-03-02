#ifndef IMDEVENTWORKSPACE_FWD_H_
#define IMDEVENTWORKSPACE_FWD_H_

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace API {

/**
  This file provides forward declarations for Mantid::API::IMDEventWorkspace

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

/// forward declare of Mantid::API::IMDEventWorkspace
class IMDEventWorkspace;
/// Shared pointer to Mantid::API::IMDEventWorkspace
typedef boost::shared_ptr<IMDEventWorkspace> IMDEventWorkspace_sptr;
/// Shared pointer to Mantid::API::IMDEventWorkspace (const version)
typedef boost::shared_ptr<const IMDEventWorkspace> IMDEventWorkspace_const_sptr;
/// unique pointer to Mantid::API::IMDEventWorkspace
typedef std::unique_ptr<IMDEventWorkspace> IMDEventWorkspace_uptr;
/// unique pointer to Mantid::API::IMDEventWorkspace (const version)
typedef std::unique_ptr<const IMDEventWorkspace> IMDEventWorkspace_const_uptr;

} // namespace MDEvents
} // namespace Mantid

#endif /* IMDEVENTWORKSPACE_FWD_H_ */
