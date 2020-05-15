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
  ImageInfoModelMD(const Mantid::API::IMDWorkspace_sptr &ws);

  // Creates a list containing pairs of strings with information about the
  // coordinates in the workspace.
  std::vector<std::string> getInfoList(const double x, const double y,
                                       const double z);

  Mantid::API::IMDWorkspace_sptr m_workspace;
};

} // namespace MantidWidgets
} // namespace MantidQt
