// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include <QLabel>
#include <QMap>

namespace MantidQt::MantidWidgets {

/**
 * A table widget containing information about the pixel the mouse is over in
 * an image
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoWidgetMini : public QLabel, public IImageInfoWidget {
  Q_OBJECT

public:
  ImageInfoWidgetMini(QWidget *parent = nullptr);

  // Note: QMap has sip binding via PyQt but only for specific types (both types have to be classes or the first type
  // has to be int)
  void cursorAt(const double x, const double y, const double signal,
                const QMap<QString, QString> &extraValues) override;
  void setWorkspace(const Mantid::API::Workspace_sptr &ws) override;
  void showInfo(const ImageInfoModel::ImageInfo &info) override;
  void setRowCount(const int count) override;
  void setColumnCount(const int count) override;
  void setItem(const int rowIndex, const int columnIndex, QTableWidgetItem *item) override;
  void hideColumn(const int index) override;
  void showColumn(const int index) override;

private:
  std::unique_ptr<ImageInfoPresenter> m_presenter;
};

} // namespace MantidQt::MantidWidgets
