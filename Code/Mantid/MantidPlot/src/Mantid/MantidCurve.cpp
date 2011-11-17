#include "MantidCurve.h"

/**
Constructor
@param wsName : Name of the workspace
*/
MantidCurve::MantidCurve(const QString& wsName) : PlotCurve(wsName), WorkspaceObserver()
{
}

/**
Constructor default
*/
MantidCurve::MantidCurve() : PlotCurve(), WorkspaceObserver()
{
}

/// Destructor
MantidCurve::~MantidCurve()
{
}

