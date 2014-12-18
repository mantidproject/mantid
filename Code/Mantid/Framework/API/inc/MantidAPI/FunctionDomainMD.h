#ifndef MANTID_API_FUNCTIONDOMAINMD_H_
#define MANTID_API_FUNCTIONDOMAINMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid {
namespace API {
/** Implements a domain for MD functions (IFunctionMD).

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
class MANTID_API_DLL FunctionDomainMD : public FunctionDomain {
public:
  /// Constructor.
  FunctionDomainMD(IMDWorkspace_const_sptr ws, size_t start = 0,
                   size_t length = 0);
  /// Destructor.
  ~FunctionDomainMD();
  /// Return the number of arguments in the domain
  virtual size_t size() const { return m_size; }
  /// Reset the iterator to point to the start of the domain.
  virtual void reset() const;
  /// Next iterator.
  const IMDIterator *getNextIterator() const;
  /// Returns the pointer to the original workspace
  IMDWorkspace_const_sptr getWorkspace() const;

protected:
  /// IMDIterator
  mutable IMDIterator *m_iterator;
  /// start of the domain, 0 <= m_startIndex < m_iterator->getDataSize()
  const size_t m_startIndex;
  /// track the iterator's index, 0 <= m_currentIndex < m_size.
  mutable size_t m_currentIndex;
  /// The size of the domain
  size_t m_size;
  /// Just reset flag
  mutable bool m_justReset;

private:
  /// A pointer to the workspace
  IMDWorkspace_const_sptr m_workspace;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAINMD_H_*/
