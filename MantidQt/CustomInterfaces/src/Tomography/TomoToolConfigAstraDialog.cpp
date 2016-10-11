#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigAstraDialog.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoToolConfigAstraDialog::DEFAULT_TOOL_NAME = "Astra";
const std::string TomoToolConfigAstraDialog::DEFAULT_TOOL_METHOD = "FBP3D_CUDA";

void TomoToolConfigAstraDialog::setupDialogUi() {
  m_astraUi.setupUi(this);
  m_astraUi.comboBox_method->clear();
  const auto methods = ToolConfigAstraToolbox::methods();
  for (size_t i = 0; i < methods.size(); i++) {
    m_astraUi.comboBox_method->addItem(
        QString::fromStdString(methods[i].second));
  }
}

void TomoToolConfigAstraDialog::setupToolConfig() {
  const auto methods = ToolConfigTomoPy::methods();

  int mi = m_astraUi.comboBox_method->currentIndex();

  m_tempSettings =
      std::unique_ptr<ToolConfigAstraToolbox>(new ToolConfigAstraToolbox(
          m_runPath, m_pathOut + m_localOutNameAppendix, m_paths.pathDarks(),
          m_paths.pathOpenBeam(), m_paths.pathSamples()));

  m_toolMethod = methods[mi].first;
}
int TomoToolConfigAstraDialog::executeQt() { return this->exec(); }
} // CustomInterfaces
} // MantidQt