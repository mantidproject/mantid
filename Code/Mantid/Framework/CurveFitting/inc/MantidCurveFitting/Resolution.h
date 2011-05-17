#ifndef MANTID_CURVEFITTING_RESOLUTION_H_
#define MANTID_CURVEFITTING_RESOLUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{
/**
Resolution function

@author Roman Tolchenov, Tessella plc
@date 12/02/2010

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Resolution : public API::ParamFunction, public API::IFunctionMW
{
public:
  /// Constructor
  Resolution():m_xStart(0),m_xEnd(0){}
  /// Destructor
  virtual ~Resolution() {};

  /// overwrite IFunction base class methods
  std::string name()const{return "Resolution";}
  void function(double* out, const double* xValues, const size_t nData)const;
  ///  function derivatives
  void functionDeriv(API::Jacobian* out, const double* xValues, const size_t nData)const
  {
    (void) out; (void) xValues; (void) nData; //Avoid compiler warning
  }

  /// Returns the number of attributes associated with the function
  int nAttributes()const{return 1;}
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames()const;
  /// Return a value of attribute attName
  IFitFunction::Attribute getAttribute(const std::string& attName)const{return IFitFunction::Attribute(m_fileName, true);}
  /// Set a value to attribute attName
  void setAttribute(const std::string& attName,const IFitFunction::Attribute& value);
  /// Check if attribute attName exists
  bool hasAttribute(const std::string& attName)const{return attName == "FileName";}

private:

  /// Call the appropriate load function
  void load(const std::string& fname);

  /// Load the resolution from an ASCII file
  void loadAscii(const std::string& fname);

  /// Load the resolution from a NeXuS file
  void loadNexus(const std::string& fname);

  /// Size of the data
  size_t size()const{return m_yData.size();}

  /// The file name
  std::string m_fileName;

  /// Stores x-values
  std::vector<double> m_xData;

  /// Stores y-values
  std::vector<double> m_yData;

  /// The first x
  double m_xStart;

  /// The lasst x
  double m_xEnd;

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_RESOLUTION_H_*/
