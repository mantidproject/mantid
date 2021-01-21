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
class BroadcastWorkspace : public API::Algorithm {
public:
  /// (Empty) Constructor
  BroadcastWorkspace() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~BroadcastWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const { return "BroadcastWorkspace"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// @copydoc Algorithm::summary
  virtual const std::string summary() const { return "Copy a workspace from one process to all the others."; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MPI"; }

private:
  void init();
  void exec();
  // void execEvent(); TODO: Make event-aware? (might lead to transmission of
  // too much data)
};

} // namespace MPIAlgorithms
} // namespace Mantid
