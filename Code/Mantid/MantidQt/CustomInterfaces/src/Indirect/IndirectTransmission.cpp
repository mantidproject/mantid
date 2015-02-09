#include "MantidQtCustomInterfaces/Indirect/IndirectTransmission.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectTransmission::IndirectTransmission(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    // Preview plot
    // TODO: Move to UI file
    m_plot = new MantidWidgets::PreviewPlot(m_parentWidget);
    m_plot->setCanvasColour(Qt::white);
    m_uiForm.plotPreview->addWidget(m_plot);

    // Update the preview plot when the algorithm is complete
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(transAlgDone(bool)));
    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(QString)), this, SLOT(dataLoaded()));
    connect(m_uiForm.dsCanInput, SIGNAL(dataReady(QString)), this, SLOT(dataLoaded()));
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
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
    QString outWsName = sampleWsName + "_trans";

    IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
    transAlg->initialize();

    transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
    transAlg->setProperty("CanWorkspace", canWsName.toStdString());
    transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

    transAlg->setProperty("Verbose", m_uiForm.ckVerbose->isChecked());
    transAlg->setProperty("Plot", m_uiForm.ckPlot->isChecked());
    transAlg->setProperty("Save", m_uiForm.ckSave->isChecked());

    runAlgorithm(transAlg);
  }

  bool IndirectTransmission::validate()
  {
    // Check if we have an appropriate instrument
    QString currentInst = getInstrumentConfiguration()->getInstrumentName();
    if(currentInst != "IRIS" && currentInst != "OSIRIS")
      return false;

    // Check for an invalid sample input
    if(!m_uiForm.dsSampleInput->isValid())
      return false;

    // Check for an invalid can input
    if(!m_uiForm.dsCanInput->isValid())
      return false;

    return true;
  }

  void IndirectTransmission::dataLoaded()
  {
    if(validate())
      previewPlot();
  }

  void IndirectTransmission::previewPlot()
  {
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
    QString outWsName = sampleWsName + "_trans";

    IAlgorithm_sptr transAlg = AlgorithmManager::Instance().create("IndirectTransmissionMonitor", -1);
    transAlg->initialize();

    transAlg->setProperty("SampleWorkspace", sampleWsName.toStdString());
    transAlg->setProperty("CanWorkspace", canWsName.toStdString());
    transAlg->setProperty("OutputWorkspace", outWsName.toStdString());

    transAlg->setProperty("Verbose", m_uiForm.ckVerbose->isChecked());
    transAlg->setProperty("Plot", false);
    transAlg->setProperty("Save", false);

    // Set the workspace name for Python script export
    m_pythonExportWsName = sampleWsName.toStdString() + "_Trans";

    runAlgorithm(transAlg);
  }

  void IndirectTransmission::transAlgDone(bool error)
  {
    if(error)
      return;

    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    QString outWsName = sampleWsName + "_trans";

    WorkspaceGroup_sptr resultWsGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outWsName.toStdString());
    std::vector<std::string> resultWsNames = resultWsGroup->getNames();

    if(resultWsNames.size() < 3)
      return;

    // Do plotting
    m_plot->clear();
    m_plot->addSpectrum(QString::fromStdString(resultWsNames[0]), 0, Qt::red);
    m_plot->addSpectrum(QString::fromStdString(resultWsNames[1]), 0, Qt::black);
    m_plot->addSpectrum(QString::fromStdString(resultWsNames[2]), 0, Qt::green);
    m_plot->resizeX();
  }

} // namespace CustomInterfaces
} // namespace Mantid
