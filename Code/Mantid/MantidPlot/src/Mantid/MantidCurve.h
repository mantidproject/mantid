#ifndef MANTIDPLOT_MANTIDCURVE_H
#define MANTIDPLOT_MANTIDCURVE_H

#include "../PlotCurve.h"
#include "MantidQtAPI/WorkspaceObserver.h"

class MantidCurve :public PlotCurve, public MantidQt::API::WorkspaceObserver
{
public:
  MantidCurve(const QString& wsName):PlotCurve(wsName), 
  WorkspaceObserver()
  {
  }
  MantidCurve():PlotCurve(), 
  WorkspaceObserver()
  {
  }
};

#endif

