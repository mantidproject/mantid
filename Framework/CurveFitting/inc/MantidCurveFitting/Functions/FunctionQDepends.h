#ifndef MANTID_CURVEFITTING_FUNCTIONQDEPENDS_H_
#define MANTID_CURVEFITTING_FUNCTIONQDEPENDS_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>

// Mantid Headers from the same project
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
// Mantid headers from other projects
// N/A
// 3rd party library headers
// N/A
// Standard library
// N/A

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** This is a specialization of IFunction1D for functions having the magnitude
    of the momentum transfer (Q) as attribute.

    Main features of this interface:
     - Declares attributes "Q" and "WorkspaceIndex"
     - Implements setMatrixWorkspace
     - Extracts or compute Q values for each spectra, if possible

    @author Jose Borreguero, NScD-ORNL
    @date 12/10/2016

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

class DLLExport FunctionQDepends : public Mantid::API::IFunction1D,
                                   public Mantid::API::ParamFunction {

public:
  /* -------------------
     Overridden methods
    -------------------*/
  virtual void declareAttributes() override;
  virtual void
  setAttribute(const std::string &attName,
               const Mantid::API::IFunction::Attribute &attValue) override;
  void setMatrixWorkspace(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
      size_t wi, double startX, double endX) override;

private:
  std::vector<double>
  extractQValues(const Mantid::API::MatrixWorkspace &workspace);
  // list of Q values associated to the spectra
  std::vector<double> m_vQ;

}; // end of class FunctionQDepends

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FUNCTIONQDEPENDS_H_*/
