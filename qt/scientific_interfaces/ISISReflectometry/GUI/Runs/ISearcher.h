// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLSEARCHER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSEARCHER_H

#include <string>

#include "GUI/Runs/IRunsPresenter.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

class SearcherSubscriber {
public:
  virtual void notifySearchComplete() = 0;
  virtual void notifySearchFailed() = 0;
};

/** @class ISearcher

ISearcher is an interface for search implementations used by
IRunsPresenter implementations.
*/
class ISearcher : public QObject {
public:
  enum class SearchType { NONE, MANUAL, AUTO };
  virtual ~ISearcher(){};
  virtual void subscribe(SearcherSubscriber *notifyee) = 0;
  virtual Mantid::API::ITableWorkspace_sptr
  search(const std::string &text, const std::string &instrument,
         SearchType searchType) = 0;
  virtual bool startSearchAsync(const std::string &text,
                                const std::string &instrument,
                                SearchType searchType) = 0;
  virtual bool searchInProgress() const = 0;
  virtual SearchResult const &getSearchResult(int index) const = 0;
  virtual void setSearchResultError(int index,
                                    const std::string &errorMessage) = 0;
  virtual void reset() = 0;
  virtual bool searchSettingsChanged(const std::string &text,
                                     const std::string &instrument,
                                     SearchType searchType) const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
