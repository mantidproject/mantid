// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPlotUtilities.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"

using namespace MantidQt::MantidWidgets;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_const_sptr;
using Mantid::API::WorkspaceGroup_sptr;

/**Compare two CurveSpecs to sort according to log value and
 * if equal by workspace index.
 * @param lhs left hand comparee
 * @param rhs right hand comparee
 * @returns true if right hand comparee has greater log value than left hand
 * comparee
 */
bool byLogValue(const CurveSpec &lhs, const CurveSpec &rhs) {
  if (lhs.logVal == rhs.logVal)
    return (lhs.index < rhs.index);
  return (lhs.logVal < rhs.logVal);
}

/**
 * Gets the given log value from the given workspace as a double.
 * Should be a single-valued log!
 * @param wsIndex :: [input] Index of workspace in group
 * @param matrixWS :: [input] Workspace to find log from
 * @param logName :: [input] Name of log
 * @returns log value as a double, or workspace index
 * @throws invalid_argument if log is wrong type or not present
 */
double getSingleWorkspaceLogValue(
    size_t wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName) {
  if (logName == MantidWSIndexWidget::WORKSPACE_INDEX || logName == "")
    return static_cast<double>(wsIndex); // cast for plotting

  // MatrixWorkspace is an ExperimentInfo
  return matrixWS->run().getLogAsSingleValue(
      logName.toStdString(), Mantid::Kernel::Math::TimeAveragedMean);
}

/**
 * Gets the custom, user-provided log value of the given index.
 * i.e. the nth in order from smallest to largest.
 * If the index is outside the range, returns 0.
 * @param wsIndex :: [input] Index of log value to use
 * @param logValues :: [input] User-provided set of log values
 * @returns Numeric log value
 */
double getSingleWorkspaceLogValue(size_t wsIndex,
                                  const std::set<double> &logValues) {
  if (wsIndex >= logValues.size())
    return 0;

  auto it = logValues.begin();
  std::advance(it, wsIndex);
  return *it;
}
