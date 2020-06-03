// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoPresenter.h"

#include <QTableWidget>

namespace MantidQt {
namespace MantidWidgets {

/**
 * A table widget containing information about the pixel the mouse is over in
 * an image
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoWidget : public IImageInfoWidget {
  Q_OBJECT

public:
  ImageInfoWidget(const std::string &workspace_type, QWidget *parent = nullptr);

  void updateTable(const double x = DBL_MAX, const double y = DBL_MAX,
                   const double z = DBL_MAX) override;

  void setWorkspace(const Mantid::API::Workspace_sptr &ws) override;

private:
  std::unique_ptr<ImageInfoPresenter> m_presenter;
};

} // namespace MantidWidgets
} // namespace MantidQt
