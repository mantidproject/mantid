#ifndef MANTIDPLOTUTILITIES_H_
#define MANTIDPLOTUTILITIES_H_

#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"

/**
 * These utilities assist with plotting in Mantid
 */
/// Structure to aid ordering of plots
struct CurveSpec {
  double logVal;
  QString wsName;
  int index;
};

/// Compare to sort according to log value
bool byLogValue(const CurveSpec &lhs, const CurveSpec &rhs);

/// Returns a single log value from the given workspace
double getSingleWorkspaceLogValue(
    size_t wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName);

/// Returns a single custom log value
double getSingleWorkspaceLogValue(size_t wsIndex,
                                  const std::set<double> &logValues);

#endif // MANTIDPLOTUTILITIES_H_
