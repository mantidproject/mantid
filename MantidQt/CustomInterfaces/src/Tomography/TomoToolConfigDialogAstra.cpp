#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigAstraDialog.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoToolConfigAstraDialog::DEFAULT_TOOL_NAME = "Astra";
const std::string TomoToolConfigAstraDialog::DEFAULT_TOOL_METHOD = "FBP3D_CUDA";

void TomoToolConfigAstraDialog::setupDialogUi() {
  m_astraUi.setupUi(m_dialog);
  m_astraUi.comboBox_method->clear();
  const auto methods = ToolConfigAstraToolbox::methods();
  for (size_t i = 0; i < methods.size(); i++) {
    m_astraUi.comboBox_method->addItem(
        QString::fromStdString(methods[i].second));
  }
}

void TomoToolConfigAstraDialog::initialiseDialog() { m_dialog = new QDialog; }

void TomoToolConfigAstraDialog::setupToolSettingsFromPaths() {
  m_toolSettings =
      std::shared_ptr<ToolConfigAstraToolbox>(new ToolConfigAstraToolbox(
          m_runPath, m_pathOut + m_localOutNameAppendix, m_paths.pathDarks(),
          m_paths.pathOpenBeam(), m_paths.pathSamples()));

}

void TomoToolConfigAstraDialog::setupMethodSelected() {
  const auto methods = ToolConfigTomoPy::methods();

  int mi = m_astraUi.comboBox_method->currentIndex();


  m_toolMethod = methods[mi].first;
}

/** Calls the execute of the QDialog
*/
int TomoToolConfigAstraDialog::executeQt() { return m_dialog->exec(); }
} // CustomInterfaces
} // MantidQt