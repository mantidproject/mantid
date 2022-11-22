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
#include <QMap>

namespace MantidQt {
namespace MantidWidgets {

/**
 * A table widget containing information about the pixel the mouse is over in
 * an image
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoWidget : public IImageInfoWidget {
  Q_OBJECT

public:
  ImageInfoWidget(QWidget *parent = nullptr);

  // Note: QMap has sip binding via PyQt but only for specific types (both types have to be classes or the first type
  // has to be int)
  void cursorAt(const double x, const double y, const double signal,
                const QMap<QString, QString> &extraValues) override;
  void setWorkspace(const Mantid::API::Workspace_sptr &ws) override;
  void showInfo(const ImageInfoModel::ImageInfo &info) override;

private:
  std::unique_ptr<ImageInfoPresenter> m_presenter;
};

} // namespace MantidWidgets
} // namespace MantidQt
