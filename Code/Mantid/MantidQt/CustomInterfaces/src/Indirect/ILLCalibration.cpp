#include "MantidQtCustomInterfaces/Indirect/ILLCalibration.h"

#include <QFileInfo>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ILLCalibration::ILLCalibration(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ILLCalibration::~ILLCalibration()
  {
  }


  void ILLCalibration::setup()
  {
  }


  void ILLCalibration::run()
  {
    std::map<QString, QString> instDetails = getInstrumentDetails();

    IAlgorithm_sptr calibrationAlg = AlgorithmManager::Instance().create("ILLIN16BCalibration");
    calibrationAlg->initialize();

    // Handle einput files
    QString runFilename = m_uiForm.rfRunFile->getFirstFilename();
    calibrationAlg->setProperty("Run", runFilename.toStdString());

    // Set options
    bool mirrorMode = m_uiForm.ckMirrorMode->isChecked();
    calibrationAlg->setProperty("MirrorMode", mirrorMode);

    std::vector<long> specRange;
    specRange.push_back(m_uiForm.spSpecMin->value());
    specRange.push_back(m_uiForm.spSpecMax->value());
    calibrationAlg->setProperty("SpectraRange", specRange);

    std::vector<double> peakRange;
    peakRange.push_back(m_uiForm.spPeakLower->value());
    peakRange.push_back(m_uiForm.spPeakUpper->value());
    calibrationAlg->setProperty("PeakRange", peakRange);

    double scaleFactor = m_uiForm.spScaleFactor->value();
    calibrationAlg->setProperty("ScaleFactor", scaleFactor);

    // Get the name format for output files
    QFileInfo runFileInfo(runFilename);
    QString outputWsName = runFileInfo.baseName() +
                           "_" + instDetails["analyser"] +
                           "_" + instDetails["reflection"] +
                           "_calib";

    // Set output workspace properties
    calibrationAlg->setProperty("OutputWorkspace", outputWsName.toStdString());

    m_batchAlgoRunner->addAlgorithm(calibrationAlg);

    bool save = m_uiForm.ckSave->isChecked();
    if(save)
    {
      //TODO: saving
    }

    m_batchAlgoRunner->executeBatchAsync();
    m_pythonExportWsName = outputWsName.toStdString();
  }


  void ILLCalibration::algorithmComplete(bool error)
  {
    if(error)
      return;

    // Handle plotting
    bool plot = m_uiForm.ckPlot->isChecked();
    if(plot)
      plotTimeBin(QString::fromStdString(m_pythonExportWsName), 0);
  }


  bool ILLCalibration::validate()
  {
    MantidQt::CustomInterfaces::UserInputValidator uiv;

    // Validate spectra range
    auto specRange = std::make_pair<double, double>(m_uiForm.spSpecMin->value(), m_uiForm.spSpecMax->value());
    uiv.checkValidRange("Spectra Range", specRange);

    // Validate peak range
    auto peakRange = std::make_pair<double, double>(m_uiForm.spPeakLower->value(), m_uiForm.spPeakUpper->value());
    uiv.checkValidRange("Peak Range", peakRange);

    // Validate run file
    if(!m_uiForm.rfRunFile->isValid())
      uiv.addErrorMessage("Run File is invalid.");

    // Show error for failed validation
    if(!uiv.isAllInputValid())
      emit showMessageBox(uiv.generateErrorMessage());

    return uiv.isAllInputValid();
  }

} // namespace CustomInterfaces
} // namespace Mantid
