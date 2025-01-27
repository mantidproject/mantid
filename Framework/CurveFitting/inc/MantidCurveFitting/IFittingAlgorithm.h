// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {

namespace API {
class IFunction;
}

namespace CurveFitting {

namespace CostFunctions {
class CostFuncFitting;
}

/**

  A base class for fitting algorithms. It declares two properties:
  "Function" and "InputWorkspace". When these properties are set it
  creates an appropriate domain creator.

  The concrete classes must implement methods:
    - name()
    - version()
    - summary()
    - exec()
    - initConcrete() to declare more properties
*/
class MANTID_CURVEFITTING_DLL IFittingAlgorithm : public API::Algorithm {
public:
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  /// Child classes declare their properties here.
  virtual void initConcrete() = 0;
  /// Child classes implement the algorithm logic here.
  virtual void execConcrete() = 0;

  void afterPropertySet(const std::string &propName) override;
  void addWorkspace(const std::string &workspacePropertyName, bool addProperties = true);
  /// Read domain type property and cache the value
  void setDomainType();

protected:
  void setFunction();
  void setStepSizeMethod();
  void addWorkspaces();
  std::vector<std::string> getCostFunctionNames() const;
  void declareCostFunctionProperty();
  std::shared_ptr<CostFunctions::CostFuncFitting> getCostFunctionInitialized() const;

  /// Keep the domain type
  API::IDomainCreator::DomainType m_domainType{API::IDomainCreator::Simple};
  /// Pointer to the fitting function
  std::shared_ptr<API::IFunction> m_function;
  /// Pointer to a domain creator
  std::shared_ptr<API::IDomainCreator> m_domainCreator;
  std::vector<std::string> m_workspacePropertyNames;
  std::vector<std::string> m_workspaceIndexPropertyNames;

  friend class API::IDomainCreator;
};

} // namespace CurveFitting
} // namespace Mantid
