// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"

#include <QTableWidget>

using Mantid::API::MatrixWorkspace;

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param view A pointer to a view object that displays the information
 */
ImageInfoPresenter::ImageInfoPresenter(IImageInfoWidget *view) : m_model(), m_view(view), m_showSignal(true) {
  m_view->setRowCount(2);
}

/**
 * @param x X position on an image of the workspace
 * @param y Y position on an image of the workspace
 * @param signal The signal value at the given position
 * @param extraValues A map of extra column names and values to display
 */
void ImageInfoPresenter::cursorAt(const double x, const double y, const double signal,
                                  const QMap<QString, QString> extraValues) {
  assert(m_model);
  m_view->showInfo(m_model->info(x, y, signal, extraValues));
}

/**
 * Set a new workspace for the view to display
 * @param workspace A pointer to a new workspace
 */
void ImageInfoPresenter::setWorkspace(const Mantid::API::Workspace_sptr &workspace) {
  if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(workspace))
    m_model = std::make_unique<ImageInfoModelMatrixWS>(matrixWS);
  else {
    m_model = std::make_unique<ImageInfoModelMD>();
  }
}

/**
 * Fill the view table cells using the model
 * @param info A reference to a collection of header/value pairs
 */
void ImageInfoPresenter::fillTableCells(const ImageInfoModel::ImageInfo &info) {
  if (info.empty())
    return;

  const auto &itemCount(info.size());
  m_view->setColumnCount(itemCount);
  for (int i = 0; i < itemCount; ++i) {
    const auto &name = info.name(i);
    if (name == "Signal") {
      if (m_showSignal) {
        m_view->showColumn(i);
      } else {
        m_view->hideColumn(i);
      }
    }
    auto header = new QTableWidgetItem(name);
    header->setFlags(header->flags() & ~Qt::ItemIsEditable);
    m_view->setItem(0, i, header);
    auto value = new QTableWidgetItem(info.value(i));
    value->setFlags(header->flags() & ~Qt::ItemIsEditable);
    m_view->setItem(1, i, value);
  }
}

} // namespace MantidQt::MantidWidgets
