//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <sstream>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {

namespace {
Kernel::Logger g_log("MultiDomainCreator");
}

void MultiDomainCreator::setCreator(size_t i, IDomainCreator *creator) {
  m_creators[i] = boost::shared_ptr<IDomainCreator>(creator);
}

/**
 * Check if i-th creator has been set with setCreator
 * @param i :: Index of a creator, 0 < i < getNCreators()
 */
bool MultiDomainCreator::hasCreator(size_t i) const {
  return static_cast<bool>(m_creators[i]);
}

/// Create a domain from the input workspace
void MultiDomainCreator::createDomain(
    boost::shared_ptr<API::FunctionDomain> &domain,
    boost::shared_ptr<API::FunctionValues> &ivalues, size_t i0) {
  if (m_workspacePropertyNames.size() != m_creators.size()) {
    throw std::runtime_error(
        "Cannot create JointDomain: number of workspaces does not match "
        "the number of creators");
  }
  auto jointDomain = new API::JointDomain;
  API::FunctionValues_sptr values;
  i0 = 0;
  for (auto c = m_creators.begin(); c != m_creators.end(); ++c) {
    if (!(*c)) {
      throw std::runtime_error("Missing domain creator");
    }
    API::FunctionDomain_sptr domain;
    (**c).createDomain(domain, values, i0);
    jointDomain->addDomain(domain);
    i0 += domain->size();
  }
  domain.reset(jointDomain);
  ivalues = values;
}

/**
 * Initialize the function with the workspace(s).
 * @param function :: A function to initialize.
 */
void MultiDomainCreator::initFunction(API::IFunction_sptr function) {
  auto mdFunction =
      boost::dynamic_pointer_cast<API::MultiDomainFunction>(function);
  if (mdFunction) {
    // loop over member functions and init them
    for (size_t iFun = 0; iFun < mdFunction->nFunctions(); ++iFun) {
      std::vector<size_t> domainIndices;
      // get domain indices for this function
      mdFunction->getDomainIndices(iFun, m_creators.size(), domainIndices);
      if (!domainIndices.empty()) {
        /*
        if ( domainIndices.size() != 1 )
        {
          std::stringstream msg;
          msg << "Function #" << iFun << " applies to multiple domains.\n"
              << "Only one of the domains is used to set workspace.";
          g_log.warning(msg.str());
        }
        */
        size_t index = domainIndices[0];
        if (index >= m_creators.size()) {
          std::stringstream msg;
          msg << "Domain index is out of range. (Function #" << iFun << ")";
          throw std::runtime_error(msg.str());
        }
        m_creators[index]->initFunction(mdFunction->getFunction(iFun));
      } else {
        g_log.warning() << "Function #" << iFun
                        << " doesn't apply to any domain" << std::endl;
      }
    }
  } else {
    IDomainCreator::initFunction(function);
  }
}

/**
 * Create the output workspace group.
 * @param baseName :: The base name for the output workspace. Suffix "Workspace"
 * will be added to it.
 * @param function :: A function to calculate the values. Must be of the
 * MultiDomainFunction type.
 * @param domain :: Domain created earlier with this creator (unused)
 * @param values :: Values created earlier with this creator (unused)
 * @param outputWorkspacePropertyName :: Name for the property to hold the
 * output workspace. If
 *    empty the property won't be created.
 * @return A shared pointer to the created workspace.
 */
boost::shared_ptr<API::Workspace> MultiDomainCreator::createOutputWorkspace(
    const std::string &baseName, API::IFunction_sptr function,
    boost::shared_ptr<API::FunctionDomain> domain,
    boost::shared_ptr<API::FunctionValues> values,
    const std::string &outputWorkspacePropertyName) {
  UNUSED_ARG(domain);
  UNUSED_ARG(values);

  auto mdFunction =
      boost::dynamic_pointer_cast<API::MultiDomainFunction>(function);
  if (!mdFunction) {
    throw std::runtime_error("A MultiDomainFunction is expected to be used "
                             "with MultiDomainCreator.");
  }

  // split the function into independent parts
  std::vector<API::IFunction_sptr> functions =
      mdFunction->createEquivalentFunctions();
  // there must be as many parts as there are domains
  if (functions.size() != m_creators.size()) {
    throw std::runtime_error("Number of functions and domains don't match");
  }

  API::WorkspaceGroup_sptr outWS =
      API::WorkspaceGroup_sptr(new API::WorkspaceGroup());

  for (size_t i = 0; i < functions.size(); ++i) {
    std::string localName =
        baseName + "Workspace_" + boost::lexical_cast<std::string>(i);
    auto fun = functions[i];
    auto creator = m_creators[i];
    boost::shared_ptr<API::FunctionDomain> localDomain;
    boost::shared_ptr<API::FunctionValues> localValues;
    fun->setUpForFit();
    creator->createDomain(localDomain, localValues);
    creator->initFunction(fun);
    auto ws = creator->createOutputWorkspace(localName, fun, localDomain,
                                             localValues, "");
    API::AnalysisDataService::Instance().addOrReplace(localName, ws);
    outWS->addWorkspace(ws);
  }

  if (!outputWorkspacePropertyName.empty()) {
    declareProperty(
        new API::WorkspaceProperty<API::WorkspaceGroup>(
            outputWorkspacePropertyName, "", Kernel::Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspaces");
    m_manager->setProperty(outputWorkspacePropertyName, outWS);
  }

  return outWS;
}

} // namespace Algorithm
} // namespace Mantid
