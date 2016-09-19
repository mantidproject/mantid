#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h"
#include "MantidAPI/AlgorithmManager.h"

#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

// Tomo GUI methods that process the energy bands tab. This
// could/should become a class of its own, as the tab evolves.

void TomographyIfaceViewQtGUI::readSettingsEnergy() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsSubGroupEnergy));

  m_uiTabEnergy.lineEdit_input_path->setText(
      qs.value("input-path", "").toString());

  m_uiTabEnergy.lineEdit_output_path->setText(
      qs.value("output-path", "").toString());

  m_uiTabEnergy.radioButton_uniform_bands->setChecked(
      qs.value("uniform-bands-on", true).toBool());
  m_uiTabEnergy.radioButton_index_ranges->setChecked(
      qs.value("index-ranges-on", false).toBool());
  m_uiTabEnergy.radioButton_tof_ranges->setChecked(
      qs.value("tof-ranges-on", false).toBool());

  m_uiTabEnergy.spinBox_uniform_bands->setValue(
      qs.value("uniform-bands-value", 1).toInt());
  m_uiTabEnergy.lineEdit_index_ranges->setText(
      qs.value("index-ranges-value", "").toString());
  m_uiTabEnergy.lineEdit_tof_ranges->setText(
      qs.value("tof-ranges-value", "").toString());

  m_uiTabEnergy.comboBox_input_format->setCurrentIndex(
      qs.value("input-image-format", 0).toInt());
  m_uiTabEnergy.comboBox_output_format->setCurrentIndex(
      qs.value("output-image-format", 0).toInt());

  qs.endGroup();
}

void TomographyIfaceViewQtGUI::saveSettingsEnergy() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(m_settingsSubGroupEnergy));

  qs.setValue("input-path", m_uiTabEnergy.lineEdit_input_path->text());
  qs.setValue("output-path", m_uiTabEnergy.lineEdit_output_path->text());

  qs.setValue("uniform-bands-on",
              m_uiTabEnergy.radioButton_uniform_bands->isChecked());
  qs.setValue("index-ranges-on",
              m_uiTabEnergy.radioButton_index_ranges->isChecked());
  qs.setValue("tof-ranges-on",
              m_uiTabEnergy.radioButton_tof_ranges->isChecked());

  qs.setValue("uniform-bands-value",
              m_uiTabEnergy.spinBox_uniform_bands->value());
  qs.setValue("index-ranges-value",
              m_uiTabEnergy.lineEdit_index_ranges->text());
  qs.setValue("tof-ranges-value", m_uiTabEnergy.lineEdit_tof_ranges->text());

  qs.setValue("input-image-format",
              m_uiTabEnergy.comboBox_input_format->currentIndex());
  qs.setValue("output-image-format",
              m_uiTabEnergy.comboBox_output_format->currentIndex());

  qs.endGroup();
}

void TomographyIfaceViewQtGUI::doSetupSectionEnergy() {
  m_aggAlgRunner =
      Mantid::Kernel::make_unique<MantidQt::API::BatchAlgorithmRunner>();

  connect(m_uiTabEnergy.pushButton_browse_input, SIGNAL(released()), this,
          SLOT(browseEnergyInputClicked()));

  connect(m_uiTabEnergy.pushButton_browse_output, SIGNAL(released()), this,
          SLOT(browseEnergyOutputClicked()));

  connect(m_uiTabEnergy.pushButton_agg, SIGNAL(released()), this,
          SLOT(pushButtonAggClicked()));

  connect(m_uiTabEnergy.pushButton_browse_script, SIGNAL(released()), this,
          SLOT(browseAggScriptClicked()));
}

std::map<std::string, std::string>
TomographyIfaceViewQtGUI::grabCurrentAggParams() const {
  std::map<std::string, std::string> params;

  params["InputPath"] = m_uiTabEnergy.lineEdit_input_path->text().toStdString();
  params["OutputPath"] =
      m_uiTabEnergy.lineEdit_output_path->text().toStdString();

  if (m_uiTabEnergy.radioButton_uniform_bands->isChecked()) {
    params["UniformBands"] =
        std::to_string(m_uiTabEnergy.spinBox_uniform_bands->value());
  } else if (m_uiTabEnergy.radioButton_index_ranges->isChecked()) {
    params["IndexRanges"] =
        m_uiTabEnergy.lineEdit_index_ranges->text().toStdString();
  } else if (m_uiTabEnergy.radioButton_tof_ranges->isChecked()) {
    params["ToFRanges"] =
        m_uiTabEnergy.lineEdit_tof_ranges->text().toStdString();
  }

  params["InputImageFormat"] = "FITS";
  params["OutputImageFormat"] = "FITS";

  return params;
}

void TomographyIfaceViewQtGUI::browseEnergyInputClicked() {
  checkUserBrowseDir(m_uiTabEnergy.lineEdit_input_path);
}

void TomographyIfaceViewQtGUI::browseEnergyOutputClicked() {
  checkUserBrowseDir(m_uiTabEnergy.lineEdit_output_path);
}

void TomographyIfaceViewQtGUI::pushButtonAggClicked() {
  m_presenter->notify(ITomographyIfacePresenter::AggregateEnergyBands);
}

void TomographyIfaceViewQtGUI::browseAggScriptClicked() {
  checkUserBrowseFile(
      m_uiTabEnergy.lineEdit_script,
      "Select script to aggregate bands on the remote compute resource", false);
}

void TomographyIfaceViewQtGUI::runAggregateBands(
    Mantid::API::IAlgorithm_sptr alg) {

  connect(m_aggAlgRunner.get(), SIGNAL(batchComplete(bool)), this,
          SLOT(finishedAggBands(bool)), Qt::QueuedConnection);

  m_aggAlgRunner->addAlgorithm(alg);

  // run lengthy I/O intensive algorithm async
  m_uiTabEnergy.pushButton_agg->setEnabled(false);
  m_aggAlgRunner->executeBatchAsync();
}

void TomographyIfaceViewQtGUI::finishedAggBands(bool error) {
  if (error) {
    userWarning("Process failed", "Could not run or finish the aggregation of "
                                  "bands. Please check the log messages for "
                                  "details.");
  } else {
    userWarning("Process finished",
                "Aggregation of bands finished. The "
                "results should now be available from the output path given. "
                "You can check the logs for more information and (detailed "
                "warning/error messages if there were any issues).");
  }
  m_uiTabEnergy.pushButton_agg->setEnabled(true);
}

} // namespace CustomInterfaces
} // namespace MantidQt
