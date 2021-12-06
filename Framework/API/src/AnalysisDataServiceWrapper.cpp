// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceWrapper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DllConfig.h"

// Explicit template types
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <memory>

namespace {
struct Service {
  Mantid::API::AnalysisDataServiceWrapper ads;
};
} // namespace

namespace Mantid::API {
AnalysisDataServiceWrapper &getDefaultAnalysisDataService() noexcept {
  // Lazy init to replace singleton
  // Long-term we can manually scope this out and inject it by-pointer, see
  // https://youtu.be/4DfvSixpdCw?t=4040
  static Service serv;
  return serv.ads;
}

AnalysisDataServiceWrapper::AnalysisDataServiceWrapper() : m_ads(AnalysisDataServiceConstructorKey()) {}

// ----- DataService forwarders
bool AnalysisDataServiceWrapper::doesExist(const std::string &name) const { return m_ads.doesExist(name); }

void AnalysisDataServiceWrapper::clear() { m_ads.clear(); }

size_t AnalysisDataServiceWrapper::size() const { return m_ads.size(); }

std::shared_ptr<API::Workspace> AnalysisDataServiceWrapper::retrieve(const std::string &name) const {
  return m_ads.retrieve(name);
}

Poco::NotificationCenter &AnalysisDataServiceWrapper::getNotificationCenter() { return m_ads.notificationCenter; }

std::vector<std::string> AnalysisDataServiceWrapper::getObjectNames(Mantid::Kernel::DataServiceSort sortState,
                                                                    Mantid::Kernel::DataServiceHidden hiddenState,
                                                                    const std::string &contain) const {
  return m_ads.getObjectNames(sortState, hiddenState, contain);
}

std::vector<std::shared_ptr<Mantid::API::Workspace>>
AnalysisDataServiceWrapper::getObjects(Mantid::Kernel::DataServiceHidden includeHidden) const {
  return m_ads.getObjects(includeHidden);
}

bool AnalysisDataServiceWrapper::isHiddenDataServiceObject(const std::string &name) {
  return m_ads.isHiddenDataServiceObject(name);
}

// ------ AnalysisDataService forwarders

const std::string &AnalysisDataServiceWrapper::illegalCharacters() const { return m_ads.illegalCharacters(); }

void AnalysisDataServiceWrapper::setIllegalCharacterList(const std::string &chars) {
  m_ads.setIllegalCharacterList(chars);
}

const std::string AnalysisDataServiceWrapper::isValid(const std::string &name) const { return m_ads.isValid(name); }

void AnalysisDataServiceWrapper::add(const std::string &name, const std::shared_ptr<API::Workspace> &workspace) {
  m_ads.add(name, workspace);
}

void AnalysisDataServiceWrapper::addOrReplace(const std::string &name,
                                              const std::shared_ptr<API::Workspace> &workspace) {
  m_ads.addOrReplace(name, workspace);
}

void AnalysisDataServiceWrapper::rename(const std::string &oldName, const std::string &newName) {
  m_ads.rename(oldName, newName);
}

void AnalysisDataServiceWrapper::remove(const std::string &name) { m_ads.remove(name); }

std::vector<Workspace_sptr> AnalysisDataServiceWrapper::retrieveWorkspaces(const std::vector<std::string> &names,
                                                                           bool unrollGroups) const {
  return m_ads.retrieveWorkspaces(names, unrollGroups);
}

void AnalysisDataServiceWrapper::sortGroupByName(const std::string &groupName) { m_ads.sortGroupByName(groupName); }

void AnalysisDataServiceWrapper::addToGroup(const std::string &groupName, const std::string &wsName) {
  m_ads.addToGroup(groupName, wsName);
}

void AnalysisDataServiceWrapper::deepRemoveGroup(const std::string &name) { m_ads.deepRemoveGroup(name); }

void AnalysisDataServiceWrapper::removeFromGroup(const std::string &groupName, const std::string &wsName) {
  m_ads.removeFromGroup(groupName, wsName);
}

std::map<std::string, Workspace_sptr> AnalysisDataServiceWrapper::topLevelItems() const {
  return m_ads.topLevelItems();
}

} // namespace Mantid::API
