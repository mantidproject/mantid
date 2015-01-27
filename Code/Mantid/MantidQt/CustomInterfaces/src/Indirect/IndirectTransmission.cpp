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
  IndirectTransmission::IndirectTransmission(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    m_uiForm.setupUi(parent);

    // Preview plot
    m_plots["PreviewPlot"] = new QwtPlot(m_parentWidget);
    m_plots["PreviewPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["PreviewPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["PreviewPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.plotPreview->addWidget(m_plots["PreviewPlot"]);

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
    QString currentInst = "IRIS"; //m_uiForm.iicInstrumentConfiguration->getInstrumentName(); TODO
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

    // Plot each spectrum
    plotMiniPlot(QString::fromStdString(resultWsNames[0]), 0, "PreviewPlot", "SamCurve");
    plotMiniPlot(QString::fromStdString(resultWsNames[1]), 0, "PreviewPlot", "CanCurve");
    plotMiniPlot(QString::fromStdString(resultWsNames[2]), 0, "PreviewPlot", "TransCurve");

    // Colour plots as per plot option
    m_curves["SamCurve"]->setPen(QColor(Qt::red));
    m_curves["CanCurve"]->setPen(QColor(Qt::black));
    m_curves["TransCurve"]->setPen(QColor(Qt::green));

    // Set X range to data range
    setXAxisToCurve("PreviewPlot", "TransCurve");
    m_plots["PreviewPlot"]->replot();
  }

} // namespace CustomInterfaces
} // namespace Mantid
