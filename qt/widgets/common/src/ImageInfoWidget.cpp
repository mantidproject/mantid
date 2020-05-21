// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoWidget.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"

#include <QAbstractScrollArea>
#include <QHeaderView>
#include <QTableWidget>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 */
ImageInfoWidget::ImageInfoWidget(const Mantid::API::Workspace_sptr &workspace,
                                 QWidget *parent)
    : QTableWidget(0, 0, parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
#endif
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  createImageInfoModel(workspace);
  horizontalHeader()->hide();
  verticalHeader()->hide();

  updateTable(0, 0, 0, false);
}

void ImageInfoWidget::updateTable(const double x, const double y,
                                  const double z, bool includeValues) {
  auto info = m_model->getInfoList(x, y, z, includeValues);

  if (info.empty())
    return;

  setColumnCount(static_cast<int>(info.size() / 2));
  setRowCount(2);

  auto row = 0;
  auto column = 0;
  for (const auto &item : info) {
    auto cell = new QTableWidgetItem(QString::fromStdString(item));
    setItem(row, column, cell);
    cell->setFlags(Qt::ItemIsSelectable);
    row++;
    if (row == 2) {
      row = 0;
      column++;
    }
  }
  resizeColumnsToContents();
}

void ImageInfoWidget::createImageInfoModel(
    const Mantid::API::Workspace_sptr &ws) {
  if (auto matWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws))
    m_model = std::make_unique<ImageInfoModelMatrixWS>(matWS);
  else if (auto MDWS =
               std::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(ws)) {
    m_model = std::make_unique<ImageInfoModelMD>(MDWS);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
