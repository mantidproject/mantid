#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/HistogramDomainCreator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace CurveFitting {

using namespace API;

/**
 * Constructor.
 * @param fun :: A function for which a domain is required.
 * @param manager :: Pointer to IPropertyManager (Algorithm) instance.
 * @param workspacePropertyName :: Name of the output property for a created
 * workspace in case a PropertyManager is used.
 */
HistogramDomainCreator::HistogramDomainCreator(
    const API::IFunctionGeneral &fun, Kernel::IPropertyManager &manager,
    const std::string &workspacePropertyName)
    : IDomainCreator(&manager,
                     std::vector<std::string>(1, workspacePropertyName)) {

}

/// Declare properties that specify the dataset within the workspace to fit
/// to.
/// @param suffix :: A suffix to give to all new properties.
/// @param addProp :: If false don't actually declare new properties but do
/// other stuff if needed
void HistogramDomainCreator::declareDatasetProperties(const std::string &suffix,
                                                    bool addProp) {
  UNUSED_ARG(suffix);
}

/// Retrive the input workspace from the property manager.
boost::shared_ptr<API::MatrixWorkspace>
HistogramDomainCreator::getInputWorkspace() const {
  return boost::shared_ptr<API::MatrixWorkspace>();
}

/**
 * Creates a domain corresponding to the assigned MatrixWorkspace
 *
 * @param domain :: Pointer to outgoing FunctionDomain instance.
 * @param values :: Pointer to outgoing FunctionValues object.
 * @param i0 :: Size offset for values object if it already contains data.
 */
void HistogramDomainCreator::createDomain(
    boost::shared_ptr<FunctionDomain> &domain,
    boost::shared_ptr<FunctionValues> &values, size_t i0) {

  // Create the values object
  if (!values) {
    values.reset(new FunctionValues);
  }

  // get the workspace
  auto tableWorkspace = getInputWorkspace();

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

Workspace_sptr HistogramDomainCreator::createOutputWorkspace(
    const std::string &baseName, IFunction_sptr function,
    boost::shared_ptr<FunctionDomain> domain,
    boost::shared_ptr<FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  if (function->getValuesSize(*domain) != values->size()) {
    throw std::runtime_error("Failed to create output workspace: domain and "
                             "values object don't match.");
  }

}

} // namespace CurveFitting
} // namespace Mantid
