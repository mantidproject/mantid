#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include <boost/assign/std/vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SCARFTomoReconstruction)

using namespace boost::assign;
using namespace Mantid::Kernel;

void SCARFTomoReconstruction::init() {
  auto requireValue = boost::make_shared<MandatoryValidator<std::string>>();

  std::vector<std::string> reconstructionOps;
  reconstructionOps += "CreateJob", "JobStatus", "JobDelete";
  auto listValue = boost::make_shared<StringListValidator>(reconstructionOps);

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
                                              "[CreateJob,JobStatus,JobDelete]",
                  Direction::Input),

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
  m_userName = getProperty("UserName");
  m_password = getProperty("Password");
  m_operation = getProperty("Operation");
  m_nxTomoPath = getProperty("RemoteNXTomoPath");
  m_jobID = getProperty("JobID");
  m_parameterPath = getProperty("ParameterFilePath");

  if (m_operation == "CreateJob") {

  } else if (m_operation == "JobStatus") {

  } else if (m_operation == "JobDelete") {
  }

  g_log.information("Run SCARFTomoReconstruction");
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
