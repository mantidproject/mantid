// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/DisplayType.h"
#include "MantidQtWidgets/Common/ImageInfoModel.h"

#include <QTableWidget>

namespace MantidQt {
namespace MantidWidgets {

/**
 * A table widget containing information about the pixel the mouse is over in
 * an image
 */
class EXPORT_OPT_MANTIDQT_COMMON ImageInfoWidget : public QTableWidget {
  Q_OBJECT

public:
  ImageInfoWidget(std::string &wsName, DisplayType *type = nullptr,
                  QWidget *parent = nullptr);
  ~ImageInfoWidget() override;

  void updateTable(const double x, const double y, const double z);

private:
  std::unique_ptr<ImageInfoModel> m_model;
};

} // namespace MantidWidgets
} // namespace MantidQt
