// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoWidgetMini.h"
#include "MantidAPI/Workspace_fwd.h"

#include <QAbstractScrollArea>
#include <QHeaderView>

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param parent A QWidget to act as the parent widget
 */
ImageInfoWidgetMini::ImageInfoWidgetMini(QWidget *parent)
    : QLabel(parent), m_presenter(std::make_unique<ImageInfoPresenter>(this)) {}

/**
 * @param x X position if the cursor
 * @param y Y position if the cursor
 * @param signal Signal value at cursor position
 * @param extraValues A map of extra column names and values to display
 */
void ImageInfoWidgetMini::cursorAt(const double x, const double y, const double signal,
                                   const QMap<QString, QString> &extraValues) {
  m_presenter->cursorAt(x, y, signal, extraValues);
}

/**
 * Display the information provided within the table cells
 * @param info A reference to a collection of header/value pairs
 */
void ImageInfoWidgetMini::showInfo(const ImageInfoModel::ImageInfo &info) {
  if (info.empty())
    return;
  auto text = QString{};

  if (!info.value(0).isEmpty() && info.value(0) != "-") {
    text += info.name(0) + "=" + info.value(0) + ", "; // TOF
    text += info.name(1) + "=" + info.value(1) + ", "; // Spectrum
    text += info.name(2) + "=" + info.value(2);        // Signal
  }

  setText(text);
}

/**
 * Set the workspace to probe for information
 * @param ws A pointer to a Workspace object
 */
void ImageInfoWidgetMini::setWorkspace(const Mantid::API::Workspace_sptr &ws) { m_presenter->setWorkspace(ws); }

void ImageInfoWidgetMini::setRowCount(const int /*count*/) {}
void ImageInfoWidgetMini::setColumnCount(const int /*count*/) {}
void ImageInfoWidgetMini::setItem(const int /*rowIndex*/, const int /*columnIndex*/, QTableWidgetItem * /*item*/) {}
void ImageInfoWidgetMini::hideColumn(const int /*index*/) {}
void ImageInfoWidgetMini::showColumn(const int /*index*/) {}

} // namespace MantidQt::MantidWidgets
