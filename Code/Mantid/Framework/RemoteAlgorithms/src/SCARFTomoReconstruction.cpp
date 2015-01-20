#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SCARFTomoReconstruction)

using namespace Mantid::Kernel;

void SCARFTomoReconstruction::init() {
  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  std::vector<std::string> reconstOps;
  reconstOps.push_back("CreateJob");
  reconstOps.push_back("JobStatus");
  reconstOps.push_back("JobCancel");
  auto listValue = boost::make_shared<StringListValidator>(reconstOps);

  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".*");

  // User
  declareProperty("UserName", "", requireValue,
                  "Name of the user to authenticate as", Direction::Input);

  // Password
  declareProperty(new MaskedProperty<std::string>("Password", "", requireValue,
                                                  Direction::Input),
                  "The password for the user");

  // Operation to perform : Update description as enum changes
  declareProperty("Operation", "", listValue, "Choose the operation to perform "
                                              "on SCARF; "
                                              "[CreateJob,JobStatus,JobCancel]",
                  Direction::Input);

  // NXTomo File path on SCARF
  declareProperty(new PropertyWithValue<std::string>("RemoteNXTomoPath", "",
                                                     Direction::Input),
                  "The path on SCARF to the NXTomo file to reconstruct");

  // Job ID on SCARF
  declareProperty(
      new PropertyWithValue<std::string>("JobID", "", Direction::Input),
      "The ID for a currently running job on SCARF");

  // Path to parameter file for reconstruction
  declareProperty(new API::FileProperty("ParameterFilePath", "",
                                        API::FileProperty::OptionalLoad, exts,
                                        Direction::Input),
                  "Parameter file for the reconstruction job");
}

void SCARFTomoReconstruction::exec() {
  try {
    m_userName = getPropertyValue("UserName");
    m_password = getPropertyValue("Password");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "To run this algorithm you need to give a valid SCARF "
      "username and password." << std::endl;
    throw;
  }
  m_operation = getPropertyValue("Operation");


  g_log.information("Running SCARFTomoReconstruction");

  if (m_operation == "CreateJob") {
    doCreate();
  } else if (m_operation == "JobStatus") {
    doStatus();
  } else if (m_operation == "JobCancel") {
    doCancel();
  }
}

void SCARFTomoReconstruction::doCreate() {
  progress(0, "Starting tomographic reconstruction job...");

  try {
    m_nxTomoPath = getPropertyValue("RemoteNXTomoPath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify the remote path to the NXTomo file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid path on the SCARF cluster" << std::endl;
    throw;
  }

  try {
    m_parameterPath = getPropertyValue("ParameterFilePath");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a the path to the parameter file "
      "which is required to create a new reconstruction job. Please provide "
      "a valid tomography reconstruction parameter file" << std::endl;
    throw;
  }

  // TODO: create
  // TODO: handle failure

  progress(1.0, "Job created.");
}

void SCARFTomoReconstruction::doStatus() {
  try {
    m_jobID = getPropertyValue("JobID");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a JobID which is required "
      "to query its status." << std::endl;
    throw;
  }

  progress(0, "Starting tomographic reconstruction job...");

  // TODO: query about jobID and report

  progress(1.0, "Job created.");
}

void SCARFTomoReconstruction::doCancel() {
  try {
    m_jobID = getPropertyValue("JobID");
  } catch(std::runtime_error& /*e*/) {
    g_log.error() << "You did not specify a JobID which is required "
      "to cancel a job." << std::endl;
    throw;
  }

  progress(0, "Cancelling tomographic reconstruction job...");

  // TODO: query+cancel jobID, and report result
  // TODO: handle failure

  progress(1.0, "Job cancelled.");
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
