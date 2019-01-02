// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGSEARCHER_H

#include "IReflSearcher.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class CatalogSearcher

CatalogSearcher implements IReflSearcher to provide ICAT search
functionality.
*/
class CatalogSearcher : public IReflSearcher {
public:
  ~CatalogSearcher() override{};
  Mantid::API::ITableWorkspace_sptr search(const std::string &text) override;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
