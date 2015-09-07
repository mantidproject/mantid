#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"


using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ContainerSubtraction");
}

namespace MantidQt {
namespace CustomInterfaces {
ContainerSubtraction::ContainerSubtraction(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);

  // Connect slots
  connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleGeometryChanged(int)));
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newData(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ContainerSubtraction::setup() {}
void ContainerSubtraction::run() {}
bool ContainerSubtraction::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Container", m_uiForm.dsContainer);
  MatrixWorkspace_sptr sampleWs;
  QString sample = m_uiForm.dsSample->getCurrentDataName();
  QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
  QString container = m_uiForm.dsContainer->getCurrentDataName();
  QString containerType =
      container.right(sample.length() - container.lastIndexOf("_"));

  g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
  g_log.debug() << "Container type is: " << containerType.toStdString()
                << std::endl;

  if (containerType != sampleType)
    uiv.addErrorMessage(
        "Sample and can workspaces must contain the same type of data.");

  // Use corrections?

  // Show errors if there are any
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

void ContainerSubtraction::loadSettings(const QSettings &settings) {
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
 * Disables corrections when using S(Q, w) as input data.
 *
 * @param dataName Name of new data source
 */
void ContainerSubtraction::newData(const QString &dataName) {
 const MatrixWorkspace_sptr sampleWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          dataName.toStdString());
  m_uiForm.spPreviewSpec->setMaximum(
      static_cast<int>(sampleWs->getNumberHistograms()) - 1);

  // Plot the sample curve
  m_uiForm.ppPreview->clear();
  m_uiForm.ppPreview->addSpectrum("Sample", sampleWs, 0, Qt::black);
}


/**
 * Handles when the type of geometry changes
 *
 * Updates the file extension to search for
 */
void ContainerSubtraction::handleGeometryChange(int index) {
  QString ext("");
  switch (index) {
  case 0:
    // Geometry is flat
    ext = "_flt_abs";
    break;
  case 1:
    // Geometry is cylinder
    ext = "_cyl_abs";
    break;
  case 2:
    // Geometry is annulus
    ext = "_ann_abs";
    break;
  }
  /*m_uiForm.dsCorrections->setWSSuffixes(QStringList(ext));
  m_uiForm.dsCorrections->setFBSuffixes(QStringList(ext + ".nxs"));*/
}

/**
 * Replots the preview plot.
 *
 * @param specIndex Spectrum index to plot
 */
void ContainerSubtraction::plotPreview(int specIndex) {
  //bool useCan = m_uiForm.ckUseCan->isChecked();

  m_uiForm.ppPreview->clear();

  // Plot sample
  m_uiForm.ppPreview->addSpectrum(
      "Sample", m_uiForm.dsSample->getCurrentDataName(), specIndex, Qt::black);

  // Plot result
  if (!m_pythonExportWsName.empty())
    m_uiForm.ppPreview->addSpectrum(
        "Corrected", QString::fromStdString(m_pythonExportWsName), specIndex,
        Qt::green);

  // Plot can
 /* if (useCan)
    m_uiForm.ppPreview->addSpectrum(
        "Can", m_uiForm.dsContainer->getCurrentDataName(), specIndex, Qt::red);*/
}
}
}