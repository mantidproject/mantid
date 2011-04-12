#ifndef MANTID_API_COMPOSITEFUNCTIONMD_H_
#define MANTID_API_COMPOSITEFUNCTIONMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunctionMD.h"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace API
{
/** A composite function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 01/04/2011

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
class DLLExport CompositeFunctionMD : public CompositeFunction, public IFunctionMD
{
public:
  /// Default constructor
  CompositeFunctionMD():CompositeFunction(),IFunctionMD(){}
  /// Copy contructor
  //CompositeFunctionMD(const CompositeFunctionMD&);
  ///Assignment operator
  //CompositeFunctionMD& operator=(const CompositeFunctionMD&);
  ///Destructor
  virtual ~CompositeFunctionMD();

              /* Overriden methods */

  /// Set the workspace
  void setWorkspace(boost::shared_ptr<const Workspace> ws,const std::string& slicing, bool copyData = true);
  /// Returns the function's name
  std::string name()const{return "CompositeFunctionMD";}
  /// Writes itself into a string
  //std::string asString()const;

  /// Function you want to fit to.
  void function(double* out)const;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(Jacobian* out);

protected:

  double function(IMDIterator& r) const;

  /// to collect different workspaces found in child functions
  std::vector< boost::shared_ptr<const IMDWorkspace> > m_workspaces;
  /// to store function indices to workspaces
  std::vector< std::vector<size_t> > m_wsIndex;
  /// offsets of particular workspaces in the m_data and m_weights arrays
  std::vector<size_t> m_offset;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_CompositeFunctionMD_H_*/
