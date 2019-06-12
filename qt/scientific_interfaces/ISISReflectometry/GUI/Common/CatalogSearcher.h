// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H

#include "GUI/Runs/IRunsView.h"
#include "IPythonRunner.h"
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
  Mantid::API::ITableWorkspace_sptr search(const std::string &text) override;
  bool startSearchAsync(const std::string &text) override;

  // RunsViewSearchSubscriber overrides
  void notifySearchComplete() override;

private:
  IPythonRunner *m_pythonRunner;
  IRunsView *m_view;
  SearcherSubscriber *m_notifyee;

  bool hasActiveSession() const;
  bool logInToCatalog();
  std::string activeSessionId() const;
  Mantid::API::IAlgorithm_sptr createSearchAlgorithm(const std::string &text);
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
