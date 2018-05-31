#include "JumpFit.h"
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
  setFitDataPresenter(
      new IndirectFitDataPresenter(m_jumpFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void JumpFit::setupFitTab() {
  m_uiForm->svSpectrumView->hideSpectrumSelector();
  m_uiForm->svSpectrumView->hideMaskSpectrumSelector();

  setSampleWSSuffices({ "_Result" });
  setSampleFBSuffices({ "_Result.nxs" });

  auto chudleyElliot =
      FunctionFactory::Instance().createFunction("ChudleyElliot");
  auto hallRoss = FunctionFactory::Instance().createFunction("HallRoss");
  auto fickDiffusion =
      FunctionFactory::Instance().createFunction("FickDiffusion");
  auto teixeiraWater =
      FunctionFactory::Instance().createFunction("TeixeiraWater");
  addComboBoxFunctionGroup("ChudleyElliot", {chudleyElliot});
  addComboBoxFunctionGroup("HallRoss", {hallRoss});
  addComboBoxFunctionGroup("FickDiffusion", {fickDiffusion});
  addComboBoxFunctionGroup("TeixeiraWater", {teixeiraWater});

  m_uiForm->cbWidth->setEnabled(false);

  // Connect width selector to handler method
  connect(m_uiForm->cbWidth, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleWidthChange(int)));

  // Handle plotting and saving
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(this, SIGNAL(functionChanged()), this,
          SLOT(updateModelFitTypeString()));
}

void JumpFit::updateModelFitTypeString() {
  m_jumpFittingModel->setFitType(selectedFitType().toStdString());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void JumpFit::handleSampleInputReady(const QString &filename) {
  setAvailableWidths(m_jumpFittingModel->getWidths());

  if (m_jumpFittingModel->getWorkspace(0)) {
    m_uiForm->cbWidth->setEnabled(true);
    const auto width =
        static_cast<int>(m_jumpFittingModel->getWidthSpectrum(0));
    setMinimumSpectrum(width);
    setMaximumSpectrum(width);
    setSelectedSpectrum(width);
  } else {
    m_uiForm->cbWidth->setEnabled(false);
    emit showMessageBox("Workspace doesn't appear to contain any width data");
  }
}

void JumpFit::setAvailableWidths(const std::vector<std::string> &widths) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm->cbWidth);
  m_uiForm->cbWidth->clear();
  for (const auto &width : widths)
    m_uiForm->cbWidth->addItem(QString::fromStdString(width));
}

/**
 * Plots the loaded file to the miniplot when the selected spectrum changes
 *
 * @param text :: The name spectrum index to plot
 */
void JumpFit::handleWidthChange(int widthIndex) {
  auto index = static_cast<std::size_t>(widthIndex);
  auto spectrum = static_cast<int>(m_jumpFittingModel->getWidthSpectrum(index));
  m_jumpFittingModel->setActiveWidth(index);
  setSelectedSpectrum(spectrum);
}

void JumpFit::updatePlotOptions() {}

void JumpFit::setPlotResultEnabled(bool enabled) {}

void JumpFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
