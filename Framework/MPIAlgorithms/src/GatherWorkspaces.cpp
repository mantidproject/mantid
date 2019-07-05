// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMPIAlgorithms/MPISerialization.h"
#include <boost/mpi.hpp>
#include <boost/version.hpp>

namespace mpi = boost::mpi;

namespace Mantid {
namespace MPIAlgorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Anonymous namespace for locally-used functors
namespace {

/// Functor used for computing the sum of the square values of a vector
// Used by the eplus templates below
template <class T> struct SumGaussError : public std::binary_function<T, T, T> {
  SumGaussError() {}
  /// Sums the arguments in quadrature
  inline T operator()(const T &l, const T &r) const {
    return std::sqrt(l * l + r * r);
  }
};

// Newer versions of boost::mpi::reduce (>=v1.55) will recognize std::vector
// as a collection of individual elements and operate on a per-element
// basis.  Older versions treat vectors as a single object.  Thus we need
// two different versions of the sum operators that we pass into reduce().
// This is explained in more detail at:
// http://stackoverflow.com/questions/28845847/custom-reduce-operation-in-boost-mpi
#ifndef BOOST_VERSION
#error BOOST_VERSION macro is not defined!
#endif
#if (BOOST_VERSION / 100 % 1000) >= 55 // is the boost version >= 1.55?

struct vplus : public std::plus<double> {};

struct eplus : public SumGaussError<double> {};

#else // older version of Boost that passes the entire MantidVec
// the operator

/// Sum for boostmpi MantidVec
struct vplus : public std::binary_function<MantidVec, MantidVec,
                                           MantidVec> { // functor for operator+
  MantidVec
  operator()(const MantidVec &_Left,
             const MantidVec &_Right) const { // apply operator+ to operands
    MantidVec v(_Left.size());
    std::transform(_Left.begin(), _Left.end(), _Right.begin(), v.begin(),
                   std::plus<double>());
    return (v);
  }
};

/// Sum for error for boostmpi MantidVec
struct eplus : public std::binary_function<MantidVec, MantidVec,
                                           MantidVec> { // functor for operator+
  MantidVec
  operator()(const MantidVec &_Left,
             const MantidVec &_Right) const { // apply operator+ to operands
    MantidVec v(_Left.size());
    std::transform(_Left.begin(), _Left.end(), _Right.begin(), v.begin(),
                   SumGaussError<double>());
    return (v);
  }
};

#endif // boost version
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GatherWorkspaces)

void GatherWorkspaces::init() {
  // Input workspace is optional, except for the root process
  if (mpi::communicator().rank())
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  else
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory));
  // Output is optional - only the root process will output a workspace
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "OutputWorkspace", "", Direction::Output, PropertyMode::Optional));
  declareProperty(
      "PreserveEvents", false,
      "Keep the output workspace as an EventWorkspace, if the "
      "input has events.\n"
      "If false, then the workspace gets converted to a "
      "Workspace2D histogram(default to save memory for reduced data)");
  std::vector<std::string> propOptions;
  propOptions.push_back("Add");
  // propOptions.push_back("Replace");
  propOptions.push_back("Append");
  declareProperty(
      "AccumulationMethod", "Append",
      boost::make_shared<StringListValidator>(propOptions),
      "Method to use for accumulating each chunk from mpi processorss.\n"
      " - Add: the processed chunk will be summed to the previous output "
      "(default).\n"
      //" - Replace: the processed chunk will replace the previous output.\n"
      " - Append: the spectra of the chunk will be appended to the output "
      "workspace, increasing its size.");
}

void GatherWorkspaces::exec() {
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world; // The communicator containing all processes

  inputWorkspace = getProperty("InputWorkspace");

  // Create a new communicator that includes only those processes that have an
  // input workspace
  const int haveWorkspace(inputWorkspace ? 1 : 0);
  included = world.split(haveWorkspace);

  // If the present process doesn't have an input workspace then its work is
  // done
  if (!haveWorkspace) {
    g_log.information("No input workspace on this process, so nothing to do.");
    return;
  }

  // Get the number of bins in each workspace and check they're all the same
  numBins = inputWorkspace->blocksize();
  std::vector<std::size_t> all_numBins;
  all_gather(included, numBins, all_numBins);
  if (std::count(all_numBins.begin(), all_numBins.end(), numBins) !=
      (int)all_numBins.size()) {
    // All the processes will error out if all the workspaces don't have the
    // same number of bins
    throw Exception::MisMatch<std::size_t>(
        numBins, 0, "All input workspaces must have the same number of bins");
  }
  // Also check that all workspaces are either histogram or not
  // N.B. boost mpi doesn't seem to like me using booleans in the all_gather
  hist = inputWorkspace->isHistogramData();
  std::vector<int> all_hist;
  all_gather(included, hist, all_hist);
  if (std::count(all_hist.begin(), all_hist.end(), hist) !=
      (int)all_hist.size()) {
    // All the processes will error out if we don't have either all histogram or
    // all point-data workspaces
    throw Exception::MisMatch<int>(
        hist, 0,
        "The input workspaces must be all histogram or all point data");
  }

  // How do we accumulate the data?
  std::string accum = this->getPropertyValue("AccumulationMethod");
  // Get the total number of spectra in the combined inputs
  totalSpec = inputWorkspace->getNumberHistograms();
  sumSpec = totalSpec;
  if (accum == "Append") {
    reduce(included, totalSpec, sumSpec, std::plus<std::size_t>(), 0);
  } else if (accum == "Add") {
    // barrier only helps when memory is too low for communication
    // included.barrier();
  }

  eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWorkspace);
  if (eventW != NULL) {
    if (getProperty("PreserveEvents")) {
      // Input workspace is an event workspace. Use the other exec method
      this->execEvent();
      return;
    }
  }

  // The root process needs to create a workspace of the appropriate size
  MatrixWorkspace_sptr outputWorkspace;
  if (included.rank() == 0) {
    g_log.debug() << "Total number of spectra is " << sumSpec << "\n";
    // Create the workspace for the output
    outputWorkspace = WorkspaceFactory::Instance().create(
        inputWorkspace, sumSpec, numBins + hist, numBins);
    setProperty("OutputWorkspace", outputWorkspace);
    ExperimentInfo_sptr inWS = inputWorkspace;
    outputWorkspace->copyExperimentInfoFrom(inWS.get());
  }

  for (size_t wi = 0; wi < totalSpec; wi++) {
    if (included.rank() == 0) {
      const auto &inSpec = inputWorkspace->getSpectrum(wi);
      if (accum == "Add") {
        outputWorkspace->dataX(wi) = inputWorkspace->readX(wi);
        reduce(included, inputWorkspace->readY(wi), outputWorkspace->dataY(wi),
               vplus(), 0);
        reduce(included, inputWorkspace->readE(wi), outputWorkspace->dataE(wi),
               eplus(), 0);
      } else if (accum == "Append") {
        // Copy over data from own input workspace
        outputWorkspace->dataX(wi) = inputWorkspace->readX(wi);
        outputWorkspace->dataY(wi) = inputWorkspace->readY(wi);
        outputWorkspace->dataE(wi) = inputWorkspace->readE(wi);

        const int numReqs(3 * (included.size() - 1));
        std::vector<boost::mpi::request> reqs(numReqs);
        int j(0);

        // Receive data from all the other processes
        // This works because the process ranks are ordered the same in
        // 'included' as
        // they are in 'world', but in general this is not guaranteed. TODO:
        // robustify
        for (int i = 1; i < included.size(); ++i) {
          size_t index = wi + i * totalSpec;
          reqs[j++] = included.irecv(i, 0, outputWorkspace->dataX(index));
          reqs[j++] = included.irecv(i, 1, outputWorkspace->dataY(index));
          reqs[j++] = included.irecv(i, 2, outputWorkspace->dataE(index));
          auto &outSpec = outputWorkspace->getSpectrum(index);
          outSpec.clearDetectorIDs();
          outSpec.addDetectorIDs(inSpec.getDetectorIDs());
        }

        // Make sure everything's been received before exiting the algorithm
        mpi::wait_all(reqs.begin(), reqs.end());
      }
      auto &outSpec = outputWorkspace->getSpectrum(wi);
      outSpec.clearDetectorIDs();
      outSpec.addDetectorIDs(inSpec.getDetectorIDs());
    } else {
      if (accum == "Add") {
        reduce(included, inputWorkspace->readY(wi), vplus(), 0);
        reduce(included, inputWorkspace->readE(wi), eplus(), 0);
      } else if (accum == "Append") {
        std::vector<boost::mpi::request> reqs(3);

        // Send the spectrum to the root process
        reqs[0] = included.isend(0, 0, inputWorkspace->readX(0));
        reqs[1] = included.isend(0, 1, inputWorkspace->readY(0));
        reqs[2] = included.isend(0, 2, inputWorkspace->readE(0));

        // Make sure the sends have completed before exiting the algorithm
        mpi::wait_all(reqs.begin(), reqs.end());
      }
    }
  }
}
void GatherWorkspaces::execEvent() {

  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator included; // The communicator containing all processes
  // The root process needs to create a workspace of the appropriate size
  EventWorkspace_sptr outputWorkspace;
  if (included.rank() == 0) {
    g_log.debug() << "Total number of spectra is " << totalSpec << "\n";
    // Create the workspace for the output
    outputWorkspace = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", sumSpec,
                                                 numBins + hist, numBins));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(
        *eventW, *outputWorkspace, true);
    setProperty("OutputWorkspace", outputWorkspace);
    ExperimentInfo_sptr inWS = inputWorkspace;
    outputWorkspace->copyExperimentInfoFrom(inWS.get());
  }

  for (size_t wi = 0; wi < totalSpec; wi++) {
    if (included.rank() == 0) {
      // How do we accumulate the data?
      std::string accum = this->getPropertyValue("AccumulationMethod");
      std::vector<Mantid::DataObjects::EventList> out_values;
      gather(included, eventW->getSpectrum(wi), out_values, 0);
      for (int i = 0; i < included.size(); i++) {
        size_t index = wi; // accum == "Add"
        if (accum == "Append")
          index = wi + i * totalSpec;
        outputWorkspace->dataX(index) = eventW->readX(wi);
        outputWorkspace->getSpectrum(index) += out_values[i];
        const auto &inSpec = eventW->getSpectrum(wi);
        auto &outSpec = outputWorkspace->getSpectrum(index);
        outSpec.clearDetectorIDs();
        outSpec.addDetectorIDs(inSpec.getDetectorIDs());
      }
    } else {
      gather(included, eventW->getSpectrum(wi), 0);
    }
  }
}

} // namespace MPIAlgorithms
} // namespace Mantid
