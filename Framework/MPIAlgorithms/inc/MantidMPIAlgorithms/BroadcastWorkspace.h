// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidMPIAlgorithms/DllConfig.h"

namespace Mantid {
namespace MPIAlgorithms {
/** BroadcastWorkspace is used to copy a workspace from one process to all the
   others.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of input workspace. Need only exist for the
   broadcasting process.</LI>
    <LI> OutputWorkspace - The name of the output workspace that will be created
   in all processes.</LI>
    <LI> BroadcasterRank - The rank of the process holding the workspace to be
   broadcast (default: 0).</LI>
    </UL>
*/
class MANTID_MPIALGORITHMS_DLL BroadcastWorkspace final : public API::Algorithm {
public:
  /// Default Constructor
  BroadcastWorkspace() = default;
  /// Destructor
  ~BroadcastWorkspace() override = default;
  /// Algorithm's name
  const std::string name() const override { return "BroadcastWorkspace"; }
  /// Algorithm's version
  int version() const override { return 1; }
  /// @copydoc Algorithm::summary
  const std::string summary() const override { return "Copy a workspace from one process to all the others."; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MPI"; }

private:
  void init() override;
  void exec() override;
  // void execEvent(); TODO: Make event-aware? (might lead to transmission of
  // too much data)
};

} // namespace MPIAlgorithms
} // namespace Mantid
