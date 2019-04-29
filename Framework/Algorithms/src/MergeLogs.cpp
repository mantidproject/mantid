// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MergeLogs.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(MergeLogs)

void MergeLogs::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "Workspace to have logs merged");
  declareProperty("LogName1", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the first log to be merged.");
  declareProperty("LogName2", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the second log to be merged.");
  declareProperty("MergedLogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the new log as the result "
                  "of log 1 being merged with log 2.");
  declareProperty("ResetLogValue", false,
                  "Reset both logs' values to unity for each one.");
  declareProperty("LogValue1", 0.0, "Unity value of log 1.");
  declareProperty("LogValue2", 1.0, "Unity value of log 2.");
}

/** Helper to validate the TimeSeriesProperty.
 * propertyName :: Algorithm's name of the TimeSeriesProperty
 * @return The issue
 */
std::string MergeLogs::validateTSP(std::string const &propertyName) {
  std::string const logName = this->getProperty(propertyName);
  MatrixWorkspace_const_sptr ws = this->getProperty("Workspace");
  if (!this->getPointerToProperty(propertyName)->isDefault() &&
      ws->run().hasProperty(logName)) {
    try {
      ws->run().getTimeSeriesProperty<double>(logName);
    } catch (std::invalid_argument &) {
      return "Must be a TimeSeriesProperty";
    }
  } else
    return "TimeSeriesLog must exist.";
  return "";
}

/** Validate the algorithm's properties.
 * @return A map of property names and their issues.
 */
std::map<std::string, std::string> MergeLogs::validateInputs(void) {
  std::map<std::string, std::string> issues;
  std::string const mlogname = this->getProperty("MergedLogName");
  std::string const logName1 = this->getProperty("LogName1");
  std::string const logName2 = this->getProperty("LogName2");
  MatrixWorkspace_const_sptr ws = this->getProperty("Workspace");
  if ((mlogname == logName1) || (mlogname == logName2) ||
      ws->run().hasProperty(mlogname))
    issues["MergedLogName"] = "TimeSeriesLog name must be unique.";
  else {
    std::string const &issueLog1 = this->validateTSP("LogName1");
    if (!issueLog1.empty())
      issues["LogName1"] = issueLog1;
    std::string const &issueLog2 = this->validateTSP("LogName2");
    if (!issueLog2.empty())
      issues["LogName2"] = issueLog2;
  }
  return issues;
}

void MergeLogs::exec() {
  MatrixWorkspace_sptr ws = this->getProperty("Workspace");
  const std::string log1name = this->getProperty("LogName1");
  const std::string log2name = this->getProperty("LogName2");
  const std::string mlogname = this->getProperty("MergedLogName");
  const bool resetlogvalue = this->getProperty("ResetLogValue");
  TimeSeriesProperty<double> *log1(
      ws->run().getTimeSeriesProperty<double>(log1name));
  TimeSeriesProperty<double> *log2(
      ws->run().getTimeSeriesProperty<double>(log2name));
  std::unique_ptr<TimeSeriesProperty<double>> mlog1(log1->clone());
  std::unique_ptr<TimeSeriesProperty<double>> mlog2(log2->clone());
  mlog1->setName(mlogname);
  mlog2->setName(mlogname);
  if (resetlogvalue) {
    const double logvalue1 = this->getProperty("LogValue1");
    const double logvalue2 = this->getProperty("LogValue2");
    const std::vector<double> logvector1(mlog1->size(), logvalue1);
    const std::vector<double> logvector2(mlog2->size(), logvalue2);
    mlog1->replaceValues(log1->timesAsVector(), logvector1);
    mlog2->replaceValues(log2->timesAsVector(), logvector2);
  }
  mlog1->merge(mlog2.get());
  ws->mutableRun().addProperty(std::move(mlog1));
  setProperty("Workspace", ws);
}

} // namespace Algorithms
} // namespace Mantid
