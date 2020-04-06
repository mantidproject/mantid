// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "SearchResult.h"
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class ISearchModel

Provides an additional interface to the Qt model for the search results table
in the Runs view. This interface is used by the presenter to access and
manipulate the view's model.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ISearchModel {
public:
  virtual void mergeNewResults(std::vector<SearchResult> const &source) = 0;
  virtual SearchResult const &getRowData(int index) const = 0;
  virtual void clear() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
