
#include "MantidQtCustomInterfaces/ReflNexusMeasurementSource.h"
#include <Poco/File.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflNexusMeasurementSource::ReflNexusMeasurementSource() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflNexusMeasurementSource::~ReflNexusMeasurementSource() {}

Measurement
ReflNexusMeasurementSource::obtain(const std::string &definedPath,
                                   const std::string &fuzzyName) const {
  std::string filenameArg = fuzzyName;
  if (!definedPath.empty()) {
    Poco::File file(definedPath);
    if (file.exists() && file.isFile()) {
      // Load the exact path
      filenameArg = definedPath;
    }
  }
  try {
    IAlgorithm_sptr algLoadRun =
        AlgorithmManager::Instance().create("LoadISISNexus");
    algLoadRun->setChild(true);
    algLoadRun->setRethrows(true);
    algLoadRun->initialize();
    algLoadRun->setProperty("Filename", filenameArg);
    algLoadRun->setPropertyValue("OutputWorkspace", "dummy");
    algLoadRun->execute();
    Workspace_sptr temp = algLoadRun->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    if (outWS.get() == NULL) {
      WorkspaceGroup_sptr tempGroup =
          boost::dynamic_pointer_cast<WorkspaceGroup>(temp);
      outWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(tempGroup->getItem(0));
    }
    auto run = outWS->run();
    const std::string measurementId =
        run.getPropertyValueAsType<std::string>("measurement_id");
    const std::string measurementSubId =
        run.getPropertyValueAsType<std::string>("measurement_subid");
    const std::string measurementLabel =
        run.getPropertyValueAsType<std::string>("measurement_label");
    const std::string measurementType =
        run.getPropertyValueAsType<std::string>("measurement_type");
    const std::string runNumber =
        run.getPropertyValueAsType<std::string>("run_number");

    double theta = -1.0;
    try {
      Property *prop = run.getProperty("stheta");
      if (TimeSeriesProperty<double> *tsp =
              dynamic_cast<TimeSeriesProperty<double> *>(prop)) {
        theta = tsp->valuesAsVector().back();
      }
    } catch (Exception::NotFoundError &) {
    }

    return Measurement(measurementId, measurementSubId, measurementLabel,
                       measurementType, theta, runNumber);

  } catch (std::runtime_error &ex) {
    std::stringstream buffer;
    buffer << "Meta-data load attemped a load using: " << filenameArg
           << std::endl;
    buffer << ex.what();
    const std::string message = buffer.str();
    return Measurement::InvalidMeasurement(message);
  }
}

ReflNexusMeasurementSource *ReflNexusMeasurementSource::clone() const {
  return new ReflNexusMeasurementSource(*this);
}

} // namespace CustomInterfaces
} // namespace MantidQt
