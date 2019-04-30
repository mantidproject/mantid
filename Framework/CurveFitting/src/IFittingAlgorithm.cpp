// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/IFittingAlgorithm.h"

#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/GeneralDomainCreator.h"
#include "MantidCurveFitting/HistogramDomainCreator.h"
#include "MantidCurveFitting/LatticeDomainCreator.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ILatticeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace CurveFitting {

using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
namespace {
/// Create a domain creator for a particular function and workspace pair.
IDomainCreator *createDomainCreator(const IFunction *fun,
                                    const std::string &workspacePropertyName,
                                    IPropertyManager *manager,
                                    IDomainCreator::DomainType domainType) {

  IDomainCreator *creator = nullptr;

  // ILatticeFunction requires API::LatticeDomain.
  if (dynamic_cast<const ILatticeFunction *>(fun)) {
    creator = new LatticeDomainCreator(manager, workspacePropertyName);
  } else if (dynamic_cast<const IFunctionMD *>(fun)) {
    creator = API::DomainCreatorFactory::Instance().createDomainCreator(
        "FitMD", manager, workspacePropertyName, domainType);
  } else if (dynamic_cast<const IFunction1DSpectrum *>(fun)) {
    creator = new SeqDomainSpectrumCreator(manager, workspacePropertyName);
  } else if (auto gfun = dynamic_cast<const IFunctionGeneral *>(fun)) {
    creator = new GeneralDomainCreator(*gfun, *manager, workspacePropertyName);
  } else {
    bool histogramFit =
        manager->getPropertyValue("EvaluationType") == "Histogram";
    if (histogramFit) {
      creator = new HistogramDomainCreator(*manager, workspacePropertyName);
    } else {
      creator = new FitMW(manager, workspacePropertyName, domainType);
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
  declareProperty(
      make_unique<API::FunctionProperty>("Function", Direction::InOut),
      "Parameters defining the fitting function and its initial values");

  declareProperty(make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "InputWorkspace", "", Kernel::Direction::Input),
                  "Name of the input Workspace");
  declareProperty("IgnoreInvalidData", false,
                  "Flag to ignore infinities, NaNs and data with zero errors.");

  std::array<std::string, 3> domainTypes = {
      {"Simple", "Sequential", "Parallel"}};
  declareProperty(
      "DomainType", "Simple",
      Kernel::IValidator_sptr(
          new Kernel::ListValidator<std::string>(domainTypes)),
      "The type of function domain to use: Simple, Sequential, or Parallel.",
      Kernel::Direction::Input);

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty("EvaluationType", "CentrePoint",
                  Kernel::IValidator_sptr(
                      new Kernel::ListValidator<std::string>(evaluationTypes)),
                  "The way the function is evaluated on histogram data sets. "
                  "If value is \"CentrePoint\" then function is evaluated at "
                  "centre of each bin. If it is \"Histogram\" then function is "
                  "integrated within the bin and the integrals returned.",
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
  } else if (propName.size() >= 14 &&
             propName.substr(0, 14) == "InputWorkspace") {
    if (getPointerToProperty("Function")->isDefault()) {
      throw std::invalid_argument("Function must be set before InputWorkspace");
    }
    addWorkspace(propName);
  } else if (propName == "DomainType") {
    setDomainType();
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
    m_workspacePropertyNames[0] = "InputWorkspace";
    for (size_t i = 1; i < ndom; ++i) {
      std::string workspacePropertyName = "InputWorkspace_" + std::to_string(i);
      m_workspacePropertyNames[i] = workspacePropertyName;
      if (!existsProperty(workspacePropertyName)) {
        declareProperty(
            Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
                workspacePropertyName, "", Kernel::Direction::Input),
            "Name of the input Workspace");
      }
    }
  } else {
    m_workspacePropertyNames.resize(1, "InputWorkspace");
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
void IFittingAlgorithm::addWorkspace(const std::string &workspacePropertyName,
                                     bool addProperties) {
  // m_function->setWorkspace(ws);
  const size_t n = std::string("InputWorkspace").size();
  const std::string suffix =
      (workspacePropertyName.size() > n) ? workspacePropertyName.substr(n) : "";
  const size_t index =
      suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));

  IFunction_sptr fun = getProperty("Function");
  setDomainType();

  IDomainCreator *creator =
      createDomainCreator(fun.get(), workspacePropertyName, this, m_domainType);

  if (!m_domainCreator) {
    if (m_workspacePropertyNames.empty()) {
      // this defines the function and fills in m_workspacePropertyNames with
      // names of the sort InputWorkspace_#
      setFunction();
    }
    if (fun->getNumberDomains() > 1) {
      auto multiCreator =
          new MultiDomainCreator(this, m_workspacePropertyNames);
      multiCreator->setCreator(index, creator);
      m_domainCreator.reset(multiCreator);
      creator->declareDatasetProperties(suffix, addProperties);
    } else {
      m_domainCreator.reset(creator);
      creator->declareDatasetProperties(suffix, addProperties);
    }
  } else {
    if (fun->getNumberDomains() > 1) {
      boost::shared_ptr<MultiDomainCreator> multiCreator =
          boost::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
      if (!multiCreator) {
        auto &reference = *m_domainCreator;
        throw std::runtime_error(
            std::string("MultiDomainCreator expected, found ") +
            typeid(reference).name());
      }
      if (!multiCreator->hasCreator(index)) {
        creator->declareDatasetProperties(suffix, addProperties);
      }
      multiCreator->setCreator(index, creator);
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
    m_domainCreator.reset(
        new MultiDomainCreator(this, m_workspacePropertyNames));
  }
  auto props = getProperties();
  for (auto &prop : props) {
    if ((*prop).direction() == Kernel::Direction::Input &&
        dynamic_cast<API::IWorkspaceProperty *>(prop)) {
      const std::string workspacePropertyName = (*prop).name();
      IDomainCreator *creator = createDomainCreator(
          m_function.get(), workspacePropertyName, this, m_domainType);

      const size_t n = std::string("InputWorkspace").size();
      const std::string suffix = (workspacePropertyName.size() > n)
                                     ? workspacePropertyName.substr(n)
                                     : "";
      const size_t index =
          suffix.empty() ? 0 : boost::lexical_cast<size_t>(suffix.substr(1));
      creator->declareDatasetProperties(suffix, false);
      if (!m_domainCreator) {
        m_domainCreator.reset(creator);
      }
      auto multiCreator =
          boost::dynamic_pointer_cast<MultiDomainCreator>(m_domainCreator);
      if (multiCreator) {
        multiCreator->setCreator(index, creator);
      }
    }
  }

  // If domain creator wasn't created it's probably because
  // InputWorkspace property was deleted. Try without the workspace
  if (!m_domainCreator) {
    IDomainCreator *creator =
        createDomainCreator(m_function.get(), "", this, m_domainType);
    creator->declareDatasetProperties("", true);
    m_domainCreator.reset(creator);
    m_workspacePropertyNames.clear();
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
    if (boost::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
            factory.create(name))) {
      out.push_back(name);
    }
  }
  return out;
}

/// Declare a "CostFunction" property.
void IFittingAlgorithm::declareCostFunctionProperty() {
  Kernel::IValidator_sptr costFuncValidator =
      boost::make_shared<Kernel::ListValidator<std::string>>(
          getCostFunctionNames());
  declareProperty(
      "CostFunction", "Least squares", costFuncValidator,
      "The cost function to be used for the fit, default is Least squares",
      Kernel::Direction::InOut);
}

/// Create a cost function from the "CostFunction" property
/// and make it ready for evaluation.
boost::shared_ptr<CostFunctions::CostFuncFitting>
IFittingAlgorithm::getCostFunctionInitialized() const {
  // Function may need some preparation.
  m_function->sortTies();
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
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
  auto costFunction =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
          API::CostFunctionFactory::Instance().create(
              getPropertyValue("CostFunction")));

  costFunction->setFittingFunction(m_function, domain, values);

  return costFunction;
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void IFittingAlgorithm::exec() {

  // This is to make it work with AlgorithmProxy
  if (!m_domainCreator) {
    setFunction();
    addWorkspaces();
  }
  m_domainCreator->ignoreInvalidData(getProperty("IgnoreInvalidData"));
  // Execute the concrete algorithm.
  this->execConcrete();
}

} // namespace CurveFitting
} // namespace Mantid
