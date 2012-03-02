#ifndef MANTID_API_IFUNCTIONMW_H_
#define MANTID_API_IFUNCTIONMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"

#include <boost/weak_ptr.hpp>

namespace Mantid
{

namespace API
{

/** This is an interface to a fitting function - a semi-abstarct class.
    Functions derived from IFunctionMW can be used with the Fit algorithm.
    IFunctionMW defines the structure of a fitting funtion.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class MANTID_API_DLL IFunctionMW: public virtual IFunction
{
public:

  void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wi,double startX, double endX);

  boost::shared_ptr<const API::MatrixWorkspace> getMatrixWorkspace() const;
  size_t getWorkspaceIndex() const {return m_workspaceIndex;}
protected:

  /// Convert a value from one unit (inUnit) to unit defined in workspace (ws) 
  double convertValue(double value, Kernel::Unit_sptr& inUnit, 
                      boost::shared_ptr<const MatrixWorkspace> ws,
                      size_t wsIndex)const;

  void convertValue(std::vector<double>& values, Kernel::Unit_sptr& outUnit, 
    boost::shared_ptr<const MatrixWorkspace> ws,
    size_t wsIndex) const;
  
  boost::weak_ptr<const API::MatrixWorkspace> m_workspace;

  size_t m_workspaceIndex;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTIONMW_H_*/
