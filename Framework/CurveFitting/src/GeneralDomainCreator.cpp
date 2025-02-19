// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/GeneralDomainCreator.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid::CurveFitting {

using namespace API;

/**
 * Constructor.
 * @param fun :: A function for which a domain is required.
 * @param manager :: Pointer to IPropertyManager (Algorithm) instance.
 * @param workspacePropertyName :: Name of the output property for a created
 * workspace in case a PropertyManager is used.
 */
GeneralDomainCreator::GeneralDomainCreator(const API::IFunctionGeneral &fun, Kernel::IPropertyManager &manager,
                                           const std::string &workspacePropertyName)
    : IDomainCreator(&manager, std::vector<std::string>(1, workspacePropertyName)) {

  m_defaultValuesSize = fun.getDefaultDomainSize();

  auto nDomainColumns = fun.getNumberDomainColumns();
  if (nDomainColumns > 0) {
    m_domainColumnNames.emplace_back("ArgumentColumn");
    for (size_t i = 1; i < nDomainColumns; ++i) {
      m_domainColumnNames.emplace_back(m_domainColumnNames.front() + "_" + std::to_string(i));
    }
  }

  auto nDataColumns = fun.getNumberValuesPerArgument();
  if (nDataColumns > 0) {
    m_dataColumnNames.emplace_back("DataColumn");
    m_weightsColumnNames.emplace_back("WeightsColumn");
    for (size_t i = 1; i < nDataColumns; ++i) {
      auto si = "_" + std::to_string(i);
      m_dataColumnNames.emplace_back(m_dataColumnNames.front() + si);
      m_weightsColumnNames.emplace_back(m_weightsColumnNames.front() + si);
    }
  }
}

/// Declare properties that specify the dataset within the workspace to fit
/// to.
/// @param suffix :: A suffix to give to all new properties.
/// @param addProp :: If false don't actually declare new properties but do
/// other stuff if needed
void GeneralDomainCreator::declareDatasetProperties(const std::string &suffix, bool addProp) {
  UNUSED_ARG(suffix);
  if (addProp) {
    for (const auto &propName : m_domainColumnNames) {
      declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""), "A name of a domain column.");
    }
    for (const auto &propName : m_dataColumnNames) {
      declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""), "A name of a fitting data column.");
    }
    for (const auto &propName : m_weightsColumnNames) {
      declareProperty(new Kernel::PropertyWithValue<std::string>(propName, ""), "A name of a fitting weights column.");
    }
  }
}

/// Retrive the input workspace from the property manager.
std::shared_ptr<API::ITableWorkspace> GeneralDomainCreator::getInputWorkspace() const {
  auto workspacePropertyName = m_workspacePropertyNames.front();
  if (!m_manager->existsProperty(workspacePropertyName)) {
    return API::ITableWorkspace_sptr();
  }
  API::Workspace_sptr ws = m_manager->getProperty(workspacePropertyName);
  auto tableWorkspace = std::dynamic_pointer_cast<API::ITableWorkspace>(ws);
  if (!tableWorkspace) {
    throw std::invalid_argument("InputWorkspace must be a TableWorkspace.");
  }
  return tableWorkspace;
}

/**
 * Creates a domain corresponding to the assigned MatrixWorkspace
 *
 * @param domain :: Pointer to outgoing FunctionDomain instance.
 * @param values :: Pointer to outgoing FunctionValues object.
 * @param i0 :: Size offset for values object if it already contains data.
 */
void GeneralDomainCreator::createDomain(std::shared_ptr<FunctionDomain> &domain,
                                        std::shared_ptr<FunctionValues> &values, size_t i0) {

  // Create the values object
  if (!values) {
    values.reset(new FunctionValues);
  }

  // get the workspace
  auto tableWorkspace = getInputWorkspace();

  size_t domainSize = 0;
  domain.reset(new FunctionDomainGeneral);
  // Create the domain
  if (!m_domainColumnNames.empty() && tableWorkspace) {
    auto &generalDomain = *static_cast<FunctionDomainGeneral *>(domain.get());
    for (auto &propName : m_domainColumnNames) {
      std::string columnName = m_manager->getPropertyValue(propName);
      auto column = tableWorkspace->getColumn(columnName);
      generalDomain.addColumn(column);
    }
    domainSize = domain->size();
  } else {
    domainSize = m_defaultValuesSize;
  }

  // No workspace - no data
  if (!tableWorkspace) {
    return;
  }

  if (domainSize == 0) {
    domainSize = tableWorkspace->rowCount();
  }

  // If domain size is 0 - there are no fitting data
  if (domainSize == 0) {
    return;
  }

  // Get the fitting data
  auto nDataColumns = m_dataColumnNames.size();
  // Append each column to values' fitting data
  for (size_t i = 0; i < nDataColumns; ++i) {
    // Set the data
    std::string columnName = m_manager->getPropertyValue(m_dataColumnNames[i]);
    auto dataColumn = tableWorkspace->getColumn(columnName);
    values->expand(i0 + domainSize);
    for (size_t j = 0; j < domainSize; ++j) {
      values->setFitData(i0 + j, dataColumn->toDouble(j));
    }
    // Set the weights
    columnName = m_manager->getPropertyValue(m_weightsColumnNames[i]);
    if (!columnName.empty()) {
      auto weightsColumn = tableWorkspace->getColumn(columnName);
      for (size_t j = 0; j < domainSize; ++j) {
        values->setFitWeight(i0 + j, weightsColumn->toDouble(j));
      }
    } else {
      for (size_t j = 0; j < domainSize; ++j) {
        values->setFitWeight(i0 + j, 1.0);
      }
    }
    i0 += domainSize;
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

Workspace_sptr GeneralDomainCreator::createOutputWorkspace(const std::string &baseName, IFunction_sptr function,
                                                           std::shared_ptr<FunctionDomain> domain,
                                                           std::shared_ptr<FunctionValues> values,
                                                           const std::string &outputWorkspacePropertyName) {
  if (function->getValuesSize(*domain) != values->size()) {
    throw std::runtime_error("Failed to create output workspace: domain and "
                             "values object don't match.");
  }
  size_t rowCount = domain->size();
  if (rowCount == 0) {
    auto &generalFunction = dynamic_cast<IFunctionGeneral &>(*function);
    rowCount = generalFunction.getDefaultDomainSize();
  }

  ITableWorkspace_sptr outputWorkspace;

  auto inputWorkspace = getInputWorkspace();
  // Clone the data and domain columns from inputWorkspace to outputWorkspace.
  if (inputWorkspace) {
    // Collect the names of columns to clone
    std::vector<std::string> columnsToClone;
    for (const auto &propName : m_domainColumnNames) {
      auto columnName = m_manager->getPropertyValue(propName);
      columnsToClone.emplace_back(columnName);
    }
    for (const auto &propName : m_dataColumnNames) {
      auto columnName = m_manager->getPropertyValue(propName);
      columnsToClone.emplace_back(columnName);
    }
    outputWorkspace = inputWorkspace->cloneColumns(columnsToClone);
    if (rowCount != outputWorkspace->rowCount()) {
      throw std::runtime_error("Cloned workspace has wrong number of rows.");
    }

    // Add columns with the calculated data
    size_t i0 = 0;
    for (const auto &propName : m_dataColumnNames) {
      auto dataColumnName = m_manager->getPropertyValue(propName);
      auto calcColumnName = dataColumnName + "_calc";
      auto column = outputWorkspace->addColumn("double", calcColumnName);
      for (size_t row = 0; row < rowCount; ++row) {
        auto value = values->getCalculated(i0 + row);
        column->fromDouble(row, value);
      }
      i0 += rowCount;
    }
  } else {
    outputWorkspace = API::WorkspaceFactory::Instance().createTable();
    outputWorkspace->setRowCount(rowCount);
    size_t i0 = 0;
    for (const auto &propName : m_dataColumnNames) {
      auto calcColumnName = m_manager->getPropertyValue(propName);
      if (calcColumnName.empty()) {
        calcColumnName = propName;
      }
      auto column = outputWorkspace->addColumn("double", calcColumnName);
      for (size_t row = 0; row < rowCount; ++row) {
        auto value = values->getCalculated(i0 + row);
        column->fromDouble(row, value);
      }
      i0 += rowCount;
    }
  }

  if (!outputWorkspacePropertyName.empty()) {
    declareProperty(
        new API::WorkspaceProperty<API::ITableWorkspace>(outputWorkspacePropertyName, "", Kernel::Direction::Output),
        "Name of the output Workspace holding resulting simulated values");
    m_manager->setPropertyValue(outputWorkspacePropertyName, baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, outputWorkspace);
  }

  return outputWorkspace;
}

/**
 * Returns the domain size. Throws if no MatrixWorkspace has been set.
 *
 * @return Total domain size.
 */
size_t GeneralDomainCreator::getDomainSize() const {
  size_t domainSize = 0;
  if (!m_domainColumnNames.empty()) {
    auto inputWorkspace = getInputWorkspace();
    domainSize = inputWorkspace->rowCount();
  } else {
    domainSize = m_defaultValuesSize;
  }
  return domainSize;
}

} // namespace Mantid::CurveFitting
