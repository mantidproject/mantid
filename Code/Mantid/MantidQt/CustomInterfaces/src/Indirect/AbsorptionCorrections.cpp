#include "MantidKernel/Unit.h"

#include "MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QRegExpValidator>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("AbsorptionCorrections");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  AbsorptionCorrections::AbsorptionCorrections(QWidget * parent) :
    IDATab(parent)
  {
    m_uiForm.setupUi(parent);

    QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
    QValidator *formulaValidator = new QRegExpValidator(regex, this);
    m_uiForm.leSampleChemicalFormula->setValidator(formulaValidator);
    m_uiForm.leCanChemicalFormula->setValidator(formulaValidator);

    // Handle algorithm completion
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)),
            this, SLOT(algorithmComplete(bool)));
  }


  void AbsorptionCorrections::setup()
  {
  }


  void AbsorptionCorrections::run()
  {
    // Get correct corrections algorithm
    QString sampleShape = m_uiForm.cbShape->currentText().replace(" ", "");
    QString algorithmName = "Indirect" + sampleShape + "Absorption";

    IAlgorithm_sptr absCorAlgo = AlgorithmManager::Instance().create(algorithmName.toStdString());
    absCorAlgo->initialize();

    // Sample details
    QString sampleWsName = m_uiForm.dsSampleInput->getCurrentDataName();
    absCorAlgo->setProperty("SampleWorkspace", sampleWsName.toStdString());

    double sampleNumberDensity = m_uiForm.spSampleNumberDensity->value();
    absCorAlgo->setProperty("SampleNumberDensity", sampleNumberDensity);

    QString sampleChemicalFormula = m_uiForm.leSampleChemicalFormula->text();
    absCorAlgo->setProperty("SampleChemicalFormula", sampleChemicalFormula.toStdString());

    addShapeSpecificSampleOptions(absCorAlgo, sampleShape);

    // Can details
    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      QString canWsName = m_uiForm.dsCanInput->getCurrentDataName();
      absCorAlgo->setProperty("CanWorkspace", canWsName.toStdString());

      bool useCanCorrections = m_uiForm.ckUseCanCorrections->isChecked();
      absCorAlgo->setProperty("UseCanCorrections", useCanCorrections);

      if(useCanCorrections)
      {
        double canNumberDensity = m_uiForm.spCanNumberDensity->value();
        absCorAlgo->setProperty("CanNumberDensity", canNumberDensity);

        QString canChemicalFormula = m_uiForm.leCanChemicalFormula->text();
        absCorAlgo->setProperty("CanChemicalFormula", canChemicalFormula.toStdString());
      }

      addShapeSpecificCanOptions(absCorAlgo, sampleShape);
    }

    bool plot = m_uiForm.ckPlot->isChecked();
    absCorAlgo->setProperty("Plot", plot);

    // Generate workspace names
    int nameCutIndex = sampleWsName.lastIndexOf("_");
    if(nameCutIndex == -1)
      nameCutIndex = sampleWsName.length();

    QString outputBaseName = sampleWsName.left(nameCutIndex);

    QString outputWsName = outputBaseName + "_" + sampleShape + "_Corrected";
    absCorAlgo->setProperty("OutputWorkspace", outputWsName.toStdString());

    // Set the correction workspace to keep the factors if desired
    bool keepCorrectionFactors = m_uiForm.ckKeepFactors->isChecked();
    QString outputFactorsWsName = outputBaseName + "_" + sampleShape + "_Factors";
    if(keepCorrectionFactors)
      absCorAlgo->setProperty("CorrectionsWorkspace", outputFactorsWsName.toStdString());

    // Add correction algorithm to batch
    m_batchAlgoRunner->addAlgorithm(absCorAlgo);

    // Add save algorithms if needed
    bool save = m_uiForm.ckSave->isChecked();
    if(save)
    {
      addSaveWorkspace(outputWsName);
      if(keepCorrectionFactors)
        addSaveWorkspace(outputFactorsWsName);
    }

    // Run algorithm batch
    m_batchAlgoRunner->executeBatchAsync();

    // Set the result workspace for Python script export
    m_pythonExportWsName = outputWsName.toStdString();
  }


  /**
   * Configures the SaveNexusProcessed algorithm to save a workspace in the default
   * save directory and adds the algorithm to the batch queue.
   *
   * @param wsName Name of workspace to save
   */
  void AbsorptionCorrections::addSaveWorkspace(QString wsName)
  {
    QString filename = wsName + ".nxs";

    // Setup the input workspace property
    API::BatchAlgorithmRunner::AlgorithmRuntimeProps saveProps;
    saveProps["InputWorkspace"] = wsName.toStdString();

    // Setup the algorithm
    IAlgorithm_sptr saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
    saveAlgo->initialize();
    saveAlgo->setProperty("Filename", filename.toStdString());

    // Add the save algorithm to the batch
    m_batchAlgoRunner->addAlgorithm(saveAlgo, saveProps);
  }


  /**
   * Sets algorithm properties specific to the sample for a given shape.
   *
   * @param alg Algorithm to set properties of
   * @param shape Sample shape
   */
  void AbsorptionCorrections::addShapeSpecificSampleOptions(IAlgorithm_sptr alg, QString shape)
  {
    if(shape == "FlatPlate")
    {
      double sampleHeight = m_uiForm.spFlatSampleHeight->value();
      alg->setProperty("SampleHeight", sampleHeight);

      double sampleWidth = m_uiForm.spFlatSampleWidth->value();
      alg->setProperty("SampleWidth", sampleWidth);

      double sampleThickness = m_uiForm.spFlatSampleThickness->value();
      alg->setProperty("SampleThickness", sampleThickness);

      double elementSize = m_uiForm.spFlatElementSize->value();
      alg->setProperty("ElementSize", elementSize);
    }
    else if(shape == "Annulus")
    {
      double sampleInnerRadius = m_uiForm.spAnnSampleInnerRadius->value();
      alg->setProperty("SampleInnerRadius", sampleInnerRadius);

      double sampleOuterRadius = m_uiForm.spAnnSampleOuterRadius->value();
      alg->setProperty("SampleOuterRadius", sampleOuterRadius);

      double canInnerRadius = m_uiForm.spAnnCanInnerRadius->value();
      alg->setProperty("CanInnerRadius", canInnerRadius);

      double canOuterRadius = m_uiForm.spAnnCanOuterRadius->value();
      alg->setProperty("CanOuterRadius", canOuterRadius);

      long events = static_cast<long>(m_uiForm.spAnnEvents->value());
      alg->setProperty("Events", events);
    }
    else if(shape == "Cylinder")
    {
      double sampleRadius = m_uiForm.spCylSampleRadius->value();
      alg->setProperty("SampleRadius", sampleRadius);

      long events = static_cast<long>(m_uiForm.spCylEvents->value());
      alg->setProperty("Events", events);
    }
  }


  /**
   * Sets algorithm properties specific to the can for a given shape.
   *
   * All options for Annulus are added in addShapeSpecificSampleOptions.
   *
   * @param alg Algorithm to set properties of
   * @param shape Sample shape
   */
  void AbsorptionCorrections::addShapeSpecificCanOptions(IAlgorithm_sptr alg, QString shape)
  {
    if(shape == "FlatPlate")
    {
      double canFrontThickness = m_uiForm.spFlatCanFrontThickness->value();
      alg->setProperty("CanFrontThickness", canFrontThickness);

      double canBackThickness = m_uiForm.spFlatCanBackThickness->value();
      alg->setProperty("CanBackThickness", canBackThickness);
    }
    else if(shape == "Cylinder")
    {
      double canRadius = m_uiForm.spCylCanRadius->value();
      alg->setProperty("CanRadius", canRadius);
    }
  }


  bool AbsorptionCorrections::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

    if(uiv.checkFieldIsNotEmpty("Sample Chemical Formula", m_uiForm.leSampleChemicalFormula))
      uiv.checkFieldIsValid("Sample Chamical Formula", m_uiForm.leSampleChemicalFormula);

    bool useCan = m_uiForm.ckUseCan->isChecked();
    if(useCan)
    {
      uiv.checkDataSelectorIsValid("Container", m_uiForm.dsCanInput);

      bool useCanCorrections = m_uiForm.ckUseCanCorrections->isChecked();
      if(useCanCorrections)
      {
        if(uiv.checkFieldIsNotEmpty("Container Chamical Formula", m_uiForm.leCanChemicalFormula))
          uiv.checkFieldIsValid("Container Chamical Formula", m_uiForm.leCanChemicalFormula);
      }
    }

    // Give error for failed validation
    if(!uiv.isAllInputValid())
    {
      QString error = uiv.generateErrorMessage();
      showMessageBox(error);
    }

    return uiv.isAllInputValid();
  }


  void AbsorptionCorrections::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSampleInput->readSettings(settings.group());
    m_uiForm.dsCanInput->readSettings(settings.group());
  }


  /**
   * Handle completion of the absorption correction algorithm.
   *
   * @param error True if algorithm has failed.
   */
  void AbsorptionCorrections::algorithmComplete(bool error)
  {
    if(error)
    {
      emit showMessageBox("Could not run absorption corrections.\nSee Results Log for details.");
    }
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
