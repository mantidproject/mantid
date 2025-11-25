// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/IFittingAlgorithm.h"

#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/GeneralDomainCreator.h"
#include "MantidCurveFitting/HistogramDomainCreator.h"
#include "MantidCurveFitting/LatticeDomainCreator.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidCurveFitting/TableWorkspaceDomainCreator.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ILatticeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidKernel/ListValidator.h"

namespace Mantid::CurveFitting {

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
namespace {
/// Create a domain creator for a particular function and workspace pair.
std::unique_ptr<IDomainCreator> createDomainCreator(const IFunction *fun, const std::string &workspacePropertyName,
                                                    IPropertyManager *manager, IDomainCreator::DomainType domainType) {

  std::unique_ptr<IDomainCreator> creator;
  Workspace_sptr ws;

  try {
    ws = manager->getProperty("InputWorkspace");
  } catch (...) {
    // InputWorkspace is not needed for some fit function so continue
  }

  // ILatticeFunction requires API::LatticeDomain.
  if (dynamic_cast<const ILatticeFunction *>(fun)) {
    creator = std::make_unique<LatticeDomainCreator>(manager, workspacePropertyName);
  } else if (dynamic_cast<const IFunctionMD *>(fun)) {
    creator = std::unique_ptr<IDomainCreator>(
        API::DomainCreatorFactory::Instance().createDomainCreator("FitMD", manager, workspacePropertyName, domainType));
  } else if (dynamic_cast<const IFunction1DSpectrum *>(fun)) {
    creator = std::make_unique<SeqDomainSpectrumCreator>(manager, workspacePropertyName);
  } else if (auto gfun = dynamic_cast<const IFunctionGeneral *>(fun)) {
    creator = std::make_unique<GeneralDomainCreator>(*gfun, *manager, workspacePropertyName);
  } else if (std::dynamic_pointer_cast<ITableWorkspace>(ws)) {
    creator = std::make_unique<TableWorkspaceDomainCreator>(manager, workspacePropertyName, domainType);
  } else {
    bool histogramFit = manager->getPropertyValue("EvaluationType") == "Histogram";
    if (histogramFit) {
      creator = std::make_unique<HistogramDomainCreator>(*manager, workspacePropertyName);
    } else {
      creator = std::make_unique<FitMW>(manager, workspacePropertyName, domainType);
    }
  }
  return creator;
}
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithm's category for identification. @see Algorithm::category
const std::string IFittingAlgorithm::category() const { return "Optimization"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IFittingAlgorithm::init() {
  declareProperty(std::make_unique<API::FunctionProperty>("Function", Direction::InOut),
                  "Parameters defining the fitting function and its initial values");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "Name of the input Workspace");
  declareProperty("IgnoreInvalidData", false, "Flag to ignore infinities, NaNs and data with zero errors.");

  std::array<std::string, 3> domainTypes = {{"Simple", "Sequential", "Parallel"}};
  declareProperty("DomainType", "Simple", Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(domainTypes)),
                  "The type of function domain to use: Simple, Sequential, or Parallel.", Kernel::Direction::Input);

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty("EvaluationType", "CentrePoint",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(evaluationTypes)),
                  "The way the function is evaluated on histogram data sets. "
                  "If value is \"CentrePoint\" then function is evaluated at "
                  "centre of each bin. If it is \"Histogram\" then function is "
                  "integrated within the bin and the integrals returned.",
                  Kernel::Direction::Input);
  const std::array<std::string, 2> stepSizes = {{"Default", "Sqrt epsilon"}};
  declareProperty(
      "StepSizeMethod", "Default", Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(stepSizes)),
      "The way the step size is calculated for numerical derivatives. See the section about step sizes in the Fit "
      "algorithm documentation to understand the difference between \"Default\" and \"Sqrt epsilon\".",
      Kernel::Direction::Input);
  declareProperty("PeakRadius", 0,
                  "A value of the peak radius the peak functions should use. A "
                  "peak radius defines an interval on the x axis around the "
                  "centre of the peak where its values are calculated. Values "
                  "outside the interval are not calculated and assumed zeros."
                  "Numerically the radius is a whole number of peak widths "
                  "(FWHM) that fit into the interval on each side from the "
                  "centre. The default value of 0 means the whole x axis.");

  initConcrete();
}

/**
 * Examine "Function" and "InputWorkspace" properties to decide which domain
 * creator to use.
 * @param propName :: A property name.
 */
void IFittingAlgorithm::afterPropertySet(const std::string &propName) {
  if (propName == "Function") {
    setFunction();
  } else if (propName.size() >= 14 && propName.substr(0, 14) == "InputWorkspace") {
    if (getPointerToProperty("Function")->isDefault()) {
      throw std::invalid_argument("Function must be set before InputWorkspace");
    }
    addWorkspace(propName);
  } else if (propName == "DomainType") {
    setDomainType();
  } else if (propName == "StepSizeMethod") {
    setStepSizeMethod();
  }
}

/**
 * Read domain type property and cache the value
 */
void IFittingAlgorithm::setDomainType() {
  std::string domainType = getPropertyValue("DomainType");
  if (domainType == "Simple") {
    m_domainType = IDomainCreator::Simple;
  } else if (domainType == "Sequential") {
    m_domainType = IDomainCreator::Sequential;
  } else if (domainType == "Parallel") {
    m_domainType = IDomainCreator::Parallel;
  } else {
    m_domainType = IDomainCreator::Simple;
  }
}

void IFittingAlgorithm::setFunction() {
  // get the function
  m_function = getProperty("Function");
  size_t ndom = m_function->getNumberDomains();
  if (ndom > 1) {
    m_workspacePropertyNames.resize(ndom);
    m_workspaceIndexPropertyNames.resize(ndom);
    m_workspacePropertyNames[0] = "InputWorkspace";
    m_workspaceIndexPropertyNames[0] = "WorkspaceIndex";
    for (size_t i = 1; i < ndom; ++i) {
      std::string workspacePropertyName = "InputWorkspace_" + std::to_string(i);
      m_workspacePropertyNames[i] = workspacePropertyName;
      std::string workspaceIndexPropertyName = "WorkspaceIndex_" + std::to_string(i);
      m_workspaceIndexPropertyNames[i] = workspaceIndexPropertyName;
      if (!existsProperty(workspacePropertyName)) {
        declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>(workspacePropertyName, "",
                                                                                 Kernel::Direction::Input),
                        "Name of the input Workspace");
      }
    }
  } else {
    m_workspacePropertyNames.resize(1, "InputWorkspace");
    m_workspaceIndexPropertyNames.resize(1, "WorkspaceIndex");
  }
}

/**
 * Sets the method to use when calculating the step size for the numerical derivative.
 */
void IFittingAlgorithm::setStepSizeMethod() {
  if (m_function) {
    const std::string stepSizeMethod = getProperty("StepSizeMethod");
    m_function->setStepSizeMethod(stepSizeMethod == "Sqrt epsilon" ? IFunction::StepSizeMethod::SQRT_EPSILON
                                                                   : IFunction::StepSizeMethod::DEFAULT);
  }
}

/**
 * Add a new workspace to the fit. The workspace is in the property named
 * workspacePropertyName.
 * @param workspacePropertyName :: A workspace property name (eg InputWorkspace
 *   or InputWorkspace_2). The property must already exist in the algorithm.
 * @param addProperties :: allow for declaration of properties that specify the
 *   dataset within the workspace to fit to.
 */
void IFittingAlgorithm::addWorkspace(const std::string &workspacePropertyName, bool addProperties) {
  // m_function->setWorkspace(ws);
  const size_t n = std::string("InputWorkspace").size();
  const std::string suffix = (workspacePropertyName.size() > n) ? workspacePropertyName.substr(n) : "";
  const size_t index = suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));

  IFunction_sptr fun = getProperty("Function");
  setDomainType();

  auto creator = createDomainCreator(fun.get(), workspacePropertyName, this, m_domainType);

  if (!m_domainCreator) {
    if (m_workspacePropertyNames.empty()) {
      // this defines the function and fills in m_workspacePropertyNames with
      // names of the sort InputWorkspace_#
      setFunction();
    }
    if (fun->getNumberDomains() > 1) {
      auto multiCreator = std::make_shared<MultiDomainCreator>(this, m_workspacePropertyNames);
      creator->declareDatasetProperties(suffix, addProperties);
      multiCreator->setCreator(index, creator.release());
      m_domainCreator = multiCreator;
    } else {
      creator->declareDatasetProperties(suffix, addProperties);
      m_domainCreator.reset(creator.release());
    }
  } else {
    if (fun->getNumberDomains() > 1) {
      std::shared_ptr<MultiDomainCreator> multiCreator = std::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
      if (!multiCreator) {
        auto const &reference = *m_domainCreator;
        throw std::runtime_error(std::string("MultiDomainCreator expected, found ") + typeid(reference).name());
      }
      if (!multiCreator->hasCreator(index)) {
        creator->declareDatasetProperties(suffix, addProperties);
      }
      multiCreator->setCreator(index, creator.release());
    } else {
      creator->declareDatasetProperties(suffix, addProperties);
    }
  }
}

/**
 * Collect all input workspace property names in the m_workspacePropertyNames
 * vector.
 */
void IFittingAlgorithm::addWorkspaces() {
  setDomainType();
  if (m_function->getNumberDomains() > 1) {
    m_domainCreator.reset(new MultiDomainCreator(this, m_workspacePropertyNames));
  }
  auto props = getProperties();
  for (auto &prop : props) {
    if ((*prop).direction() == Kernel::Direction::Input && dynamic_cast<API::IWorkspaceProperty *>(prop)) {
      const std::string workspacePropertyName = (*prop).name();
      auto creator = createDomainCreator(m_function.get(), workspacePropertyName, this, m_domainType);

      const size_t n = std::string("InputWorkspace").size();
      const std::string suffix = (workspacePropertyName.size() > n) ? workspacePropertyName.substr(n) : "";
      const size_t index = suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));
      creator->declareDatasetProperties(suffix, false);
      if (!m_domainCreator) {
        m_domainCreator.reset(creator.release());
      }
      auto multiCreator = std::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
      if (multiCreator) {
        multiCreator->setCreator(index, creator.release());
      }
    }
  }

  // If domain creator wasn't created it's probably because
  // InputWorkspace property was deleted. Try without the workspace
  if (!m_domainCreator) {
    auto creator = createDomainCreator(m_function.get(), "", this, m_domainType);
    creator->declareDatasetProperties("", true);
    m_domainCreator.reset(creator.release());
    m_workspacePropertyNames.clear();
    m_workspaceIndexPropertyNames.clear();
  }
}

/// Return names of registered cost function for CostFuncFitting
/// dynamic type.
std::vector<std::string> IFittingAlgorithm::getCostFunctionNames() const {
  std::vector<std::string> out;
  auto &factory = CostFunctionFactory::Instance();
  auto names = factory.getKeys();
  out.reserve(names.size());
  for (auto &name : names) {
    if (std::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(factory.create(name))) {
      out.emplace_back(name);
    }
  }
  return out;
}

/// Declare a "CostFunction" property.
void IFittingAlgorithm::declareCostFunctionProperty() {
  Kernel::IValidator_sptr costFuncValidator =
      std::make_shared<Kernel::ListValidator<std::string>>(getCostFunctionNames());
  declareProperty("CostFunction", "Least squares", costFuncValidator,
                  "The cost function to be used for the fit, default is Least squares", Kernel::Direction::InOut);
}

/// Create a cost function from the "CostFunction" property
/// and make it ready for evaluation.
std::shared_ptr<CostFunctions::CostFuncFitting> IFittingAlgorithm::getCostFunctionInitialized() const {
  // Function may need some preparation.
  m_function->sortTies();
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  const bool ignoreInvalidData = getProperty("IgnoreInvalidData");
  m_domainCreator->ignoreInvalidData(ignoreInvalidData);
  m_domainCreator->createDomain(domain, values);

  // Set peak radius to the values which will be passed to
  // all IPeakFunctions
  int peakRadius = getProperty("PeakRadius");
  if (auto d1d = dynamic_cast<API::FunctionDomain1D *>(domain.get())) {
    if (peakRadius != 0) {
      d1d->setPeakRadius(peakRadius);
    }
  }

  // Do something with the function which may depend on workspace.
  m_domainCreator->initFunction(m_function);

  // get the cost function which must be a CostFuncFitting
  auto costFunction = std::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
      API::CostFunctionFactory::Instance().create(getPropertyValue("CostFunction")));

  costFunction->setIgnoreInvalidData(ignoreInvalidData);
  costFunction->setFittingFunction(m_function, domain, values);

  return costFunction;
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void IFittingAlgorithm::exec() {
  if (!m_domainCreator) {
    setFunction();
    addWorkspaces();
  }
  m_domainCreator->ignoreInvalidData(getProperty("IgnoreInvalidData"));
  // Execute the concrete algorithm.
  this->execConcrete();
}

} // namespace Mantid::CurveFitting
