#include "MantidPlotUtilities.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <MantidQtMantidWidgets/MantidDisplayBase.h>

using namespace MantidQt::MantidWidgets;
using Mantid::API::WorkspaceGroup_const_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::ExperimentInfo;
using Mantid::HistogramData::Histogram;



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
    int wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName) {
  if (logName == MantidWSIndexWidget::WORKSPACE_INDEX) {
    return wsIndex;
  } else {
    // MatrixWorkspace is an ExperimentInfo
    if (auto ei = boost::dynamic_pointer_cast<const ExperimentInfo>(matrixWS)) {
      auto log = ei->run().getLogData(logName.toStdString());
      if (log) {
        if (dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(log) ||
            dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(log)) {
          return std::stod(log->value());
        } else {
          throw std::invalid_argument(
              "Log is of wrong type (expected single numeric value");
        }
      } else {
        throw std::invalid_argument("Log not present in workspace");
      }
    } else {
      throw std::invalid_argument("Bad input workspace type");
    }
  }
}

