#ifndef MANTID_MDALGORITHMS_USERFUNCTIONMD_H_
#define MANTID_MDALGORITHMS_USERFUNCTIONMD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace MDAlgorithms {
/**
A user defined function.

@author Roman Tolchenov, Tessella plc
@date 15/01/2010

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport UserFunctionMD : virtual public API::IFunctionMD,
                                 virtual public API::ParamFunction {
public:
  UserFunctionMD();
  std::string name() const { return "UserFunctionMD"; }

  std::vector<std::string> getAttributeNames() const;
  bool hasAttribute(const std::string &attName) const;
  Attribute getAttribute(const std::string &attName) const;
  void setAttribute(const std::string &attName, const Attribute &attr);
  /**
    * Defining function's parameters here, ie after the workspace is set and
    * the dimensions are known.
    */
  void initDimensions();

protected:
  /**
    * Calculate the function value at a point r in the MD workspace
    * @param r :: MD workspace iterator with a reference to the current point
    */
  double functionMD(const API::IMDIterator &r) const;
  /** Static callback function used by MuParser to initialize variables
  implicitly
  @param varName :: The name of a new variable
  @param pufun :: Pointer to the function
  */
  static double *AddVariable(const char *varName, void *pufun);

  /**
    * Initializes the mu::Parser.
    */
  void setFormula();

private:
  /// Expression parser
  mu::Parser m_parser;
  ///
  mutable std::vector<double> m_vars;
  std::vector<std::string> m_varNames;
  std::string m_formula;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_USERFUNCTIONMD_H_*/
