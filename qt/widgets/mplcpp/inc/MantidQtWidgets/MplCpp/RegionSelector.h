// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include <QLayout>

namespace MantidQt::Widgets::MplCpp {
class MANTID_MPLCPP_DLL RegionSelector : public Common::Python::InstanceHolder {
public:
  RegionSelector(Mantid::API::Workspace_sptr const &workspace, QLayout *layout);
  void updateWorkspace(Mantid::API::Workspace_sptr const &workspace);

private:
  Common::Python::Object getView() const;
  void show() const;

  QLayout *m_layout;
};
} // namespace MantidQt::Widgets::MplCpp
