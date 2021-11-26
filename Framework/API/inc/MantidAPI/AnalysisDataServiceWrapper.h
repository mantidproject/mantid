// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/DataService.h"

#include <memory>
#include <string>
#include <vector>

namespace Mantid::API {
class AnalysisDataServiceImpl;

/**
 * Wraps the AnalysisDataService implementation in a testable, constructable way.
 * Users can create the wrapper to use their own ADS instance, or call
 * getDefaultAnalysisDataService for singleton-like behaviour if required.
 *
 * Additionally, all method are marked virtual so that they are mockable
 */
class MANTID_API_DLL AnalysisDataServiceWrapper {
public:
  AnalysisDataServiceWrapper();
  // From DataService
  virtual bool doesExist(const std::string &name) const;
  virtual void clear();
  virtual size_t size() const;
  virtual std::shared_ptr<API::Workspace> retrieve(const std::string &name) const;
  virtual Poco::NotificationCenter &getNotificationCenter();
  virtual std::vector<std::string>
  getObjectNames(Mantid::Kernel::DataServiceSort sortState = Mantid::Kernel::DataServiceSort::Unsorted,
                 Mantid::Kernel::DataServiceHidden hiddenState = Mantid::Kernel::DataServiceHidden::Auto,
                 const std::string &contain = "") const;
  virtual std::vector<std::shared_ptr<Mantid::API::Workspace>>
  getObjects(Mantid::Kernel::DataServiceHidden includeHidden = Mantid::Kernel::DataServiceHidden::Auto) const;
  virtual bool isHiddenDataServiceObject(const std::string &name);

  // From AnalysisDataServiceImpl

  virtual const std::string &illegalCharacters() const;
  virtual void setIllegalCharacterList(const std::string &chars);
  virtual const std::string isValid(const std::string &name) const;
  virtual void add(const std::string &name, const std::shared_ptr<API::Workspace> &workspace);
  virtual void addOrReplace(const std::string &name, const std::shared_ptr<API::Workspace> &workspace);
  virtual void rename(const std::string &oldName, const std::string &newName);
  virtual void remove(const std::string &name);
  template <typename WSTYPE> std::shared_ptr<WSTYPE> retrieveWS(const std::string &name) const {
    return m_ads.retrieveWS<WSTYPE>(name);
  }
  virtual std::vector<Workspace_sptr> retrieveWorkspaces(const std::vector<std::string> &names,
                                                         bool unrollGroups = false) const;
  virtual void sortGroupByName(const std::string &groupName);
  virtual void addToGroup(const std::string &groupName, const std::string &wsName);
  virtual void deepRemoveGroup(const std::string &name);
  virtual void removeFromGroup(const std::string &groupName, const std::string &wsName);
  virtual std::map<std::string, Workspace_sptr> topLevelItems() const;

private:
  // Break circular deps, ideally AnalysisDataService::Instance should be defined here
  // but to maintain backwards compat we need that in the other header causing the circle
  Mantid::API::AnalysisDataServiceImpl m_ads;
};

MANTID_API_DLL AnalysisDataServiceWrapper &getDefaultAnalysisDataService() noexcept;

struct AnalysisDataServiceConstructorKey {
  friend class AnalysisDataServiceWrapper;

private:
  AnalysisDataServiceConstructorKey() = default;
};

} // namespace Mantid::API
