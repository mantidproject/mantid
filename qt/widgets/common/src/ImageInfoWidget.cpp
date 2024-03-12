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

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param parent A QWidget to act as the parent widget
 */
ImageInfoWidget::ImageInfoWidget(QWidget *parent)
    : QTableWidget(0, 0, parent), m_presenter(std::make_unique<ImageInfoPresenter>(this)) {
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  horizontalHeader()->hide();
  verticalHeader()->hide();
}

/**
 * @param x X position if the cursor
 * @param y Y position if the cursor
 * @param signal Signal value at cursor position
 * @param extraValues A map of extra column names and values to display
 */
void ImageInfoWidget::cursorAt(const double x, const double y, const double signal,
                               const QMap<QString, QString> &extraValues) {
  m_presenter->cursorAt(x, y, signal, extraValues);
}

/**
 * Display the information provided within the table cells
 * @param info A reference to a collection of header/value pairs
 */
void ImageInfoWidget::showInfo(const ImageInfoModel::ImageInfo &info) {
  m_presenter->fillTableCells(info);
  horizontalHeader()->setMinimumSectionSize(50);
  resizeColumnsToContents();
}

/**
 * Set the workspace to probe for information
 * @param ws A pointer to a Workspace object
 */
void ImageInfoWidget::setWorkspace(const Mantid::API::Workspace_sptr &ws) { m_presenter->setWorkspace(ws); }

void ImageInfoWidget::setRowCount(const int count) { QTableWidget::setRowCount(count); }

void ImageInfoWidget::setColumnCount(const int count) { QTableWidget::setColumnCount(count); }

void ImageInfoWidget::setItem(const int rowIndex, const int columnIndex, QTableWidgetItem *item) {
  QTableWidget::setItem(rowIndex, columnIndex, item);
}

void ImageInfoWidget::hideColumn(const int index) { QTableWidget::hideColumn(index); }

void ImageInfoWidget::showColumn(const int index) { QTableWidget::showColumn(index); }

// Set presenter flag for wether to show or hide the signal column
void ImageInfoWidget::setShowSignal(const bool showSignal) { m_presenter->setShowSignal(showSignal); }

} // namespace MantidQt::MantidWidgets
