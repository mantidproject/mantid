// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <QString>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModel {

public:
  virtual ~ImageInfoModel() = default;

  /** Creates a list with information about the coordinates in the workspace.
  @param x: x data coordinate
  @param y: y data coordinate, for a matrix workspace this will be the spectrum
  number
  @param signal: the signal value at x,y
  @return a vector containing pairs of strings
  */
  virtual std::vector<QString> getInfoList(const double x, const double y,
                                           const double signal) = 0;

  virtual void setWorkspace(const Mantid::API::Workspace_sptr &ws) = 0;

protected:
  void addNameAndValue(const std::string &label, std::vector<QString> &list,
                       const double value, const int precision,
                       bool includeValue = true,
                       const Mantid::Kernel::Unit_sptr &units = nullptr);
};

} // namespace MantidWidgets
} // namespace MantidQt
