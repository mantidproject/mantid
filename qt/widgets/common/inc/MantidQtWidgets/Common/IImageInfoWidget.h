// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/ImageInfoModel.h"
#include <QTableWidget>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON IImageInfoWidget : public QTableWidget {
  Q_OBJECT
public:
  IImageInfoWidget(QWidget *parent = nullptr) : QTableWidget(0, 0, parent) {}

  virtual void cursorAt(const double x, const double y, const double signal) = 0;
  virtual void showInfo(const ImageInfoModel::ImageInfo &info) = 0;
  virtual void setWorkspace(const Mantid::API::Workspace_sptr &ws) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
