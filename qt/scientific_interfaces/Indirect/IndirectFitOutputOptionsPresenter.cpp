// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IndirectFitOutputOptionsModel *model, IndirectFitOutputOptionsView *view)
    : QObject(nullptr), m_model(model), m_view(view) {

  connect(m_view.get(), SIGNAL(plotClicked()), this, SLOT(plotResult()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveResult()));
}

IndirectFitOutputOptionsPresenter::~IndirectFitOutputOptionsPresenter() {}

void IndirectFitOutputOptionsPresenter::plotResult() {
  m_view->setAsPlotting(true);
  m_model->plotResult(m_view->getPlotType());
  m_view->setAsPlotting(false);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
