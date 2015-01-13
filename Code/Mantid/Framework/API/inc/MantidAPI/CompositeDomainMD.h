#ifndef MANTID_API_COMPOSITEDOMAINMD_H_
#define MANTID_API_COMPOSITEDOMAINMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/IMDWorkspace.h"

#include <vector>

namespace Mantid {
namespace API {
class FunctionDomainMD;

/**
    A composite domain for MD functions.


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
class MANTID_API_DLL CompositeDomainMD : public CompositeDomain {
public:
  CompositeDomainMD(IMDWorkspace_const_sptr ws, size_t maxDomainSize);
  ~CompositeDomainMD();
  /// Return the total number of arguments in the domain
  virtual size_t size() const { return m_totalSize; }
  /// Return the number of parts in the domain
  virtual size_t getNParts() const { return m_domains.size(); }
  /// Return i-th domain
  virtual const FunctionDomain &getDomain(size_t i) const;

protected:
  mutable IMDIterator *m_iterator; ///< IMDIterator
  size_t m_totalSize;              ///< The total size of the domain
  mutable std::vector<FunctionDomainMD *>
      m_domains; ///< smaller parts of the domain
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEDOMAINMD_H_*/
