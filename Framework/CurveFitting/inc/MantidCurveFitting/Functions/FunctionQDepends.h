// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>

// Mantid Headers from the same project
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
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
*/

class MANTID_CURVEFITTING_DLL FunctionQDepends : virtual public Mantid::API::IFunction1D,
                                                 virtual public Mantid::API::ParamFunction {

public:
  /* -------------------
     Overridden methods
    -------------------*/
  virtual void declareAttributes() override;
  virtual void setAttribute(const std::string &attName, const Mantid::API::IFunction::Attribute &attValue) override;
  void setMatrixWorkspace(std::shared_ptr<const Mantid::API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;

private:
  std::vector<double> extractQValues(const Mantid::API::MatrixWorkspace &workspace);
  // list of Q values associated to the spectra
  std::vector<double> m_vQ;

}; // end of class FunctionQDepends

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
