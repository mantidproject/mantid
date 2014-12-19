#ifndef MANTID_API_IFUNCTIONMW_H_
#define MANTID_API_IFUNCTIONMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"

#ifndef Q_MOC_RUN
#include <boost/weak_ptr.hpp>
#endif

namespace Mantid {

namespace API {

/** This is a specialization of IFunction for functions defined on a
   MatrixWorkspace.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class MANTID_API_DLL IFunctionMW : public virtual IFunction {
public:
  /// Set MatrixWorkspace
  void
  setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,
                     size_t wi, double startX, double endX);
  /// Get shared pointer to the workspace
  boost::shared_ptr<const API::MatrixWorkspace> getMatrixWorkspace() const;
  /// Get the workspace index
  size_t getWorkspaceIndex() const { return m_workspaceIndex; }

protected:
  /// Keep a weak pointer to the workspace
  boost::weak_ptr<const API::MatrixWorkspace> m_workspace;
  /// An index to a spectrum
  size_t m_workspaceIndex;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTIONMW_H_*/
