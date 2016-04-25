#include "MantidCurveFitting/IFittingAlgorithm.h"

#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/GeneralDomainCreator.h"
#include "MantidCurveFitting/LatticeDomainCreator.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"

#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ILatticeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"

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
    creator = new FitMW(manager, workspacePropertyName, domainType);
  }
  return creator;
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IFittingAlgorithm::IFittingAlgorithm()
    : API::Algorithm(), m_domainType(API::IDomainCreator::Simple) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IFittingAlgorithm::~IFittingAlgorithm() {}

/// Algorithm's category for identification. @see Algorithm::category
const std::string IFittingAlgorithm::category() const { return "Optimization"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IFittingAlgorithm::init() {
  declareProperty(
      make_unique<API::FunctionProperty>("Function"),
      "Parameters defining the fitting function and its initial values");

  declareProperty(make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "InputWorkspace", "", Kernel::Direction::Input),
                  "Name of the input Workspace");
  declareProperty("IgnoreInvalidData", false,
                  "Flag to ignore infinities, NaNs and data with zero errors.");

  std::vector<std::string> domainTypes{"Simple", "Sequential", "Parallel"};
  declareProperty(
      "DomainType", "Simple",
      Kernel::IValidator_sptr(
          new Kernel::ListValidator<std::string>(domainTypes)),
      "The type of function domain to use: Simple, Sequential, or Parallel.",
      Kernel::Direction::Input);

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
  // Kernel::Property *prop = getPointerToProperty("Minimizer");
  // auto minimizerProperty =
  //    dynamic_cast<Kernel::PropertyWithValue<std::string> *>(prop);
  // std::vector<std::string> minimizerOptions =
  //    API::FuncMinimizerFactory::Instance().getKeys();
  // if (m_domainType != IDomainCreator::Simple) {
  //  auto it = std::find(minimizerOptions.begin(), minimizerOptions.end(),
  //                      "Levenberg-Marquardt");
  //  minimizerOptions.erase(it);
  //}
  // minimizerProperty->replaceValidator(Kernel::IValidator_sptr(
  //    new Kernel::StartsWithValidator(minimizerOptions)));
}

void IFittingAlgorithm::setFunction() {
  // get the function
  m_function = getProperty("Function");
  auto mdf = boost::dynamic_pointer_cast<API::MultiDomainFunction>(m_function);
  if (mdf) {
    size_t ndom = mdf->getMaxIndex() + 1;
    m_workspacePropertyNames.resize(ndom);
    m_workspacePropertyNames[0] = "InputWorkspace";
    for (size_t i = 1; i < ndom; ++i) {
      std::string workspacePropertyName =
          "InputWorkspace_" + boost::lexical_cast<std::string>(i);
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
  // get the workspace
  API::Workspace_const_sptr ws = getProperty(workspacePropertyName);
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
    auto multiFun = boost::dynamic_pointer_cast<API::MultiDomainFunction>(fun);
    if (multiFun) {
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
  }
}

/**
 * Collect all input workspace property names in the m_workspacePropertyNames
 * vector.
 */
void IFittingAlgorithm::addWorkspaces() {
  setDomainType();
  auto multiFun =
      boost::dynamic_pointer_cast<API::MultiDomainFunction>(m_function);
  if (multiFun) {
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
      m_workspacePropertyNames.push_back(workspacePropertyName);
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
    creator->declareDatasetProperties("", false);
    m_domainCreator.reset(creator);
    m_workspacePropertyNames.clear();
  }
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
