#include "MantidQtAPI/VatesViewerInterface.h"
#include "MantidQtAPI/InterfaceManager.h"

using namespace MantidQt::API;

VatesViewerInterface::VatesViewerInterface() : QWidget() {}

VatesViewerInterface::VatesViewerInterface(QWidget *parent) : QWidget(parent) {}

VatesViewerInterface::~VatesViewerInterface() {}

void VatesViewerInterface::setupPluginMode(
    int /*WsType*/, const std::string & /*instrumentName*/) {}

void VatesViewerInterface::renderWorkspace(QString workSpaceName,
                                           int workspaceType,
                                           std::string instrumentName) {
  UNUSED_ARG(workSpaceName);
  UNUSED_ARG(workspaceType);
  UNUSED_ARG(instrumentName);
}

/** Load a Vates window from a Mantid project file
 *
 * This method simple provides a way to instantiate the correct window via
 * the interface manager.
 *
 * @param lines :: the lines containing the state of the window
 * @param app :: handle to the main application window
 * @param fileVersion :: version of the Mantid project file.
 * @return a handle to the newly created Vates window
 */
IProjectSerialisable *VatesViewerInterface::loadFromProject(
    const std::string &lines, ApplicationWindow *app, const int fileVersion) {
  UNUSED_ARG(app);
  UNUSED_ARG(fileVersion);

  MantidQt::API::InterfaceManager interfaceManager;
  auto *vsui = interfaceManager.createVatesSimpleGui();

  if (!vsui)
    return nullptr;

  vsui->loadFromProject(lines);
  return vsui;
}

void VatesViewerInterface::shutdown() {}
