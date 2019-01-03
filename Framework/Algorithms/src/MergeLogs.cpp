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
using Mantid::Kernel::MandatoryValidator;

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

void MergeLogs::exec() {
  MatrixWorkspace_sptr ws = this->getProperty("Workspace");
  MatrixWorkspace_sptr ws2 = ws->clone();
  const std::string log1name = this->getProperty("LogName1");
  const std::string log2name = this->getProperty("LogName2");
  const std::string mlogname = this->getProperty("MergedLogName");
  if ((mlogname == log1name) || (mlogname == log2name))
    throw std::runtime_error("MergedLogName must be a unique name.");
  const bool resetlogvalue = this->getProperty("ResetLogValue");
  try {
    TimeSeriesProperty<double> *log1 =
        ws->run().getTimeSeriesProperty<double>(log1name);
    TimeSeriesProperty<double> *log2 =
        ws->run().getTimeSeriesProperty<double>(log2name);
    TimeSeriesProperty<double> *mlog1 = log1->clone();
    TimeSeriesProperty<double> *mlog2 = log2->clone();
    mlog1->setName(mlogname);
    mlog2->setName(mlogname);
    if (resetlogvalue) {
      const double logvalue1 = this->getProperty("LogValue1");
      const double logvalue2 = this->getProperty("LogValue2");
      std::vector<double> logvector1(mlog1->size(), logvalue1);
      std::vector<double> logvector2(mlog2->size(), logvalue2);
      mlog1->replaceValues(log1->timesAsVector(), logvector1);
      mlog2->replaceValues(log2->timesAsVector(), logvector2);
    }
    ws->mutableRun().addProperty(mlog1);
    ws2->mutableRun().addProperty(mlog2);
    // Time Series Logs are combined when adding workspaces
    ws += ws2;
    ws -= ws2;
  } catch (std::invalid_argument &) {
    throw std::runtime_error(
        "LogName1 and LogName2 must be TimeSeriesProperties.");
  }
}

} // namespace Algorithms
} // namespace Mantid
