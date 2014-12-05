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

void VatesViewerInterface::renderWorkspace(QString wsname, int wstype)
{
  UNUSED_ARG(wsname);
  UNUSED_ARG(wstype);
}

void VatesViewerInterface::shutdown()
{
}
