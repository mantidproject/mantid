// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <utility>

#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/Workspace.h"

namespace Mantid::API {

/**
 * @param manager :: A property manager which has information about the data
 * source (eg workspace)
 * and the function.
 * @param workspacePropertyNames :: Property names for workspaces to get the
 * data from.
 * @param domainType :: Type of domain to create: Simple, Sequential, or
 * Parallel.
 */
IDomainCreator::IDomainCreator(Kernel::IPropertyManager *manager, std::vector<std::string> workspacePropertyNames,
                               DomainType domainType)
    : m_manager(manager), m_workspacePropertyNames(std::move(workspacePropertyNames)), m_domainType(domainType),
      m_outputCompositeMembers(false), m_convolutionCompositeMembers(false), m_ignoreInvalidData(false) {}

/**
 * @param value If true then each composite is unrolled and its output is
 * appended to
 * the default output, otherwise just the composite is used
 * @param conv If true and the fitting function is Convolution and its model
 * (function with
 * index 1) is composite then output the components of the model convolved with
 * the
 * resolution (function index 0).
 */
void IDomainCreator::separateCompositeMembersInOutput(const bool value, const bool conv) {
  m_outputCompositeMembers = value;
  m_convolutionCompositeMembers = conv;
}

/**
 * Declare a property to the algorithm.
 * @param prop :: A new property.
 * @param doc :: A doc string.
 */
void IDomainCreator::declareProperty(Kernel::Property *prop, const std::string &doc) {
  if (!m_manager) {
    throw std::runtime_error("IDomainCreator: property manager isn't defined.");
  }
  m_manager->declareProperty(std::unique_ptr<Kernel::Property>(prop), doc);
}

/**
 * Initialize the function with the workspace. Default is to call
 * IFunction::setWorkspace().
 */
void IDomainCreator::initFunction(API::IFunction_sptr function) {
  if (!function) {
    throw std::runtime_error("IDomainCreator: cannot initialize empty function.");
  }
  if (!m_manager) {
    throw std::runtime_error("IDomainCreator: property manager isn't defined.");
  }
  if (m_manager->existsProperty("InputWorkspace")) {
    API::Workspace_sptr workspace = m_manager->getProperty("InputWorkspace");
    if (!workspace) {
      throw std::runtime_error("IDomainCreator: cannot initialize function: workspace undefined.");
    }
    function->setWorkspace(workspace);
  }
}

} // namespace Mantid::API
