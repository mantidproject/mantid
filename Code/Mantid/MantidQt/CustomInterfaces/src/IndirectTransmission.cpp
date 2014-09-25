#include "MantidQtCustomInterfaces/IndirectTransmission.h"

#include <QFileInfo>

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectTransmission::IndirectTransmission(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    // PREVIEW PLOT
    m_plots["PreviewPlot"] = new QwtPlot(m_parentWidget);
    m_plots["PreviewPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["PreviewPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["PreviewPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.trans_plotPreview->addWidget(m_plots["PreviewPlot"]);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectTransmission::~IndirectTransmission()
  {
  }
  
  void IndirectTransmission::setup()
  {
  }

  void IndirectTransmission::run()
  {
    using namespace Mantid::API;

    QString sampleWsName = m_uiForm.trans_dsSampleInput->getCurrentDataName();
    QString canWsName = m_uiForm.trans_dsCanInput->getCurrentDataName();

    IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
    transAlg->initialize();

    transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
    transAlg->setProperty("CanWorkspace", canWsName.toStdString());

    transAlg->setProperty("Verbose", m_uiForm.trans_ckVerbose->isChecked());
    transAlg->setProperty("Plot", m_uiForm.trans_ckPlot->isChecked());
    transAlg->setProperty("Save", m_uiForm.trans_ckSave->isChecked());

    runAlgorithm(transAlg);
  }

  bool IndirectTransmission::validate()
  {
    // Check if we have an appropriate instrument
    QString currentInst = m_uiForm.cbInst->currentText();
    if(currentInst != "IRIS" && currentInst != "OSIRIS")
      return false;

    // Check for an invalid sample input
    if(!m_uiForm.trans_dsSampleInput->isValid())
      return false;

    // Check for an invalid can input
    if(!m_uiForm.trans_dsCanInput->isValid())
      return false;

    return true;
  }

  /* void IndirectTransmission::previewPlot() */
  /* { */
  /* } */

} // namespace CustomInterfaces
} // namespace Mantid
