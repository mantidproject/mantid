// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_IREFLSEARCHER_H
#define MANTID_CUSTOMINTERFACES_IREFLSEARCHER_H

#include <string>

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class IReflSearcher

IReflSearcher is an interface for search implementations used by IReflPresenter
implementations.
*/
class IReflSearcher {
public:
  virtual ~IReflSearcher(){};
  virtual Mantid::API::ITableWorkspace_sptr search(const std::string &text) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
