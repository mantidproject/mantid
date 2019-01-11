// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_IFITTINGALGORITHM_H_
#define MANTID_CURVEFITTING_IFITTINGALGORITHM_H_

#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidKernel/System.h"

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
class DLLExport IFittingAlgorithm : public API::ParallelAlgorithm {
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
  void addWorkspace(const std::string &workspacePropertyName,
                    bool addProperties = true);
  /// Read domain type property and cache the value
  void setDomainType();

protected:
  void setFunction();
  void addWorkspaces();
  std::vector<std::string> getCostFunctionNames() const;
  void declareCostFunctionProperty();
  boost::shared_ptr<CostFunctions::CostFuncFitting>
  getCostFunctionInitialized() const;

  /// Keep the domain type
  API::IDomainCreator::DomainType m_domainType{API::IDomainCreator::Simple};
  /// Pointer to the fitting function
  boost::shared_ptr<API::IFunction> m_function;
  /// Pointer to a domain creator
  boost::shared_ptr<API::IDomainCreator> m_domainCreator;
  std::vector<std::string> m_workspacePropertyNames;

  friend class API::IDomainCreator;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_IFITTINGALGORITHM_H_ */
