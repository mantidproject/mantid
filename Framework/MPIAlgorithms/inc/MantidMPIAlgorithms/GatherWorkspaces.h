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
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMPIAlgorithms/DllConfig.h"

#ifdef MPI_BUILD
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;
#endif

namespace Mantid {
namespace MPIAlgorithms {
/** GatherWorkspaces be viewed as ConjoinWorkspaces for MPI.
    It stitches together the input workspaces provided by each of the processes
   into
    a single workspace in the root process.

    The spectra in the output workspace will be ordered by the rank of the input
   processes.
    It is up to the caller to ensure this results in the required ordering.
    Furthermore, there are all sorts of things that ought to be consistent for
   this
    algorithm to make sense (e.g. the instrument). The general philosophy,
   though, is to
    leave the responsibility for this to the user and only check the vital
   things (i.e. that
    the number of bins is consistent).

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the separate workspaces (must be the same
   for all processes).</LI>
    <LI> OutputWorkspace - The name of the output workspace. Will only be
   created by the root process.</LI>
    </UL>
*/
class MANTID_MPIALGORITHMS_DLL GatherWorkspaces final : public API::Algorithm {
public:
  /// Default Constructor
  GatherWorkspaces() = default;
  /// Destructor
  ~GatherWorkspaces() override = default;
  /// Algorithm's name
  const std::string name() const override { return "GatherWorkspaces"; }
  /// Algorithm's version
  int version() const override { return 1; }
  /// @copydoc Algorithm::summary
  const std::string summary() const override {
    return "Stitches together the input workspaces provided by each of the "
           "processes into a single workspace.";
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "MPI"; }

private:
  void init() override;
  void exec() override;

  /// Execute in Add mode with chunked processing
  void execAddChunked(API::MatrixWorkspace_sptr &outputWorkspace, std::size_t chunkSize);
  /// Execute in Append mode with chunked processing
  void execAppendChunked(API::MatrixWorkspace_sptr &outputWorkspace, std::size_t chunkSize);
  /// Execute for EventWorkspaces
  void execEvent();

  API::MatrixWorkspace_sptr m_inputWorkspace;
  DataObjects::EventWorkspace_const_sptr m_eventW;
  std::size_t m_totalSpec{0};
  std::size_t m_sumSpec{0};
  int m_hist{0};
  std::size_t m_numBins{0};
#ifdef MPI_BUILD
  mpi::communicator m_included;
#endif
};

} // namespace MPIAlgorithms
} // namespace Mantid
