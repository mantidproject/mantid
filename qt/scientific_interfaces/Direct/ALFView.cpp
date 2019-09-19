// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_model.h"

// will need these later
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"


#include "ALFView.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid;


DECLARE_SUBWINDOW(ALFView)

/// static logger
Mantid::Kernel::Logger g_log("ALFView");

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_view(nullptr),m_presenter(nullptr) {

  m_view = new ALFView_view(this);
  m_model = new ALFView_model();
  m_presenter = new ALFView_presenter(m_view, m_model);
}

void ALFView::initLayout() {
  this->setCentralWidget(m_view);
  m_presenter->initLayout();
}
} // namespace CustomInterfaces
} // namespace MantidQt