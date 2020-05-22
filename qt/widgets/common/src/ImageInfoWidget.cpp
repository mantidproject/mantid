// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoWidget.h"
#include "MantidAPI/Workspace_fwd.h"

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
    : IImageInfoWidget(parent),
      m_presenter(std::make_unique<ImageInfoPresenter>(this)) {

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
#endif
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  horizontalHeader()->hide();
  verticalHeader()->hide();

  m_presenter->createImageInfoModel(workspace);
  updateTable(0, 0, 0, false);
}

void ImageInfoWidget::updateTable(const double x, const double y,
                                  const double z, bool includeValues) {
  auto info = m_presenter->getInfoList(x, y, z, includeValues);

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
} // namespace MantidWidgets

} // namespace MantidQt
