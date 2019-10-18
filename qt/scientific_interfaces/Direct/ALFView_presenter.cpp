// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_view.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FunctionFactory.h"

#include <functional>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_presenter::ALFView_presenter(ALFView_view *view,
                                     BaseInstrumentModel *model,
                                     PlotFitAnalysisPanePresenter *analysisPane)
    : BaseInstrumentPresenter(view, model, analysisPane->getView()), m_view(view),
      m_model(model), m_analysisPane(analysisPane) {
}

void ALFView_presenter::setUpInstrumentAnalysisSplitter() {
  auto composite = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
      Mantid::API::FunctionFactory::Instance().createFunction(
          "CompositeFunction"));
  auto func = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = FlatBackground");
  composite->addFunction(func);
  func = Mantid::API::FunctionFactory::Instance().createInitialized(
      "name = Gaussian, Height = 3000, Sigma= 1.0");
  composite->addFunction(func);

  m_analysisPane->addFunction(composite);
  m_view->setupAnalysisPane(m_analysisPane->getView());
}
// want to pass the presenter not the view...
void ALFView_presenter::loadSideEffects() {
  m_analysisPane->clearCurrentWS();
}

} // namespace CustomInterfaces
} // namespace MantidQt