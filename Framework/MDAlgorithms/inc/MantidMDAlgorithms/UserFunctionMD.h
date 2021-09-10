// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace MDAlgorithms {
/**
A user defined function.

@author Roman Tolchenov, Tessella plc
@date 15/01/2010
*/

class DLLExport UserFunctionMD : virtual public API::IFunctionMD, virtual public API::ParamFunction {
public:
  UserFunctionMD();
  std::string name() const override { return "UserFunctionMD"; }

  std::vector<std::string> getAttributeNames() const override;
  bool hasAttribute(const std::string &attName) const override;
  Attribute getAttribute(const std::string &attName) const override;
  void setAttribute(const std::string &attName, const Attribute &attr) override;
  /**
   * Defining function's parameters here, ie after the workspace is set and
   * the dimensions are known.
   */
  void initDimensions() override;

protected:
  /**
   * Calculate the function value at a point r in the MD workspace
   * @param r :: MD workspace iterator with a reference to the current point
   */
  double functionMD(const API::IMDIterator &r) const override;
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
