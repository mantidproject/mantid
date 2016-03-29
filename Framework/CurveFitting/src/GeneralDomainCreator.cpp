#include "MantidCurveFitting/GeneralDomainCreator.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace CurveFitting {

using namespace API;

/**
 * Constructor.
 *
 * @param manager :: Pointer to IPropertyManager (Algorithm) instance.
 * @param workspacePropertyName :: Name of the output property for a created
 * workspace in case a PropertyManager is used.
 */
GeneralDomainCreator::GeneralDomainCreator(
    const API::IFunctionGeneral &fun, Kernel::IPropertyManager *manager,
    const std::string &workspacePropertyName)
    : IDomainCreator(manager,
                     std::vector<std::string>(1, workspacePropertyName)) {
  if (manager == nullptr) {
    throw std::invalid_argument(
        "Property manager cannot be null for GeneralDomainCreator.");
  }

  m_defaultValuesSize = fun.getDefaultValuesSize();

  auto nDomainColumns = fun.getNumberDomainColumns();
  if (nDomainColumns > 0) {
    m_domainColumnNames.push_back("ArgumentColumn");
    for(size_t i = 1; i < nDomainColumns; ++i) {
      m_domainColumnNames.push_back(m_domainColumnNames.front() +
                                    boost::lexical_cast<std::string>(i));
    }
  }

  auto nDataColumns = fun.getNumberValuesPerArgument();
  if (nDataColumns > 0) {
    m_dataColumnNames.push_back("DataColumn");
    m_weightsColumnNames.push_back("WeightsColumn");
    for(size_t i = 1; i < nDataColumns; ++i) {
      auto si = boost::lexical_cast<std::string>(i);
      m_dataColumnNames.push_back(m_dataColumnNames.front() + si);
      m_weightsColumnNames.push_back(m_weightsColumnNames.front() + si);
    }
  }
}

/// Declare properties that specify the dataset within the workspace to fit
/// to.
/// @param suffix :: A suffix to give to all new properties.
/// @param addProp :: If false don't actually declare new properties but do
/// other stuff if needed
void GeneralDomainCreator::declareDatasetProperties(const std::string &suffix,
                                                    bool addProp) {
  for(auto &propName : m_domainColumnNames) {
    declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""),
                    "A name of a domain column.");
  }
  for(auto &propName : m_dataColumnNames) {
    declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""),
                    "A name of a fitting data column.");
  }
  for(auto &propName : m_weightsColumnNames) {
    declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""),
                    "A name of a fitting weights column.");
  }
}

/**
 * Creates a domain corresponding to the assigned MatrixWorkspace
 *
 * @param domain :: Pointer to outgoing FunctionDomain instance.
 * @param values :: Pointer to outgoing FunctionValues object.
 * @param i0 :: Size offset for values object if it already contains data.
 */
void GeneralDomainCreator::createDomain(
    boost::shared_ptr<FunctionDomain> &domain,
    boost::shared_ptr<FunctionValues> &values, size_t i0) {

  // get the workspace
  auto workspacePropertyName = m_workspacePropertyNames.front();
  API::Workspace_sptr ws = m_manager->getProperty(workspacePropertyName);
  auto tableWorkspace = boost::dynamic_pointer_cast<API::ITableWorkspace>(ws);
  if (!tableWorkspace) {
    throw std::invalid_argument("InputWorkspace must be a TableWorkspace.");
  }

  // Create the domain
  domain.reset(new FunctionDomainGeneral);
  auto &generalDomain = *static_cast<FunctionDomainGeneral*>(domain.get());
  for(auto &propName : m_domainColumnNames) {
    std::string columnName = m_manager->getPropertyValue(propName);
    auto column = tableWorkspace->getColumn(columnName);
    generalDomain.addColumn(column);
  }
  auto domainSize = domain->size();

  // Get the fitting data
  if (!values) {
    values.reset(new FunctionValues);
  }
  auto nDataColumns = m_dataColumnNames.size();
  for(size_t i = 0; i < nDataColumns; ++i) {
    std::string columnName = m_manager->getPropertyValue(m_dataColumnNames[i]);
    auto dataColumn = tableWorkspace->getColumn(columnName);
    columnName = m_manager->getPropertyValue(m_weightsColumnNames[i]);
    auto weightsColumn = tableWorkspace->getColumn(columnName);
    values->expand(i0 + domainSize);
    for(size_t j = 0; j < domainSize; ++j) {
      values->setFitData(i0 + j, dataColumn->toDouble(j));
      values->setFitWeight(i0 + j, weightsColumn->toDouble(j));
    }
  }
}

/**
 * Creates an output workspace using the given function and domain
 *
 * @param baseName :: Basename for output workspace.
 * @param function :: Function that can handle a FunctionDomain1D-domain.
 * @param domain :: Pointer to FunctionDomain instance.
 * @param values :: Pointer to FunctionValues instance, currently not used.
 * @param outputWorkspacePropertyName :: Name of output workspace property, if
 * used.
 * @return Workspace with calculated values.
 */

Workspace_sptr GeneralDomainCreator::createOutputWorkspace(
    const std::string &baseName, IFunction_sptr function,
    boost::shared_ptr<FunctionDomain> domain,
    boost::shared_ptr<FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  // don't need values, since the values need to be calculated spectrum by
  // spectrum (see loop below).
  UNUSED_ARG(values);
  return Workspace_sptr();
}

/**
 * Returns the domain size. Throws if no MatrixWorkspace has been set.
 *
 * @return Total domain size.
 */
size_t GeneralDomainCreator::getDomainSize() const {
  return 0;
}

/// Add data and weights to a FunctionValues object
void GeneralDomainCreator::addDataAndWeights(API::FunctionValues &values,
                                             const API::Column &dataColumn,
                                             const API::Column *weightsColumn) {
}

} // namespace CurveFitting
} // namespace Mantid
