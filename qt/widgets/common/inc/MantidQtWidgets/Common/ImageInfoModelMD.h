// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtWidgets/Common/ImageInfoModel.h"

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModelMD : public ImageInfoModel {

public:
  /** Creates a list with information about the coordinates in the workspace.
  @param x: x data coordinate
  @param y: y data coordinate
  @param signal: the signal value at x, y
  @return a vector containing pairs of strings
  */
  std::vector<QString> getInfoList(const double x, const double y,
                                   const double signal) override;

  void setWorkspace(const Mantid::API::Workspace_sptr &ws) override {
    UNUSED_ARG(ws);
  }
};

} // namespace MantidWidgets
} // namespace MantidQt
