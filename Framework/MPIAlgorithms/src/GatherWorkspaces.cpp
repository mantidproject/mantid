// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidMPIAlgorithms/Chunking.h"
#include "MantidMPIAlgorithms/MPISerialization.h"
#include <algorithm>
#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <functional>
#include <numeric>

namespace mpi = boost::mpi;

namespace Mantid::MPIAlgorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GatherWorkspaces)

void GatherWorkspaces::init() {
  // Input workspace is optional, except for the root process
  if (mpi::communicator().rank())
    declareProperty(
        std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  else
    declareProperty(
        std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Mandatory));

  // Output is optional - only the root process will output a workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output, PropertyMode::Optional));

  declareProperty("PreserveEvents", false,
                  "Keep the output workspace as an EventWorkspace, if the "
                  "input has events.\n"
                  "If false, then the workspace gets converted to a "
                  "Workspace2D histogram (default to save memory for reduced data)");

  std::vector<std::string> propOptions{"Add", "Append"};
  declareProperty("AccumulationMethod", "Append", std::make_shared<StringListValidator>(propOptions),
                  "Method to use for accumulating each chunk from MPI processors.\n"
                  " - Add: the processed chunk will be summed to the previous output.\n"
                  " - Append: the spectra of the chunk will be appended to the output "
                  "workspace, increasing its size.");

  declareProperty("ChunkSize", 0, std::make_shared<BoundedValidator<int>>(-1, INT_MAX),
                  "Number of spectra to process at a time. Use 0 for automatic chunk sizing "
                  "(recommended, targets ~100MB chunks), or -1 to process all spectra at once "
                  "(fastest but uses most memory). A positive value specifies exact number of spectra per chunk.");
}

void GatherWorkspaces::exec() {
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world;

  m_inputWorkspace = getProperty("InputWorkspace");

  // Create a new communicator that includes only those processes that have an
  // input workspace
  const int haveWorkspace(m_inputWorkspace ? 1 : 0);
  m_included = world.split(haveWorkspace);

  // If the present process doesn't have an input workspace then its work is done
  if (!haveWorkspace) {
    g_log.information("No input workspace on this process, so nothing to do.");
    return;
  }

  // Get the number of bins in each workspace and check they're all the same
  m_numBins = m_inputWorkspace->blocksize();
  std::vector<std::size_t> all_numBins;
  all_gather(m_included, m_numBins, all_numBins);
  if (std::count(all_numBins.begin(), all_numBins.end(), m_numBins) != static_cast<long>(all_numBins.size())) {
    throw Exception::MisMatch<std::size_t>(m_numBins, 0, "All input workspaces must have the same number of bins");
  }

  // Also check that all workspaces are either histogram or not
  m_hist = m_inputWorkspace->isHistogramData();
  std::vector<int> all_hist;
  all_gather(m_included, m_hist, all_hist);
  if (std::count(all_hist.begin(), all_hist.end(), m_hist) != static_cast<long>(all_hist.size())) {
    throw Exception::MisMatch<int>(m_hist, 0, "The input workspaces must be all histogram or all point data");
  }

  // How do we accumulate the data?
  const std::string accum = this->getPropertyValue("AccumulationMethod");
  const int requestedChunkSize = getProperty("ChunkSize");

  // Get the total number of spectra in the combined inputs
  m_totalSpec = m_inputWorkspace->getNumberHistograms();
  m_sumSpec = m_totalSpec;
  if (accum == "Append") {
    reduce(m_included, m_totalSpec, m_sumSpec, std::plus<std::size_t>(), 0);
  }

  // Check for EventWorkspace
  m_eventW = std::dynamic_pointer_cast<const EventWorkspace>(m_inputWorkspace);
  if (m_eventW) {
    if (getProperty("PreserveEvents")) {
      this->execEvent();
      return;
    }
  }

  // // Calculate chunk size
  std::size_t chunkSize = Mantid::MPIAlgorithms::Chunking::chooseChunkSize(requestedChunkSize, m_totalSpec, m_numBins);

  // The root process needs to create a workspace of the appropriate size
  MatrixWorkspace_sptr outputWorkspace;
  if (m_included.rank() == 0) {
    g_log.debug() << "Total number of spectra is " << m_sumSpec << "\n";
    outputWorkspace = WorkspaceFactory::Instance().create(m_inputWorkspace, m_sumSpec, m_numBins + m_hist, m_numBins);
    setProperty("OutputWorkspace", outputWorkspace);
    ExperimentInfo_sptr inWS = m_inputWorkspace;
    outputWorkspace->copyExperimentInfoFrom(inWS.get());
  }

  if (accum == "Add") {
    execAddChunked(outputWorkspace, chunkSize);
  } else {
    execAppendChunked(outputWorkspace, chunkSize);
  }
}

void GatherWorkspaces::execAddChunked(MatrixWorkspace_sptr &outputWorkspace, std::size_t chunkSize) {
  // In Add mode, sum Y values and add errors in quadrature from all processes

  for (std::size_t chunkStart = 0; chunkStart < m_totalSpec; chunkStart += chunkSize) {
    const std::size_t chunkEnd = std::min(chunkStart + chunkSize, m_totalSpec);
    const std::size_t spectraInChunk = chunkEnd - chunkStart;

    // Pack local data into vectors
    std::vector<double> localY(spectraInChunk * m_numBins);
    std::vector<double> localE(spectraInChunk * m_numBins);

    for (std::size_t i = 0; i < spectraInChunk; ++i) {
      const std::size_t specIndex = chunkStart + i;
      const auto &y = m_inputWorkspace->y(specIndex);
      const auto &e = m_inputWorkspace->e(specIndex);
      std::copy(y.cbegin(), y.cend(), localY.begin() + i * m_numBins);
      std::copy(e.cbegin(), e.cend(), localE.begin() + i * m_numBins);
    }

    if (m_included.rank() == 0) {
      // Root receives and reduces
      std::vector<double> sumY(spectraInChunk * m_numBins, 0.0);
      std::vector<double> sumE(spectraInChunk * m_numBins, 0.0);

      reduce(m_included, localY, sumY, std::plus<double>(), 0);
      reduce(m_included, localE, sumE, [](double a, double b) { return std::sqrt(a * a + b * b); }, 0);

      // Copy X from input and reduced Y, E to output
      for (std::size_t i = 0; i < spectraInChunk; ++i) {
        const std::size_t specIndex = chunkStart + i;

        outputWorkspace->setSharedX(specIndex, m_inputWorkspace->sharedX(specIndex));

        auto &outY = outputWorkspace->mutableY(specIndex);
        auto &outE = outputWorkspace->mutableE(specIndex);
        std::copy(sumY.begin() + i * m_numBins, sumY.begin() + (i + 1) * m_numBins, outY.begin());
        std::copy(sumE.begin() + i * m_numBins, sumE.begin() + (i + 1) * m_numBins, outE.begin());

        // Copy detector IDs
        const auto &inSpec = m_inputWorkspace->getSpectrum(specIndex);
        auto &outSpec = outputWorkspace->getSpectrum(specIndex);
        outSpec.clearDetectorIDs();
        outSpec.addDetectorIDs(inSpec.getDetectorIDs());
      }
    } else {
      // Non-root processes just contribute to reduce
      reduce(m_included, localY, std::plus<double>(), 0);
      reduce(m_included, localE, [](double a, double b) { return std::sqrt(a * a + b * b); }, 0);
    }
  }
}

void GatherWorkspaces::execAppendChunked(MatrixWorkspace_sptr &outputWorkspace, std::size_t chunkSize) {
  // In Append mode, concatenate spectra from all processes

  for (std::size_t chunkStart = 0; chunkStart < m_totalSpec; chunkStart += chunkSize) {
    const std::size_t chunkEnd = std::min(chunkStart + chunkSize, m_totalSpec);
    const std::size_t spectraInChunk = chunkEnd - chunkStart;
    const std::size_t xSize = m_numBins + m_hist;

    // Pack local data
    std::vector<double> localX(spectraInChunk * xSize);
    std::vector<double> localY(spectraInChunk * m_numBins);
    std::vector<double> localE(spectraInChunk * m_numBins);

    for (std::size_t i = 0; i < spectraInChunk; ++i) {
      const std::size_t specIndex = chunkStart + i;
      const auto &x = m_inputWorkspace->x(specIndex);
      const auto &y = m_inputWorkspace->y(specIndex);
      const auto &e = m_inputWorkspace->e(specIndex);
      std::copy(x.cbegin(), x.cend(), localX.begin() + i * xSize);
      std::copy(y.cbegin(), y.cend(), localY.begin() + i * m_numBins);
      std::copy(e.cbegin(), e.cend(), localE.begin() + i * m_numBins);
    }

    if (m_included.rank() == 0) {
      // Gather data from all processes
      std::vector<std::vector<double>> allX, allY, allE;
      gather(m_included, localX, allX, 0);
      gather(m_included, localY, allY, 0);
      gather(m_included, localE, allE, 0);

      // Unpack into output workspace
      for (int rank = 0; rank < m_included.size(); ++rank) {
        for (std::size_t i = 0; i < spectraInChunk; ++i) {
          const std::size_t inputSpecIndex = chunkStart + i;
          const std::size_t outputSpecIndex = inputSpecIndex + rank * m_totalSpec;

          auto &outX = outputWorkspace->mutableX(outputSpecIndex);
          auto &outY = outputWorkspace->mutableY(outputSpecIndex);
          auto &outE = outputWorkspace->mutableE(outputSpecIndex);

          std::copy(allX[rank].begin() + i * xSize, allX[rank].begin() + (i + 1) * xSize, outX.begin());
          std::copy(allY[rank].begin() + i * m_numBins, allY[rank].begin() + (i + 1) * m_numBins, outY.begin());
          std::copy(allE[rank].begin() + i * m_numBins, allE[rank].begin() + (i + 1) * m_numBins, outE.begin());

          // Copy detector IDs from local input (all ranks have same structure)
          const auto &inSpec = m_inputWorkspace->getSpectrum(inputSpecIndex);
          auto &outSpec = outputWorkspace->getSpectrum(outputSpecIndex);
          outSpec.clearDetectorIDs();
          outSpec.addDetectorIDs(inSpec.getDetectorIDs());
        }
      }
    } else {
      // Non-root processes just send their data
      gather(m_included, localX, 0);
      gather(m_included, localY, 0);
      gather(m_included, localE, 0);
    }
  }
}

void GatherWorkspaces::execEvent() {

  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator included; // The communicator containing all processes
  // The root process needs to create a workspace of the appropriate size
  EventWorkspace_sptr outputWorkspace;
  if (included.rank() == 0) {
    g_log.debug() << "Total number of spectra is " << m_totalSpec << "\n";
    // Create the workspace for the output
    outputWorkspace = std::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", m_sumSpec, m_numBins + m_hist, m_numBins));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(*m_eventW, *outputWorkspace, true);
    setProperty("OutputWorkspace", outputWorkspace);
    ExperimentInfo_sptr inWS = m_inputWorkspace;
    outputWorkspace->copyExperimentInfoFrom(inWS.get());
  }

  for (size_t wi = 0; wi < m_totalSpec; wi++) {
    if (included.rank() == 0) {
      // How do we accumulate the data?
      std::string accum = this->getPropertyValue("AccumulationMethod");
      std::vector<Mantid::DataObjects::EventList> out_values;
      gather(included, m_eventW->getSpectrum(wi), out_values, 0);
      for (int i = 0; i < included.size(); i++) {
        size_t index = wi; // accum == "Add"
        if (accum == "Append")
          index = wi + i * m_totalSpec;
        outputWorkspace->dataX(index) = m_eventW->readX(wi);
        outputWorkspace->getSpectrum(index) += out_values[i];
        const auto &inSpec = m_eventW->getSpectrum(wi);
        auto &outSpec = outputWorkspace->getSpectrum(index);
        outSpec.clearDetectorIDs();
        outSpec.addDetectorIDs(inSpec.getDetectorIDs());
      }
    } else {
      gather(included, m_eventW->getSpectrum(wi), 0);
    }
  }
}

} // namespace Mantid::MPIAlgorithms
