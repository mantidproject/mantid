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
/*** Upload a file to a remote compute resource

    Destination directory depends on the specified transaction ID.  See
   StartRemoteTransaction.
    Note that there are no workspaces associated with this algorithm.

    Input Properties:
    <UL>
    <LI> ComputeResource  - The name of the compute resource the file will be
   sent to </LI>
    <LI> TransactionID    - ID of the transaction this file belongs to.  See
   StartRemoteTransaction </LI>
    <LI> LocalFileName    - The name of the file to be uploaded.  This should be
   the full pathnam
      on the local filesystem. </LI>
    <LI> RemoteFileName   - The name to save the file as on the remote compute
   resource.  This is only
      name;  the actual path is determined by the compute resource. </LI>
    </UL>

    @author Ross Miller, ORNL
    @date 04/30/2013
    */

class DLLExport UploadRemoteFile : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Constructor
  UploadRemoteFile() { this->useAlgorithm("UploadRemoteFile", 2); }

  /// Algorithm's name
  const std::string name() const override { return "UploadRemoteFile"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Uploads a file to the specified compute resource."; }

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