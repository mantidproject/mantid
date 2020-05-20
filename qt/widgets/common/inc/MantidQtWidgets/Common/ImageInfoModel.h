// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/Workspace.h"
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModel {

public:
  // Creates a list containing pairs of strings with information about the
  // coordinates in the workspace.
  virtual std::vector<std::string> getInfoList(const double x, const double y,
                                               const double z) = 0;

protected:
  void addNameAndValue(const std::string &label, const double value,
                       const int precision, std::vector<std::string> &list);
};

} // namespace MantidWidgets
} // namespace MantidQt
