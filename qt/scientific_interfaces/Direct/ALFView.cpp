// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView.h"
#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentPresenter.h"

#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(ALFView)

/// static logger
Mantid::Kernel::Logger g_log("ALFView");

ALFView::ALFView(QWidget *parent)
    : UserSubWindow(parent), m_view(nullptr), m_presenter(nullptr),
      m_analysisPane(nullptr) {
  m_model = new ALFCustomInstrumentModel();
  m_view = new ALFCustomInstrumentView(m_model->getInstrument(), this);
  auto analysisView = new MantidWidgets::PlotFitAnalysisPaneView(-15.0, 15.0, m_view);
  auto analysisModel = new MantidWidgets::PlotFitAnalysisPaneModel();
  m_analysisPane =
      new MantidWidgets::PlotFitAnalysisPanePresenter(analysisView, analysisModel);

  m_presenter = new ALFCustomInstrumentPresenter(m_view, m_model, m_analysisPane);
}
void ALFView::initLayout() { this->setCentralWidget(m_view); }

} // namespace CustomInterfaces
} // namespace MantidQt
