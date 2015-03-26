#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/Indirect/CalcCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <QLineEdit>
#include <QList>
#include <QValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("CalcCorr");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  CalcCorr::CalcCorr(QWidget * parent) :
    IDATab(parent)
  {
    m_uiForm.setupUi(parent);

    connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(getBeamWidthFromWorkspace(const QString&)));
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

    QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
    QValidator *formulaValidator = new QRegExpValidator(regex, this);
    m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
    m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);
  }


  void CalcCorr::setup()
  {
  }


  void CalcCorr::run()
  {
    //TODO

    // Set the result workspace for Python script export
    m_pythonExportWsName = "";
  }


  bool CalcCorr::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

    if(uiv.checkFieldIsNotEmpty("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula))
      uiv.checkFieldIsValid("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula, m_uiForm.valSampleChemicalFormula);

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if (useCan)
    {
      uiv.checkDataSelectorIsValid("Can", m_uiForm.dsContainer);

      if(uiv.checkFieldIsNotEmpty("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula))
        uiv.checkFieldIsValid("Can Chemical Formula", m_uiForm.leCanChemicalFormula, m_uiForm.valCanChemicalFormula);

      QString sample = m_uiForm.dsSample->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = m_uiForm.dsContainer->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Can type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
        uiv.addErrorMessage("Sample and can workspaces must contain the same type of data.");
    }

    // Show error mssage if needed
    if(!uiv.isAllInputValid())
      emit showMessageBox(uiv.generateErrorMessage());

    return uiv.isAllInputValid();
  }


  /**
   * Handles completion of the correction algorithm.
   *
   * @param error True of the algorithm failed
   */
  void CalcCorr::algorithmComplete(bool error)
  {
    if(error)
    {
      emit showMessageBox("Absorption correction calculation failed.\nSee Results Log for more details.");
    }
  }


  void CalcCorr::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSample->readSettings(settings.group());
    m_uiForm.dsContainer->readSettings(settings.group());
  }


  void CalcCorr::getBeamWidthFromWorkspace(const QString& wsName)
  {
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

    if(!ws)
    {
      g_log.warning() << "Failed to find workspace " << wsName.toStdString() << std::endl;
      return;
    }

    std::string paramName = "Workflow.beam-width";
    auto instrument = ws->getInstrument();
    if(instrument->hasParameter(paramName))
    {
      std::string beamWidth = instrument->getStringParameter(paramName)[0];
      //TODO
    }
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
