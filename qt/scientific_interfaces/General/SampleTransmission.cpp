//----------------------
// Includes
//----------------------
#include "SampleTransmission.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Statistics.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "UserInputValidator.h"

namespace {
Mantid::Kernel::Logger g_log("SampleTransmission");
}

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(SampleTransmission)
}
} // namespace MantidQt

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

//----------------------
// Public member functions
//----------------------
/// Constructor
SampleTransmission::SampleTransmission(QWidget *parent)
    : UserSubWindow(parent), m_algRunner(new API::AlgorithmRunner(this)) {
  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
}

/**
 * Set up the dialog layout.
 */
void SampleTransmission::initLayout() {
  m_uiForm.setupUi(this);
  connect(m_uiForm.pbCalculate, SIGNAL(clicked()), this, SLOT(calculate()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(showHelp()));

  validate(true);
}

/**
 * Opens the Qt help page for the interface.
 */
void SampleTransmission::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Sample Transmission Calculator"));
}

/**
 * Validate user input.
 * Outputs any warnings to the results log at warning level.
 *
 * @param silent If the results should not be logged
 * @return Result of validation
 */
bool SampleTransmission::validate(bool silent) {
  UserInputValidator uiv;

  // Valudate input binning
  int wavelengthBinning = m_uiForm.cbBinningType->currentIndex();
  switch (wavelengthBinning) {
  // Single
  case 0:
    uiv.checkBins(m_uiForm.spSingleLow->value(),
                  m_uiForm.spSingleWidth->value(),
                  m_uiForm.spSingleHigh->value());
    break;

  // Multiple
  case 1:
    uiv.checkFieldIsNotEmpty("Multiple binning", m_uiForm.leMultiple,
                             m_uiForm.valMultiple);
    break;
  }

  // Validate chemical formula
  uiv.checkFieldIsNotEmpty("Chemical Formula", m_uiForm.leChemicalFormula,
                           m_uiForm.valChemicalFormula);

  // Ensure number density is not zero
  uiv.setErrorLabel(m_uiForm.valDensity,
                    uiv.checkNotEqual("Density", m_uiForm.spDensity->value()));

  // Ensure thickness is not zero
  uiv.setErrorLabel(
      m_uiForm.valThickness,
      uiv.checkNotEqual("Thickness", m_uiForm.spThickness->value()));

  // Give error message
  if (!silent && !uiv.isAllInputValid())
    showInformationBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

/**
 * Performs a calculation with the current settings
 */
void SampleTransmission::calculate() {
  // Do not try to run with invalid input
  if (!validate())
    return;

  // Create the transmission calculation algorithm
  IAlgorithm_sptr transCalcAlg =
      AlgorithmManager::Instance().create("CalculateSampleTransmission");
  transCalcAlg->initialize();

  // Set the wavelength binning based on type set in UI
  int wavelengthBinning = m_uiForm.cbBinningType->currentIndex();
  switch (wavelengthBinning) {
  // Single
  case 0: {
    // Convert values to binning params using the 'C' locale.
    std::ostringstream binning;
    binning.imbue(std::locale::classic());
    binning << m_uiForm.spSingleLow->value() << ','
            << m_uiForm.spSingleWidth->value() << ','
            << m_uiForm.spSingleHigh->value();
    transCalcAlg->setProperty("WavelengthRange", binning.str());
    break;
  }

  // Multiple
  case 1:
    transCalcAlg->setProperty("WavelengthRange",
                              m_uiForm.leMultiple->text().toStdString());
    break;
  }

  // Set sample material properties
  transCalcAlg->setProperty("ChemicalFormula",
                            m_uiForm.leChemicalFormula->text().toStdString());

  transCalcAlg->setProperty("DensityType",
                            m_uiForm.cbDensity->currentText().toStdString());
  transCalcAlg->setProperty("Density", m_uiForm.spDensity->value());

  transCalcAlg->setProperty("Thickness", m_uiForm.spThickness->value());

  transCalcAlg->setProperty("OutputWorkspace", "CalculatedSampleTransmission");

  // Clear the previous results
  m_uiForm.twResults->clear();
  m_uiForm.ppTransmission->clear();

  // Run algorithm
  m_algRunner->startAlgorithm(transCalcAlg);
}

/**
 * Handles completion of the calculation algorithm.
 *
 * @param error If the algorithm exited with an error
 */
void SampleTransmission::algorithmComplete(bool error) {
  using namespace Mantid::Kernel;

  // Ignore errors
  if (error) {
    showInformationBox(
        "Transmission calculation failed.\nSee Results Log for details.");
    return;
  }

  MatrixWorkspace_sptr ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          "CalculatedSampleTransmission");

  // Fill the output table
  double scattering = ws->y(1)[0];
  QTreeWidgetItem *scatteringItem = new QTreeWidgetItem();
  scatteringItem->setText(0, "Scattering");
  scatteringItem->setText(1, QString::number(scattering));
  m_uiForm.twResults->addTopLevelItem(scatteringItem);

  QTreeWidgetItem *transmissionItem = new QTreeWidgetItem();
  transmissionItem->setText(0, "Transmission");
  m_uiForm.twResults->addTopLevelItem(transmissionItem);
  transmissionItem->setExpanded(true);

  Statistics stats = getStatistics(ws->y(0).rawData());

  QMap<QString, double> transmissionStats;
  transmissionStats["Min"] = stats.minimum;
  transmissionStats["Max"] = stats.maximum;
  transmissionStats["Mean"] = stats.mean;
  transmissionStats["Median"] = stats.median;
  transmissionStats["Std. Dev."] = stats.standard_deviation;

  for (auto it = transmissionStats.begin(); it != transmissionStats.end();
       ++it) {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, it.key());
    item->setText(1, QString::number(it.value()));
    transmissionItem->addChild(item);
  }

  m_uiForm.twResults->resizeColumnToContents(0);

  try {
    // Plot transmission curve on preview plot
    m_uiForm.ppTransmission->addSpectrum("Transmission",
                                         "CalculatedSampleTransmission", 0);
    m_uiForm.ppTransmission->resizeX();
  } catch (std::runtime_error &e) {
    // PreviewPlot may throw an exception if our workspace has less than two X
    // values
    showInformationBox(
        QString::fromStdString("Unable to plot CalculatedSampleTransmission: " +
                               std::string(e.what())));
  }
}
