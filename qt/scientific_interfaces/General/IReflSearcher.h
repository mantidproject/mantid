// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ISEARCHER_H
#define MANTID_CUSTOMINTERFACES_ISEARCHER_H

#include <string>

#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class ISearcher

ISearcher is an interface for search implementations used by IReflPresenter
implementations.
*/
class ISearcher {
public:
  virtual ~ISearcher(){};
  virtual Mantid::API::ITableWorkspace_sptr search(const std::string &text) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
