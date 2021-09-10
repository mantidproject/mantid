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
 * @param parent A QWidget to act as the parent widget
 */
ImageInfoWidget::ImageInfoWidget(QWidget *parent)
    : IImageInfoWidget(parent), m_presenter(std::make_unique<ImageInfoPresenter>(this)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
#endif
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  horizontalHeader()->hide();
  verticalHeader()->hide();
}

/**
 * @param x X position if the cursor
 * @param y Y position if the cursor
 * @param signal Signal value at cursor position
 */
void ImageInfoWidget::cursorAt(const double x, const double y, const double signal) {
  m_presenter->cursorAt(x, y, signal);
}

/**
 * Display the information provided within the table cells
 * @param info A reference to a collection of header/value pairs
 */
void ImageInfoWidget::showInfo(const ImageInfoModel::ImageInfo &info) {
  if (info.empty())
    return;

  const auto itemCount(info.size());
  setColumnCount(itemCount);
  for (int i = 0; i < itemCount; ++i) {
    auto header = new QTableWidgetItem(info.name(i));
    header->setFlags(header->flags() & ~Qt::ItemIsEditable);
    setItem(0, i, header);
    auto value = new QTableWidgetItem(info.value(i));
    value->setFlags(header->flags() & ~Qt::ItemIsEditable);
    setItem(1, i, value);
  }
  horizontalHeader()->setMinimumSectionSize(50);
  resizeColumnsToContents();
}

/**
 * Set the workspace to probe for information
 * @param ws A pointer to a Workspace object
 */
void ImageInfoWidget::setWorkspace(const Mantid::API::Workspace_sptr &ws) { m_presenter->setWorkspace(ws); }
} // namespace MantidWidgets
} // namespace MantidQt
