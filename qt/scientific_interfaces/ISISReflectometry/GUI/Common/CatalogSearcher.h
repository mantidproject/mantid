// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H

#include "ISearcher.h"

namespace MantidQt {
namespace CustomInterfaces {

class IMainWindowView;

/** @class CatalogSearcher

CatalogSearcher implements ISearcher to provide ICAT search
functionality.
*/
class CatalogSearcher : public ISearcher {
public:
  CatalogSearcher(IMainWindowView *view);
  ~CatalogSearcher() override{};
  Mantid::API::ITableWorkspace_sptr search(const std::string &text) override;

private:
  IMainWindowView *m_view;

  bool hasActiveSession() const;
  void logInToCatalog();
  std::string activeSessionId() const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
