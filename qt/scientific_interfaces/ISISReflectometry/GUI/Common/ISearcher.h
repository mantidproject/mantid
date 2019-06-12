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
  virtual void
  notifySearchResults(Mantid::API::ITableWorkspace_sptr results) = 0;
};

/** @class ISearcher

ISearcher is an interface for search implementations used by
IRunsPresenter implementations.
*/
class ISearcher {
public:
  virtual ~ISearcher(){};
  virtual void subscribe(SearcherSubscriber *notifyee) = 0;
  virtual Mantid::API::ITableWorkspace_sptr search(const std::string &text) = 0;
  virtual void startSearchAsync(const std::string &text) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
