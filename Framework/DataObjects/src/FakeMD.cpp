// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//--------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------
#include "MantidDataObjects/FakeMD.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Utils.h"

namespace Mantid {
namespace DataObjects {

using Kernel::ThreadPool;
using Kernel::ThreadSchedulerFIFO;

/**
 * Constructor
 * @param uniformParams Add a uniform, randomized distribution of events
 * @param peakParams Add a peak with a normal distribution around a central
 point
 * @param randomSeed Seed int for the random number generator
 * @param randomizeSignal If true, the events' signal and error values will be "
                          randomized around 1.0+-0.5
 */
FakeMD::FakeMD(const std::vector<double> &uniformParams,
               const std::vector<double> &peakParams, const int randomSeed,
               const bool randomizeSignal)
    : m_uniformParams(uniformParams), m_peakParams(peakParams),
      m_randomSeed(randomSeed), m_randomizeSignal(randomizeSignal), m_detIDs(),
      m_randGen(1), m_uniformDist() {
  if (uniformParams.empty() && peakParams.empty()) {
    throw std::invalid_argument(
        "You must specify at least one of peakParams or uniformParams");
  }
}

/**
 * Add the fake data to the given workspace
 * @param workspace A pointer to MD event workspace to fill using the object
 * parameters
 */
void FakeMD::fill(API::IMDEventWorkspace_sptr workspace) {
  setupDetectorCache(*workspace);

  CALL_MDEVENT_FUNCTION(this->addFakePeak, workspace)
  CALL_MDEVENT_FUNCTION(this->addFakeUniformData, workspace)

  // Mark that events were added, so the file back end (if any) needs updating
  workspace->setFileNeedsUpdating(true);
}

/**
 * Setup a detector cache for randomly picking IDs from the first
 * instrument in the ExperimentInfo list.
 * @param workspace The input workspace
 */
void FakeMD::setupDetectorCache(const API::IMDEventWorkspace &workspace) {
  try {
    auto inst = workspace.getExperimentInfo(0)->getInstrument();
    m_detIDs = inst->getDetectorIDs(true); // true=skip monitors
    size_t max = m_detIDs.size() - 1;
    m_uniformDist =
        Kernel::uniform_int_distribution<size_t>(0, max); // Includes max
  } catch (std::invalid_argument &) {
  }
}

/** Function makes up a fake single-crystal peak and adds it to the workspace.
 *
 * @param ws A pointer to the workspace that receives the events
 */
template <typename MDE, size_t nd>
void FakeMD::addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (m_peakParams.empty())
    return;

  if (m_peakParams.size() != nd + 2)
    throw std::invalid_argument("PeakParams needs to have ndims+2 arguments.");
  if (m_peakParams[0] <= 0)
    throw std::invalid_argument("PeakParams: number_of_events needs to be > 0");
  auto num = size_t(m_peakParams[0]);

  // Width of the peak
  double desiredRadius = m_peakParams.back();

  std::mt19937 rng(static_cast<unsigned int>(m_randomSeed));
  std::uniform_real_distribution<coord_t> flat(0, 1.0);

  // Inserter to help choose the correct event type
  auto eventHelper =
      MDEventInserter<typename MDEventWorkspace<MDE, nd>::sptr>(ws);

  for (size_t i = 0; i < num; ++i) {
    // Algorithm to generate points along a random n-sphere (sphere with not
    // necessarily 3 dimensions)
    // from http://en.wikipedia.org/wiki/N-sphere as of May 6, 2011.

    // First, points in a hyper-cube of size 1.0, centered at 0.
    coord_t centers[nd];
    coord_t radiusSquared = 0;
    for (size_t d = 0; d < nd; d++) {
      centers[d] = flat(rng) - 0.5f; // Distribute around +- the center
      radiusSquared += centers[d] * centers[d];
    }

    // Make a unit vector pointing in this direction
    auto radius = static_cast<coord_t>(sqrt(radiusSquared));
    for (size_t d = 0; d < nd; d++)
      centers[d] /= radius;

    // Now place the point along this radius, scaled with ^1/n for uniformity.
    coord_t radPos = flat(rng);
    radPos = static_cast<coord_t>(
        pow(radPos, static_cast<coord_t>(1.0 / static_cast<coord_t>(nd))));
    for (size_t d = 0; d < nd; d++) {
      // Multiply by the scaling and the desired peak radius
      centers[d] *= (radPos * static_cast<coord_t>(desiredRadius));
      // Also offset by the center of the peak, as taken in Params
      centers[d] += static_cast<coord_t>(m_peakParams[d + 1]);
    }

    // Default or randomized error/signal
    float signal = 1.0;
    float errorSquared = 1.0;
    if (m_randomizeSignal) {
      signal = float(0.5 + flat(rng));
      errorSquared = float(0.5 + flat(rng));
    }

    // Create and add the event.
    eventHelper.insertMDEvent(signal, errorSquared, 0, pickDetectorID(),
                              centers); // 0 = run index
  }

  ws->splitBox();
  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  ws->splitAllIfNeeded(ts);
  tp.joinAll();
  ws->refreshCache();
}

/**
 * Function makes up a fake uniform event data and adds it to the workspace.
 * @param ws
 */
template <typename MDE, size_t nd>
void FakeMD::addFakeUniformData(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (m_uniformParams.empty())
    return;

  bool randomEvents = true;
  if (m_uniformParams[0] < 0) {
    randomEvents = false;
    m_uniformParams[0] = -m_uniformParams[0];
  }

  if (m_uniformParams.size() == 1) {
    if (randomEvents) {
      for (size_t d = 0; d < nd; ++d) {
        m_uniformParams.push_back(ws->getDimension(d)->getMinimum());
        m_uniformParams.push_back(ws->getDimension(d)->getMaximum());
      }
    } else // regular events
    {
      auto nPoints = size_t(m_uniformParams[0]);
      double Vol = 1;
      for (size_t d = 0; d < nd; ++d)
        Vol *= (ws->getDimension(d)->getMaximum() -
                ws->getDimension(d)->getMinimum());

      if (Vol == 0 || Vol > std::numeric_limits<float>::max())
        throw std::invalid_argument(
            " Domain ranges are not defined properly for workspace: " +
            ws->getName());

      double dV = Vol / double(nPoints);
      double delta0 = std::pow(dV, 1. / double(nd));
      for (size_t d = 0; d < nd; ++d) {
        double min = ws->getDimension(d)->getMinimum();
        m_uniformParams.push_back(min * (1 + FLT_EPSILON) - min + FLT_EPSILON);
        double extent = ws->getDimension(d)->getMaximum() - min;
        auto nStrides = size_t(extent / delta0);
        if (nStrides < 1)
          nStrides = 1;
        m_uniformParams.push_back(extent / static_cast<double>(nStrides));
      }
    }
  }
  if ((m_uniformParams.size() != 1 + nd * 2))
    throw std::invalid_argument(
        "UniformParams: needs to have ndims*2+1 arguments ");

  if (randomEvents)
    addFakeRandomData<MDE, nd>(m_uniformParams, ws);
  else
    addFakeRegularData<MDE, nd>(m_uniformParams, ws);

  ws->splitBox();
  auto *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  ws->splitAllIfNeeded(ts);
  tp.joinAll();
  ws->refreshCache();
}

/**
 * Add fake randomized data to the workspace
 * @param params A reference to the parameter vector
 * @param ws The workspace to hold the data
 */
template <typename MDE, size_t nd>
void FakeMD::addFakeRandomData(const std::vector<double> &params,
                               typename MDEventWorkspace<MDE, nd>::sptr ws) {

  auto num = size_t(params[0]);
  if (num == 0)
    throw std::invalid_argument(
        " number of distributed events can not be equal to 0");

  // Inserter to help choose the correct event type
  auto eventHelper =
      MDEventInserter<typename MDEventWorkspace<MDE, nd>::sptr>(ws);

  // Array of distributions for each dimension
  std::mt19937 rng(static_cast<unsigned int>(m_randomSeed));
  std::array<std::uniform_real_distribution<double>, nd> gens;
  for (size_t d = 0; d < nd; ++d) {
    double min = params[d * 2 + 1];
    double max = params[d * 2 + 2];
    if (max <= min)
      throw std::invalid_argument(
          "UniformParams: min must be < max for all dimensions.");
    gens[d] = std::uniform_real_distribution<double>(min, max);
  }
  // Unit-size randomizer
  std::uniform_real_distribution<double> flat(0, 1.0); // Random from 0 to 1.0
  // Create all the requested events
  for (size_t i = 0; i < num; ++i) {
    coord_t centers[nd];
    for (size_t d = 0; d < nd; d++) {
      centers[d] = static_cast<coord_t>(gens[d](rng));
    }

    // Default or randomized error/signal
    float signal = 1.0;
    float errorSquared = 1.0;
    if (m_randomizeSignal) {
      signal = float(0.5 + flat(rng));
      errorSquared = float(0.5 + flat(rng));
    }

    // Create and add the event.
    eventHelper.insertMDEvent(signal, errorSquared, 0, pickDetectorID(),
                              centers); // 0 = run index
  }
}

template <typename MDE, size_t nd>
void FakeMD::addFakeRegularData(const std::vector<double> &params,
                                typename MDEventWorkspace<MDE, nd>::sptr ws) {
  // the parameters for regular distribution of events over the box
  std::vector<double> startPoint(nd), delta(nd);
  std::vector<size_t> indexMax(nd);
  size_t gridSize(0);

  auto num = size_t(params[0]);
  if (num == 0)
    throw std::invalid_argument(
        " number of distributed events can not be equal to 0");

  // Inserter to help choose the correct event type
  auto eventHelper =
      MDEventInserter<typename MDEventWorkspace<MDE, nd>::sptr>(ws);

  gridSize = 1;
  for (size_t d = 0; d < nd; ++d) {
    double min = ws->getDimension(d)->getMinimum();
    double max = ws->getDimension(d)->getMaximum();
    double shift = params[d * 2 + 1];
    double step = params[d * 2 + 2];
    if (shift < 0)
      shift = 0;
    if (shift >= step)
      shift = step * (1 - FLT_EPSILON);

    startPoint[d] = min + shift;
    if ((startPoint[d] < min) || (startPoint[d] >= max))
      throw std::invalid_argument("RegularData: starting point must be within "
                                  "the box for all dimensions.");

    if (step <= 0)
      throw(std::invalid_argument(
          "Step of the regular grid is less or equal to 0"));

    indexMax[d] = size_t((max - min) / step);
    if (indexMax[d] == 0)
      indexMax[d] = 1;
    // deal with round-off errors
    while ((startPoint[d] + double(indexMax[d] - 1) * step) >= max)
      step *= (1 - FLT_EPSILON);

    delta[d] = step;

    gridSize *= indexMax[d];
  }
  // Create all the requested events
  size_t cellCount(0);
  for (size_t i = 0; i < num; ++i) {
    coord_t centers[nd];

    auto indexes =
        Kernel::Utils::getIndicesFromLinearIndex(cellCount, indexMax);
    ++cellCount;
    if (cellCount >= gridSize)
      cellCount = 0;

    for (size_t d = 0; d < nd; d++) {
      centers[d] = coord_t(startPoint[d] + delta[d] * double(indexes[d]));
    }

    // Default or randomized error/signal
    float signal = 1.0;
    float errorSquared = 1.0;

    // Create and add the event.
    eventHelper.insertMDEvent(signal, errorSquared, 0, pickDetectorID(),
                              centers); // 0 = run index
  }
}

/**
 *  Pick a detector ID for a particular event
 *  @returns A detector ID randomly selected from the instrument
 */
detid_t FakeMD::pickDetectorID() {
  if (m_detIDs.empty()) {
    return -1;
  } else {
    return m_detIDs[m_uniformDist(m_randGen)];
  }
}

} // namespace DataObjects
} // namespace Mantid
