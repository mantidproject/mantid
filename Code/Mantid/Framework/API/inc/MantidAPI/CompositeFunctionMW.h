#ifndef MANTID_API_COMPOSITEFUNCTIONMW_H_
#define MANTID_API_COMPOSITEFUNCTIONMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunctionMW.h"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace API
{
/** A composite function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 20/10/2009

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
class DLLExport CompositeFunctionMW : public CompositeFunction, public IFunctionMW
{
public:
  /// Default constructor
  CompositeFunctionMW():CompositeFunction(),IFunctionMW(){}
  /// Copy contructor
  CompositeFunctionMW(const CompositeFunctionMW&);
  ///Assignment operator
  //CompositeFunctionMW& operator=(const CompositeFunctionMW&);
  ///Destructor
  virtual ~CompositeFunctionMW();

              /* Overriden methods */

  void setWorkspace(boost::shared_ptr<const Workspace> ws,const std::string& slicing,bool copyData = true);
  /// Set the workspace
  void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int spec,int xMin,int xMax);
  /// Returns the function's name
  std::string name()const{return "CompositeFunctionMW";}
  /// Writes itself into a string
  std::string asString()const;

  /// Function you want to fit to.
  void function(double* out, const double* xValues, const int& nData)const;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData);
  /// Derivatives to be used in covariance matrix calculation. 
  void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  void setUpNewStuff(boost::shared_array<double> xs = boost::shared_array<double>(),boost::shared_array<double> weights = boost::shared_array<double>());

};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEFUNCTIONMW_H_*/
