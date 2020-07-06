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
ImageInfoWidget::ImageInfoWidget(const std::string &workspace_type,
                                 QWidget *parent)
    : IImageInfoWidget(parent),
      m_presenter(std::make_unique<ImageInfoPresenter>(this)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
#endif
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  horizontalHeader()->hide();
  verticalHeader()->hide();

  m_presenter->createImageInfoModel(workspace_type);
  updateTable();
}

void ImageInfoWidget::updateTable(const double x, const double y,
                                  const double z) {
  auto info = m_presenter->getInfoList(x, y, z);

  if (info.empty())
    return;

  setColumnCount(static_cast<int>(info.size() / 2));
  setRowCount(2);

  auto row = 0;
  auto column = 0;
  for (const auto &item : info) {
    auto cell = new QTableWidgetItem(item);
    setItem(row, column, cell);
    cell->setFlags(Qt::ItemIsSelectable);
    row++;
    if (row == 2) {
      row = 0;
      column++;
    }
  }
  horizontalHeader()->setMinimumSectionSize(50);
  resizeColumnsToContents();
}
void ImageInfoWidget::setWorkspace(const Mantid::API::Workspace_sptr &ws) {
  m_presenter->setWorkspace(ws);
  updateTable();
}
} // namespace MantidWidgets
} // namespace MantidQt
