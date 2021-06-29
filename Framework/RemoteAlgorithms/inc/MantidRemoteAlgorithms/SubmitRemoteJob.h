// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {
/*** Submit a job to be executed on the remote compute resource.

    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource that will execute
   the job </LI>
    <LI> NumNodes         - The number of nodes to reserve for this job </LI>
    <LI> CoresPerNode     - The number of cores this job will use on each node
   </LI>
    <LI> TaskName         - A short, human readable identifier for the job
   (Optional) </LI>
    <LI> TransactionID    - ID of the transaction this job belongs to.  See
   StartRemoteTransaction </LI>
    <LI> PythonScript     - The actual python code that will be executed </LI>
    <LI> ScriptName       - A name for the python script </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> JobID            - An ID for tracking the status of the submitted job
   (Queued, Running,
      Completed, Error, etc..) </LI>
    <\UL>

    @author Ross Miller, ORNL
    @date 04/30/2013
    */

class DLLExport SubmitRemoteJob : public Mantid::API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Constructor
  SubmitRemoteJob() { this->useAlgorithm("SubmitRemoteJob", 2); }

  /// Algorithm's name
  const std::string name() const override { return "SubmitRemoteJob"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Submit a job to be executed on the specified remote compute "
           "resource.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Remote"; }

private:
  void init() override;
  /// Execution code
  void exec() override;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid