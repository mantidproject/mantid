#include "JumpFit.h"
#include "JumpFitDataPresenter.h"

#include "../General/UserInputValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFit::JumpFit(QWidget *parent)
    : IndirectFitAnalysisTab(new JumpFitModel, parent),
      m_uiForm(new Ui::JumpFit) {
  m_uiForm->setupUi(parent);

  m_jumpFittingModel = dynamic_cast<JumpFitModel *>(fittingModel());
  setFitDataPresenter(Mantid::Kernel::make_unique<JumpFitDataPresenter>(
      m_jumpFittingModel, m_uiForm->fitDataView, m_uiForm->cbParameterType,
      m_uiForm->cbParameter, m_uiForm->lbParameterType, m_uiForm->lbParameter));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void JumpFit::setupFitTab() {
  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

  setSampleWSSuffices({"_Result"});
  setSampleFBSuffices({"_Result.nxs"});

  auto &functionFactory = FunctionFactory::Instance();
  auto chudleyElliot = functionFactory.createFunction("ChudleyElliot");
  auto hallRoss = functionFactory.createFunction("HallRoss");
  auto fickDiffusion = functionFactory.createFunction("FickDiffusion");
  auto teixeiraWater = functionFactory.createFunction("TeixeiraWater");
  auto eisfDiffCylinder = functionFactory.createFunction("EISFDiffCylinder");
  auto eisfDiffSphere = functionFactory.createFunction("EISFDiffSphere");
  auto eisfDiffSphereAklyl =
      functionFactory.createFunction("EISFDiffSphereAlkyl");
  addComboBoxFunctionGroup("ChudleyElliot", {chudleyElliot});
  addComboBoxFunctionGroup("HallRoss", {hallRoss});
  addComboBoxFunctionGroup("FickDiffusion", {fickDiffusion});
  addComboBoxFunctionGroup("TeixeiraWater", {teixeiraWater});
  addComboBoxFunctionGroup("EISFDiffCylinder", {eisfDiffCylinder});
  addComboBoxFunctionGroup("EISFDiffSphere", {eisfDiffSphere});
  addComboBoxFunctionGroup("EISFDiffSphereAlkyl", {eisfDiffSphereAklyl});

  m_uiForm->cbParameter->setEnabled(false);

  // Handle plotting and saving
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

void JumpFit::updateModelFitTypeString() {
  m_jumpFittingModel->setFitType(selectedFitType().toStdString());
}

void JumpFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

void JumpFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
  m_uiForm->cbPlotType->setEnabled(enabled);
}

void JumpFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

void JumpFit::plotClicked() {
  IndirectFitAnalysisTab::plotResult(m_uiForm->cbPlotType->currentText());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
