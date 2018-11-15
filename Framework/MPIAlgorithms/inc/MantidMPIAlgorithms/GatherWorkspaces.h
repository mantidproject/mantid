// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MPIALGORITHMS_GATHERWORKSPACES_H_
#define MANTID_MPIALGORITHMS_GATHERWORKSPACES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;

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
class GatherWorkspaces : public API::Algorithm {
public:
  /// (Empty) Constructor
  GatherWorkspaces() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~GatherWorkspaces() {}
  /// Algorithm's name
  virtual const std::string name() const { return "GatherWorkspaces"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// @copydoc Algorithm::summary
  virtual const std::string summary() const {
    return "Stitches together the input workspaces provided by each of the "
           "processes into a single workspace.";
  }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MPI"; }

private:
  void init();
  void exec();

  void execEvent();
  API::MatrixWorkspace_sptr inputWorkspace;
  DataObjects::EventWorkspace_const_sptr eventW;
  std::size_t totalSpec;
  std::size_t sumSpec;
  int hist;
  std::size_t numBins;
  mpi::communicator included;
};

} // namespace MPIAlgorithms
} // namespace Mantid

#endif /*MANTID_MPIALGORITHMS_GATHERWORKSPACES_H_*/
