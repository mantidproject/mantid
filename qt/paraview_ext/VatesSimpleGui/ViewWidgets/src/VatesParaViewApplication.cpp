#include "MantidVatesSimpleGuiViewWidgets/VatesParaViewApplication.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/PluginLibraries.h"

#include "pqAlwaysConnectedBehavior.h"
#include "pqAutoLoadPluginXMLBehavior.h"
#include "pqCollaborationBehavior.h"
#include "pqCommandLineOptionsBehavior.h"
#include "pqCrashRecoveryBehavior.h"
#include "pqDataTimeStepBehavior.h"
#include "pqDefaultViewBehavior.h"
#include "pqInterfaceTracker.h"
#include "pqModelTransformSupportBehavior.h"
#include "pqObjectPickingBehavior.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineContextMenuBehavior.h"
#include "pqPluginSettingsBehavior.h"
#include "pqSpreadSheetVisibilityBehavior.h"
#include "pqStandardPropertyWidgetInterface.h"
#include "pqStandardViewFrameActionsImplementation.h"
#include "pqUndoRedoBehavior.h"
#include "pqVerifyRequiredPluginBehavior.h"
#include "pqViewStreamingBehavior.h"

#include <Poco/Path.h>

#include <string>

#include "vtksys/SystemTools.hxx"

namespace Mantid {
namespace Vates {
namespace SimpleGui {
VatesParaViewApplication::VatesParaViewApplication()
    : m_logger("VatesParaViewApplication"), m_behaviorsSetup(false) {
  // Get the plugin path that we set in the ConfigService.
  const auto &configSvc = Kernel::ConfigService::Instance();
  const auto pvPluginsPath =
      MantidQt::API::qtPluginPathFromCfg("pvplugins.directory");
  if (pvPluginsPath.empty()) {
    throw std::runtime_error(
        "pvplugins.directory key not setup.\nVates plugins will not be "
        "available.\n"
        "Further use will cause the program to crash.\nPlease exit and "
        "set this variable.");
  }

  Q_ASSERT(pqApplicationCore::instance() == nullptr);

  // Provide ParaView's application core with a path to the running executable
  int argc = 1;
  std::string exePath = configSvc.getPathToExecutable();
  std::vector<char> argvConversion(exePath.begin(), exePath.end());
  argvConversion.push_back('\0');
  char *argv[] = {&argvConversion[0]};

  m_logger.debug("Intialize pqApplicationCore with " + exePath + "\n");
  // We need to manually set the PV_PLUGIN_PATH because it's
  // not going to be picked up from the paraview/vtk side otherwise.
  m_logger.debug("Setting PV_PLUGIN_PATH=" + pvPluginsPath + "\n");
  vtksys::SystemTools::PutEnv("PV_PLUGIN_PATH=" + pvPluginsPath);
  new pqPVApplicationCore(argc, argv);
}

/**
 * This function duplicates the nearly identical call in ParaView for their
 * main program setup. This is necessary for the plugin mode since it does
 * not have access to the QMainWindow of MantidPlot.
 */
void VatesParaViewApplication::setupParaViewBehaviors() {
  if (this->m_behaviorsSetup) {
    return;
  }
  this->m_behaviorsSetup = true;
  // Register ParaView interfaces.
  pqInterfaceTracker *pgm = pqApplicationCore::instance()->interfaceTracker();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardPropertyWidgetInterface(pgm));

  pgm->addInterface(new pqStandardViewFrameActionsImplementation(pgm));

  // Load plugins distributed with application.
  pqApplicationCore::instance()->loadDistributedPlugins();

  // Define application behaviors.
  new pqDataTimeStepBehavior(this);
  new pqSpreadSheetVisibilityBehavior(this);
  new pqPipelineContextMenuBehavior(this);
  new pqObjectPickingBehavior(this);
  new pqDefaultViewBehavior(this);
  new pqUndoRedoBehavior(this);
  new pqAlwaysConnectedBehavior(this);
  new pqCrashRecoveryBehavior(this);
  new pqAutoLoadPluginXMLBehavior(this);
  new pqVerifyRequiredPluginBehavior(this);
  new pqCommandLineOptionsBehavior(this);
  new pqCollaborationBehavior(this);
  new pqViewStreamingBehavior(this);
  new pqPluginSettingsBehavior(this);
  new pqModelTransformSupportBehavior(this);
}

VatesParaViewApplication::~VatesParaViewApplication() {}

VatesParaViewApplication *VatesParaViewApplication::instance() {
  static QPointer<VatesParaViewApplication> arg;
  if (!arg) {
    arg = new VatesParaViewApplication();
  }
  return arg;
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
