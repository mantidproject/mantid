#ifndef MANTID_API_JOINTDOMAIN_H_
#define MANTID_API_JOINTDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/CompositeDomain.h"

#include <stdexcept>
#include <vector>
#include <algorithm>

namespace Mantid {
namespace API {
/** An implementation of CompositeDomain.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

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
class MANTID_API_DLL JointDomain : public CompositeDomain {
public:
  /// Return the number of points in the domain
  virtual size_t size() const;
  /// Return the number of parts in the domain
  virtual size_t getNParts() const;
  /// Return i-th domain
  virtual const FunctionDomain &getDomain(size_t i) const;
  void addDomain(FunctionDomain_sptr domain);

protected:
  /// Vector with member domains.
  std::vector<FunctionDomain_sptr> m_domains;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_JOINTDOMAIN_H_*/
