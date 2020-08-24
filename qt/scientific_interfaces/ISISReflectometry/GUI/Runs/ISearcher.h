// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

#include "GUI/Runs/IRunsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class SearcherSubscriber {
public:
  virtual void notifySearchComplete() = 0;
  virtual void notifySearchFailed() = 0;
};

/** @class ISearcher

ISearcher is an interface for search implementations used by IRunsPresenter
implementations.
*/
class ISearcher {
public:
  enum class SearchType { NONE, MANUAL, AUTO };
  virtual ~ISearcher(){};
  virtual void subscribe(SearcherSubscriber *notifyee) = 0;
  virtual std::vector<SearchResult> search(const std::string &text,
                                           const std::string &instrument,
                                           const std::string &cycle,
                                           SearchType searchType) = 0;
  virtual bool startSearchAsync(const std::string &text,
                                const std::string &instrument,
                                const std::string &cycle,
                                SearchType searchType) = 0;
  virtual bool searchInProgress() const = 0;
  virtual SearchResult const &getSearchResult(int index) const = 0;
  virtual void reset() = 0;
  virtual bool searchSettingsChanged(const std::string &text,
                                     const std::string &instrument,
                                     const std::string &cycle,
                                     SearchType searchType) const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
