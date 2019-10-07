// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_model.h"
#include "ALFView_presenter.h"

// will need these later
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include "ALFView.h"

namespace MantidQt {
namespace CustomInterfaces {

// DECLARE_SUBWINDOW(ALFView)

/// static logger
Mantid::Kernel::Logger g_log("ALFView");

ALFView::ALFView(QWidget *parent)
    : UserSubWindow(parent), m_view(nullptr), m_presenter(nullptr) {
  m_model = new ALFView_model();
  m_view = new ALFView_view(m_model->getInstrument(), this);
  m_presenter = new ALFView_presenter(m_view, m_model);
}

void ALFView::initLayout() {
  this->setCentralWidget(m_view);
  m_presenter->initLayout();
}
} // namespace CustomInterfaces
} // namespace MantidQt
