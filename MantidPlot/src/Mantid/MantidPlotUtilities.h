#ifndef MANTIDPLOTUTILITIES_H_
#define MANTIDPLOTUTILITIES_H_

#include "MantidQtMantidWidgets/MantidWSIndexDialog.h"

/**
* This utility class generates a surface or contour plot from a group of
* workspaces.
*/

/// Returns a single log value from the given workspace
double getSingleWorkspaceLogValue(
    int wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName);

/// Returns a single custom log value
double getSingleWorkspaceLogValue(int wsIndex,
                                  const std::set<double> &logValues);

#endif // MANTIDPLOTUTILITIES_H_
