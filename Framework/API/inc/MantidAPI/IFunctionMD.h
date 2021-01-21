// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Unit.h"
#ifndef Q_MOC_RUN
#include <boost/scoped_array.hpp>
#include <boost/variant.hpp>
#endif
#include <memory>
#include <string>
#include <vector>

namespace Mantid {

namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class IMDWorkspace;
class IMDIterator;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
class FunctionDomainMD;

/** This is a specialization of IFunction for functions defined on an
   IMDWorkspace.
    It uses FunctionDomainMD as a domain. Implement functionMD(...) method in a
   concrete
    function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 12/01/2011
*/
class MANTID_API_DLL IFunctionMD : public virtual IFunction {
public:
  /* Overidden methods */

  /// Virtual copy constructor
  std::shared_ptr<IFunction> clone() const override;
  /// Set the workspace.
  /// @param ws :: Shared pointer to a workspace
  void setWorkspace(std::shared_ptr<const Workspace> ws) override;

  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override { calNumericalDeriv(domain, jacobian); }

protected:
  /// Performs the function evaluations on the MD domain
  void evaluateFunction(const FunctionDomainMD &domain, FunctionValues &values) const;

  virtual void useDimension(const std::string &id);
  /// Do finction initialization after useAllDimensions() was called
  virtual void initDimensions() {}
  /// Does the function evaluation. Must be implemented in derived classes.
  virtual double functionMD(const IMDIterator &r) const = 0;

  /// maps dimension id to its index in m_dimensions
  std::map<std::string, size_t> m_dimensionIndexMap;
  /// dimensions used in this function in the expected order
  std::vector<std::shared_ptr<const Mantid::Geometry::IMDDimension>> m_dimensions;

private:
  /// Use all the dimensions in the workspace
  virtual void useAllDimensions(std::shared_ptr<const IMDWorkspace> workspace);
};

} // namespace API
} // namespace Mantid
