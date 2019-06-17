// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H

#include "GUI/Common/IPythonRunner.h"
#include "GUI/Runs/IRunsView.h"
#include "ISearcher.h"
#include "MantidAPI/IAlgorithm_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

class IMainWindowView;

/** @class CatalogSearcher

CatalogSearcher implements ISearcher to provide ICAT search
functionality.
*/
class CatalogSearcher : public ISearcher, public RunsViewSearchSubscriber {
public:
  CatalogSearcher(IPythonRunner *pythonRunner, IRunsView *m_view);
  ~CatalogSearcher() override{};

  // ISearcher overrides
  void subscribe(SearcherSubscriber *notifyee) override;
  Mantid::API::ITableWorkspace_sptr
  search(const std::string &text, const std::string &instrument) override;
  bool startSearchAsync(const std::string &text,
                        const std::string &instrument) override;
  bool searchInProgress() const override;
  SearchResult const &getSearchResult(int index) const override;
  void setSearchResultError(int index,
                            const std::string &errorMessage) override;
  void resetResults() override;
  bool searchSettingsChanged(const std::string &text,
                             const std::string &instrument) const override;

  // RunsViewSearchSubscriber overrides
  void notifySearchComplete() override;

private:
  IPythonRunner *m_pythonRunner;
  IRunsView *m_view;
  SearcherSubscriber *m_notifyee;
  std::string m_searchText;
  std::string m_instrument;
  bool m_searchInProgress;

  bool hasActiveSession() const;
  bool logInToCatalog();
  std::string activeSessionId() const;
  Mantid::API::IAlgorithm_sptr createSearchAlgorithm(const std::string &text);
  ISearchModel &results() const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
