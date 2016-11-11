#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogAstra.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoToolConfigDialogAstra::DEFAULT_TOOL_NAME = "Astra";
const std::string TomoToolConfigDialogAstra::DEFAULT_TOOL_METHOD = "FBP3D_CUDA";

void TomoToolConfigDialogAstra::setupDialogUi() {
  m_astraUi.setupUi(m_dialog);
  m_astraUi.comboBox_method->clear();
  const auto &methods = getToolMethods();
  for (auto &method : methods) {
    m_astraUi.comboBox_method->addItem(QString::fromStdString(method.second));
  }
}

void TomoToolConfigDialogAstra::initialiseDialog() { m_dialog = new QDialog; }

void TomoToolConfigDialogAstra::setupToolSettingsFromPaths() {
  m_toolSettings = std::make_shared<ToolConfigAstraToolbox>(
      m_runPath, m_pathOut + m_localOutNameAppendix, m_paths.pathDarks(),
      m_paths.pathOpenBeam(), m_paths.pathSamples());
}

void TomoToolConfigDialogAstra::setupMethodSelected() {
  const auto &methods = getToolMethods();

  const int mi = m_astraUi.comboBox_method->currentIndex();
  m_toolMethod = methods[mi].first;
}

/** Calls the execute of the QDialog
*/
int TomoToolConfigDialogAstra::executeQt() { return m_dialog->exec(); }

std::vector<std::pair<std::string, std::string>>
TomoToolConfigDialogAstra::getToolMethods() {
  return ToolConfigAstraToolbox::methods();
}

} // CustomInterfaces
} // MantidQt