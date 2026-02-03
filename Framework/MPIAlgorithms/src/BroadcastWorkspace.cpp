// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidMPIAlgorithms/Chunking.h"
#include <algorithm>

#ifdef MPI_BUILD
#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>

namespace mpi = boost::mpi;
#endif

namespace Mantid::MPIAlgorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(BroadcastWorkspace)

void BroadcastWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "The workspace to be shared to other MPI processes. Input is optional - only the 'BroadcasterRank' "
                  "process should provide an InputWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The OutputWorkspace will be a copy of the InputWorkspace. Some workspace metadata may be lost.");
#ifdef MPI_BUILD
  auto maxRank = mpi::communicator().size() - 1;
#else
  int maxRank = 0;
#endif

  declareProperty("BroadcasterRank", 0, std::make_shared<BoundedValidator<int>>(0, maxRank),
                  "The rank of the process holding the workspace to broadcast (default: 0).");

  declareProperty("ChunkSize", 0, std::make_shared<BoundedValidator<int>>(-1, INT_MAX),
                  "Number of spectra to broadcast at a time. Use 0 for automatic chunk sizing "
                  "(recommended, targets ~100MB chunks), or -1 to broadcast all spectra at once "
                  "(fastest but uses most memory). A positive value specifies exact number of spectra per chunk.");
}

void BroadcastWorkspace::exec() {
#ifdef MPI_BUILD
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world;

  const int root = getProperty("BroadcasterRank");
  const int requestedChunkSize = getProperty("ChunkSize");

  MatrixWorkspace_const_sptr inputWorkspace;
  std::size_t numSpec{0};
  std::size_t numBins{0};
  bool hist{false};
  std::string xUnit, yUnit, yUnitLabel;
  bool distribution{false};
  bool sharedX{true};

  if (world.rank() == root) {
    inputWorkspace = getProperty("InputWorkspace");
    if (!inputWorkspace) {
      g_log.fatal("InputWorkspace '" + getPropertyValue("InputWorkspace") + "' not found in root process");
      mpi::environment::abort(-1);
    }
    numSpec = inputWorkspace->getNumberHistograms();
    numBins = inputWorkspace->blocksize();
    hist = inputWorkspace->isHistogramData();

    xUnit = inputWorkspace->getAxis(0)->unit()->unitID();
    yUnit = inputWorkspace->YUnit();
    yUnitLabel = inputWorkspace->YUnitLabel();
    distribution = inputWorkspace->isDistribution();

    // Check if all spectra share the same X data (common case)
    if (numSpec > 1) {
      const auto &x0 = inputWorkspace->sharedX(0);
      for (std::size_t i = 1; i < numSpec; ++i) {
        if (inputWorkspace->sharedX(i) != x0) {
          sharedX = false;
          break;
        }
      }
    }
  }

  broadcast(world, numSpec, root);
  broadcast(world, numBins, root);
  broadcast(world, hist, root);
  broadcast(world, xUnit, root);
  broadcast(world, yUnit, root);
  broadcast(world, yUnitLabel, root);
  broadcast(world, distribution, root);
  broadcast(world, sharedX, root);

  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", numSpec, numBins + hist, numBins);

  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(xUnit);
  outputWorkspace->setYUnit(yUnit);
  outputWorkspace->setYUnitLabel(yUnitLabel);
  outputWorkspace->setDistribution(distribution);

  // Handle X data - broadcast once if shared across all spectra
  const std::size_t xSize = numBins + hist;
  std::vector<double> xData(xSize);

  if (sharedX) {
    if (world.rank() == root) {
      const auto &x = inputWorkspace->x(0);
      std::copy(x.cbegin(), x.cend(), xData.begin());
    }
    broadcast(world, xData, root);

    auto &outX = outputWorkspace->mutableX(0);
    std::copy(xData.cbegin(), xData.cend(), outX.begin());
    auto sharedXPtr = outputWorkspace->sharedX(0);
    for (std::size_t i = 1; i < numSpec; ++i) {
      outputWorkspace->setSharedX(i, sharedXPtr);
    }
  }

  std::size_t chunkSize = Mantid::MPIAlgorithms::Chunking::chooseChunkSize(requestedChunkSize, numSpec, numBins);

  // Broadcast Y and E data in chunks
  for (std::size_t chunkStart = 0; chunkStart < numSpec; chunkStart += chunkSize) {
    const std::size_t chunkEnd = std::min(chunkStart + chunkSize, numSpec);
    const std::size_t spectraInChunk = chunkEnd - chunkStart;

    std::vector<double> chunkY(spectraInChunk * numBins);
    std::vector<double> chunkE(spectraInChunk * numBins);
    std::vector<double> chunkX;

    if (!sharedX) {
      chunkX.resize(spectraInChunk * xSize);
    }

    if (world.rank() == root) {
      for (std::size_t i = 0; i < spectraInChunk; ++i) {
        const std::size_t specIndex = chunkStart + i;
        const auto &y = inputWorkspace->y(specIndex);
        const auto &e = inputWorkspace->e(specIndex);

        std::copy(y.cbegin(), y.cend(), chunkY.begin() + i * numBins);
        std::copy(e.cbegin(), e.cend(), chunkE.begin() + i * numBins);

        if (!sharedX) {
          const auto &x = inputWorkspace->x(specIndex);
          std::copy(x.cbegin(), x.cend(), chunkX.begin() + i * xSize);
        }
      }
    }

    broadcast(world, chunkY, root);
    broadcast(world, chunkE, root);
    if (!sharedX) {
      broadcast(world, chunkX, root);
    }

    for (std::size_t i = 0; i < spectraInChunk; ++i) {
      const std::size_t specIndex = chunkStart + i;

      auto &outY = outputWorkspace->mutableY(specIndex);
      auto &outE = outputWorkspace->mutableE(specIndex);

      std::copy(chunkY.begin() + i * numBins, chunkY.begin() + (i + 1) * numBins, outY.begin());
      std::copy(chunkE.begin() + i * numBins, chunkE.begin() + (i + 1) * numBins, outE.begin());

      if (!sharedX) {
        auto &outX = outputWorkspace->mutableX(specIndex);
        std::copy(chunkX.begin() + i * xSize, chunkX.begin() + (i + 1) * xSize, outX.begin());
      }
    }
  }

  setProperty("OutputWorkspace", outputWorkspace);
#else
  API::MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  setProperty("OutputWorkspace", inputWorkspace);
  g_log.warning() << this->name() << " is only available in builds with MPI enabled (MPI_BUILD=ON)\n";
#endif
}

} // namespace Mantid::MPIAlgorithms
