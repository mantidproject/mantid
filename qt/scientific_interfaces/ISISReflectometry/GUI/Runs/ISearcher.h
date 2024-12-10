// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Runs/SearchCriteria.h"

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
  virtual ~ISearcher() {};
  virtual void subscribe(SearcherSubscriber *notifyee) = 0;
  virtual std::vector<SearchResult> search(SearchCriteria searchCriteria) = 0;
  virtual bool startSearchAsync(SearchCriteria searchCriteria) = 0;
  virtual bool searchInProgress() const = 0;
  virtual SearchResult const &getSearchResult(int index) const = 0;
  virtual void reset() = 0;
  virtual bool hasUnsavedChanges() const = 0;
  virtual void setSaved() = 0;
  virtual SearchCriteria searchCriteria() const = 0;
  virtual std::string getSearchResultsCSV() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
