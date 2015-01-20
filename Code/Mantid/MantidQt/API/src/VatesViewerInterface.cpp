#include "MantidQtAPI/VatesViewerInterface.h"

using namespace MantidQt::API;

VatesViewerInterface::VatesViewerInterface() : QWidget()
{
}

VatesViewerInterface::VatesViewerInterface(QWidget *parent) : QWidget(parent)
{
}

VatesViewerInterface::~VatesViewerInterface()
{
}

void VatesViewerInterface::setupPluginMode()
{
}

void VatesViewerInterface::renderWorkspace(QString workSpaceName, int workspaceType, std::string instrumentName)
{
  UNUSED_ARG(workSpaceName);
  UNUSED_ARG(workspaceType);
  UNUSED_ARG(instrumentName);
}

void VatesViewerInterface::shutdown()
{
}
