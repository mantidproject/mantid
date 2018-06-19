#include "MSDFit.h"
#include "../General/UserInputValidator.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MSDFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
MSDFit::MSDFit(QWidget *parent)
    : IndirectFitAnalysisTab(new MSDFitModel, parent),
      m_uiForm(new Ui::MSDFit) {
  m_uiForm->setupUi(parent);

  m_msdFittingModel = dynamic_cast<MSDFitModel *>(fittingModel());
  setFitDataPresenter(
      new IndirectFitDataPresenter(m_msdFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void MSDFit::setupFitTab() {
  auto gaussian = FunctionFactory::Instance().createFunction("MSDGauss");
  auto peters = FunctionFactory::Instance().createFunction("MSDPeters");
  auto yi = FunctionFactory::Instance().createFunction("MSDYi");
  addComboBoxFunctionGroup("Gaussian", {gaussian});
  addComboBoxFunctionGroup("Peters", {peters});
  addComboBoxFunctionGroup("Yi", {yi});

  setSampleWSSuffices({"_eq"});
  setSampleFBSuffices({"_eq.nxs"});

  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

void MSDFit::updateModelFitTypeString() {
  m_msdFittingModel->setFitType(selectedFitType().toStdString());
}

void MSDFit::updatePlotOptions() {}

void MSDFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
}

void MSDFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
