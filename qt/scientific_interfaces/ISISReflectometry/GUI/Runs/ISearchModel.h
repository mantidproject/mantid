// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_ISEARCHMODEL_H_
#define MANTID_ISISREFLECTOMETRY_ISEARCHMODEL_H_

#include "Common/DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "SearchResult.h"
#include <map>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL ISearchModel {
public:
  virtual void
  addDataFromTable(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                   const std::string &instrument) = 0;
  virtual SearchResult const &getRowData(int index) const = 0;
  virtual void setError(int index, std::string const &error) = 0;
  virtual void clear() = 0;
};

/// Typedef for a shared pointer to \c SearchModel
using ISearchModel_sptr = boost::shared_ptr<ISearchModel>;
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_ISEARCHMODEL_H_ */
