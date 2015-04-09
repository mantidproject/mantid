#include "PythonThreading.h"

#include "MantidVatesSimpleGuiViewWidgets/VatesParaViewApplication.h"
#include "MantidKernel/ConfigService.h"

#include "pqPVApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqStandardPropertyWidgetInterface.h"
#include "pqStandardViewFrameActionsImplementation.h"
#include "pqQtMessageHandlerBehavior.h"
#include "pqDataTimeStepBehavior.h"
#include "pqSpreadSheetVisibilityBehavior.h"
#include "pqPipelineContextMenuBehavior.h"
#include "pqObjectPickingBehavior.h"
#include "pqDefaultViewBehavior.h"
#include "pqUndoRedoBehavior.h"
#include "pqAlwaysConnectedBehavior.h"
#include "pqCrashRecoveryBehavior.h"
#include "pqAutoLoadPluginXMLBehavior.h"
#include "pqVerifyRequiredPluginBehavior.h"
#include "pqFixPathsInStateFilesBehavior.h"
#include "pqCommandLineOptionsBehavior.h"
#include "pqCollaborationBehavior.h"
#include "pqViewStreamingBehavior.h"
#include "pqPluginSettingsBehavior.h"

#include <Poco/Path.h>

#include <string>

#include "vtksys/SystemTools.hxx"

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      VatesParaViewApplication::VatesParaViewApplication() : m_logger("VatesParaViewApplication"), m_behaviorsSetup(false)
      {
        // Get the plugin path that we set in the ConfigService.
        auto & configSvc = Kernel::ConfigService::Instance();
        // This currently points at the top pvplugins directory and not the subdirectory.
        // Needs tidying up!!
        std::string pvPluginPathTop = configSvc.getPVPluginsPath();
        if (pvPluginPathTop.empty())
        {
          throw std::runtime_error("pvplugins.directory key not setup.\nVates plugins will not be available.\n"
                                   "Further use will cause the program to crash.\nPlease exit and "
                                   "set this variable.");
        }

        GlobalInterpreterLock gil;
        Q_ASSERT(pqApplicationCore::instance()==NULL);

        // Provide ParaView's application core with a path to the running executable
        int argc = 1;
        std::string exePath = configSvc.getDirectoryOfExecutable();
        std::vector<char> argvConversion(exePath.begin(), exePath.end());
        argvConversion.push_back('\0');
        char *argv[] = {&argvConversion[0]};

        m_logger.debug("Intialize pqApplicationCore with "+exePath+"\n");
        // We need to manually set the PV_PLUGIN_PATH because it's
        // not going to be picked up from the paraview/vtk side otherwise.
        Poco::Path pluginsDir(pvPluginPathTop, "pvplugins");
        m_logger.debug("Setting PV_PLUGIN_PATH="+pluginsDir.toString()+"\n");
        vtksys::SystemTools::PutEnv("PV_PLUGIN_PATH=" + pluginsDir.toString());
        new pqPVApplicationCore(argc, argv);
      }

      /**
       * This function duplicates the nearly identical call in ParaView for their
       * main program setup. This is necessary for the plugin mode since it does
       * not have access to the QMainWindow of MantidPlot.
       */
      void VatesParaViewApplication::setupParaViewBehaviors()
      {
        if (this->m_behaviorsSetup)
        {
          return;
        }
        this->m_behaviorsSetup = true;
        // Register ParaView interfaces.
        pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();

        // * adds support for standard paraview views.
        pgm->addInterface(new pqStandardPropertyWidgetInterface(pgm));

        pgm->addInterface(new pqStandardViewFrameActionsImplementation(pgm));

        // Load plugins distributed with application.
        pqApplicationCore::instance()->loadDistributedPlugins();

        // Define application behaviors.
        new pqQtMessageHandlerBehavior(this);
        new pqDataTimeStepBehavior(this);
        new pqSpreadSheetVisibilityBehavior(this);
        new pqPipelineContextMenuBehavior(this);
        new pqObjectPickingBehavior(this);
        new pqDefaultViewBehavior(this);
        new pqUndoRedoBehavior(this);
        new pqAlwaysConnectedBehavior(this);
        new pqCrashRecoveryBehavior(this);
        new pqAutoLoadPluginXMLBehavior(this);
        //new pqPluginDockWidgetsBehavior(mainWindow);
        new pqVerifyRequiredPluginBehavior(this);
        //new pqPluginActionGroupBehavior(mainWindow);
        new pqFixPathsInStateFilesBehavior(this);
        new pqCommandLineOptionsBehavior(this);
        //new pqPersistentMainWindowStateBehavior(mainWindow);
        new pqCollaborationBehavior(this);
        new pqViewStreamingBehavior(this);
        new pqPluginSettingsBehavior(this);
      }

      VatesParaViewApplication::~VatesParaViewApplication()
      {

      }

      VatesParaViewApplication* VatesParaViewApplication::instance()
      {
        static QPointer<VatesParaViewApplication> arg;
        if (arg == NULL)
        {
          arg = new VatesParaViewApplication();
        }
        return arg;
      }

    } //SimpleGui
  } //Vates
} //Mantid
