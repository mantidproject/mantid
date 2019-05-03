// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflNexusMeasurementItemSource.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <boost/regex.hpp>
#include <sstream>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 */
ReflNexusMeasurementItemSource::ReflNexusMeasurementItemSource() {}

/** Destructor
 */
ReflNexusMeasurementItemSource::~ReflNexusMeasurementItemSource() {}

MeasurementItem
ReflNexusMeasurementItemSource::obtain(const std::string &definedPath,
                                       const std::string &fuzzyName) const {
  std::string filenameArg = fuzzyName;
  if (!definedPath.empty()) {
    Poco::File file(definedPath);
    try {
      if (file.exists() && file.isFile()) {
        // Load the exact path
        filenameArg = definedPath;
      }
    } catch (Poco::PathNotFoundException &) {
      /* Deliberately swallow the exception.
         Poco::File::exists throws for network drives
      */
    }
  }
  try {

    auto hostWorkspace =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    IAlgorithm_sptr algLoadRun =
        AlgorithmManager::Instance().create("LoadNexusLogs");
    algLoadRun->setChild(true);
    algLoadRun->setRethrows(true);
    algLoadRun->initialize();
    algLoadRun->setProperty("Filename", filenameArg);
    algLoadRun->setProperty("Workspace", hostWorkspace);
    algLoadRun->execute();

    const auto &logs = hostWorkspace->run();
    const std::string measurementItemId =
        logs.getPropertyValueAsType<std::string>("measurement_id");
    const std::string measurementItemSubId =
        logs.getPropertyValueAsType<std::string>("measurement_subid");
    const std::string measurementItemLabel =
        logs.getPropertyValueAsType<std::string>("measurement_label");
    const std::string measurementItemType =
        logs.getPropertyValueAsType<std::string>("measurement_type");
    std::string runNumber;
    try {
      runNumber = logs.getPropertyValueAsType<std::string>("run_number");
    } catch (Exception::NotFoundError &) {
      boost::regex re("([0-9]*)$");
      boost::smatch match;
      bool regex_res = boost::regex_search(fuzzyName, match, re);
      if (regex_res) {
        runNumber = match[0];
      }
    }
    std::string runTitle;
    try {
      runTitle = logs.getPropertyValueAsType<std::string>("run_title");
    } catch (Exception::NotFoundError &) {
      // OK, runTitle will be empty
    }

    double theta = -1.0;
    try {
      Property *prop = logs.getProperty("stheta");
      if (TimeSeriesProperty<double> *tsp =
              dynamic_cast<TimeSeriesProperty<double> *>(prop)) {
        theta = tsp->valuesAsVector().back();
      }
    } catch (Exception::NotFoundError &) {
    }

    return MeasurementItem(measurementItemId, measurementItemSubId,
                           measurementItemLabel, measurementItemType, theta,
                           runNumber, runTitle);

  } catch (std::invalid_argument &ex) {
    std::stringstream buffer;
    buffer << "Meta-data load attemped a load using: " << filenameArg << '\n';
    buffer << ex.what();
    const std::string message = buffer.str();
    return MeasurementItem::InvalidMeasurementItem(message);
  }
}

ReflNexusMeasurementItemSource *ReflNexusMeasurementItemSource::clone() const {
  return new ReflNexusMeasurementItemSource(*this);
}

} // namespace CustomInterfaces
} // namespace MantidQt
