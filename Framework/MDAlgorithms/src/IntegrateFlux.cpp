// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegrateFlux.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>
#include <numeric>

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace Mantid::HistogramData;
using Mantid::Types::Event::TofEvent;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateFlux)

namespace {

/// Void deleter for shared pointers
class NoEventWorkspaceDeleting {
public:
  /// deleting operator. Does nothing
  void operator()(const API::MatrixWorkspace * /*unused*/) {}
};
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string IntegrateFlux::name() const { return "IntegrateFlux"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateFlux::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateFlux::category() const {
  return "MDAlgorithms\\Normalisation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string IntegrateFlux::summary() const {
  return "Interates spectra in a matrix workspace at a set of points.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateFlux::init() {
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("Momentum")),
      "An input workspace. Must have units of Momentum");
  auto validator = boost::make_shared<Kernel::BoundedValidator<int>>();
  validator->setLower(2);
  declareProperty("NPoints", 1000, validator,
                  "Number of points per output spectrum.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateFlux::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  size_t nX = static_cast<size_t>(static_cast<int>(getProperty("NPoints")));

  auto outputWS = createOutputWorkspace(*inputWS, nX);

  integrateSpectra(*inputWS, *outputWS);

  setProperty("OutputWorkspace", outputWS);
}

/**
 * Create an empty output workspace with required dimensions and defined
 * x-values
 * @param inputWS :: The input event workspace.
 * @param nX :: Suggested size of the output spectra. It can change in the
 * actual output.
 */
boost::shared_ptr<API::MatrixWorkspace>
IntegrateFlux::createOutputWorkspace(const API::MatrixWorkspace &inputWS,
                                     size_t nX) const {
  size_t nSpec = inputWS.getNumberHistograms();

  if (nSpec == 0) {
    throw std::runtime_error("Input workspace has no data.");
  }

  // make sure the output spectrum size isn't too large
  auto maxPoints = getMaxNumberOfPoints(inputWS);
  if (nX > maxPoints) {
    nX = maxPoints;
  }

  // and not 0 or 1 as they are to be used for interpolation
  if (nX < 2) {
    throw std::runtime_error("Failed to create output."
                             "Output spectra should have at least two points.");
  }

  // crate empty output workspace
  API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
      boost::shared_ptr<const API::MatrixWorkspace>(&inputWS,
                                                    NoEventWorkspaceDeleting()),
      nSpec, nX, nX);

  // claculate the integration points and save them in the x-vactors of
  // integrFlux
  double xMin = inputWS.getXMin();
  double xMax = inputWS.getXMax();
  double dx = (xMax - xMin) / static_cast<double>(nX - 1);
  const auto &x = ws->x(0);

  ws->setPoints(0, Points(x.size(), LinearGenerator(xMin, dx)));
  for (size_t sp = 1; sp < nSpec; ++sp)
    ws->setSharedX(sp, ws->sharedX(0));

  return ws;
}

/**
 * Integrate spectra in inputWS at x-values in integrWS and save the results in
 * y-vectors of integrWS.
 * @param inputWS :: A workspace to integrate. The events have to be
 * weighted-no-time.
 * @param integrWS :: A workspace to store the results.
 */
void IntegrateFlux::integrateSpectra(const API::MatrixWorkspace &inputWS,
                                     API::MatrixWorkspace &integrWS) const {
  auto eventWS = dynamic_cast<const DataObjects::EventWorkspace *>(&inputWS);

  if (eventWS) {
    auto eventType = eventWS->getEventType();
    switch (eventType) {
    case (API::WEIGHTED_NOTIME):
      integrateSpectraEvents<DataObjects::WeightedEventNoTime>(*eventWS,
                                                               integrWS);
      return;
    case (API::WEIGHTED):
      integrateSpectraEvents<DataObjects::WeightedEvent>(*eventWS, integrWS);
      return;
    case (API::TOF):
      integrateSpectraEvents<TofEvent>(*eventWS, integrWS);
      return;
    }
  } else {
    integrateSpectraMatrix(inputWS, integrWS);
  }
}

/**
 * Integrate spectra in inputWS at x-values in integrWS and save the results in
 * y-vectors of integrWS.
 * @param inputWS :: An event workspace to integrate.
 * @param integrWS :: A workspace to store the results.
 */
template <class EventType>
void IntegrateFlux::integrateSpectraEvents(
    const DataObjects::EventWorkspace &inputWS,
    API::MatrixWorkspace &integrWS) const {
  inputWS.sortAll(DataObjects::TOF_SORT, nullptr);
  size_t nSpec = inputWS.getNumberHistograms();
  assert(nSpec == integrWS.getNumberHistograms());

  auto &X = integrWS.x(0);
  // loop overr the spectra and integrate
  for (size_t sp = 0; sp < nSpec; ++sp) {
    const std::vector<EventType> *el;
    DataObjects::getEventsFrom(inputWS.getSpectrum(sp), el);
    auto &outY = integrWS.mutableY(sp);

    double sum = 0;
    auto x = X.begin() + 1;
    size_t i = 1;
    // the integral is a running sum of the event weights in the spectrum
    for (auto evnt = el->begin(); evnt != el->end(); ++evnt) {
      double tof = evnt->tof();
      while (x != X.end() && *x < tof) {
        outY[i] = sum;
        ++x;
        ++i;
      }
      if (x == X.end())
        break;
      sum += evnt->weight();
      outY[i] = sum;
    }

    while (x != X.end()) {
      outY[i] = sum;
      ++x;
      ++i;
    }
  }
}

/**
 * Integrate spectra in inputWS at x-values in integrWS and save the results in
 * y-vectors of integrWS.
 * @param inputWS :: A 2d workspace to integrate.
 * @param integrWS :: A workspace to store the results.
 */
void IntegrateFlux::integrateSpectraMatrix(
    const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const {
  bool isHistogram = inputWS.isHistogramData();

  if (isHistogram) {
    integrateSpectraHistograms(inputWS, integrWS);
  } else {
    integrateSpectraPointData(inputWS, integrWS);
  }
}

/**
 * Integrate spectra in inputWS at x-values in integrWS and save the results in
 * y-vectors of integrWS.
 * @param inputWS :: A 2d workspace to integrate.
 * @param integrWS :: A workspace to store the results.
 */
void IntegrateFlux::integrateSpectraHistograms(
    const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const {
  size_t nSpec = inputWS.getNumberHistograms();
  assert(nSpec == integrWS.getNumberHistograms());

  auto &X = integrWS.x(0);

  // loop over the spectra and integrate
  for (size_t sp = 0; sp < nSpec; ++sp) {
    auto &inX = inputWS.x(sp);
    auto inY = inputWS.counts(sp); // make a copy

    // integral at the first point is always 0
    auto outY = integrWS.mutableY(sp).begin();
    *outY = 0.0;
    ++outY;
    // initialize summation
    double sum = 0;
    // cache some iterators
    auto inXbegin = inX.begin();
    auto inXend = inX.end();
    auto x0 = inXbegin; // iterator over x in input workspace
    // loop over the iteration points starting from the second one
    for (auto outX = X.begin() + 1; outX != X.end(); ++outX, ++outY) {
      // there are no data to integrate
      if (x0 == inXend) {
        *outY = sum;
        continue;
      }

      // in each iteration we find the integral of the input spectrum
      // between bounds [lowerBound,upperBound]
      const double &lowerBound = *(outX - 1);
      double upperBound = *outX;

      // interval [*x0, *x1] is the smalest interval in inX that contains
      // the integration interval [lowerBound,upperBound]
      auto x1 = std::lower_bound(x0, inXend, upperBound);

      // reached end of input data
      if (x1 == inXend) {
        --x1;
        if (x1 == x0) {
          *outY = sum;
          x0 = inXend;
          continue;
        }
        upperBound = *x1;
      }

      // if starting point in input x is smaller (not equal) than the lower
      // integration bound
      // then there is a partial bin at the beginning of the interval
      if (*x0 < lowerBound) {
        // first find the part of bin [*x0,*(x0+1)] which hasn't been integrated
        // yet
        // the left boundary == lowerBound
        // the right boundary == min( upperBound, *(x0+1) )
        const double leftX = lowerBound;
        const double rightX = std::min(upperBound, *(x0 + 1));

        auto i = static_cast<size_t>(std::distance(inXbegin, x0));
        // add bin's fraction between leftX and rightX
        sum += inY[i] * (rightX - leftX) / (*(x0 + 1) - *x0);

        // if rightX == upperBound there is nothing left to integrate, move to
        // the next integration point
        if (rightX == upperBound) {
          *outY = sum;
          continue;
        }

        ++x0;
      }

      // accumulate values in bins that fit entirely into the integration
      // interval [lowerBound,upperBound]
      auto i0 = static_cast<size_t>(std::distance(inXbegin, x0));
      auto i1 = static_cast<size_t>(std::distance(inXbegin, x1));
      if (*x1 > upperBound)
        --i1;
      for (auto i = i0; i < i1; ++i) {
        sum += inY[i];
      }

      // if x1 is greater than upperBound there is a partial "bin" that has to
      // be added
      if (*x1 > upperBound) {
        // find the part of "bin" [*(x1-1),*x1] which needs to be integrated
        // the left boundary == *(x1-1)
        // the right boundary == upperBound
        const double leftX = *(x1 - 1);
        const double rightX = upperBound;

        auto i = static_cast<size_t>(std::distance(inXbegin, x1));
        // add the area under the line between leftX and rightX
        sum += inY[i - 1] * (rightX - leftX) / (*x1 - *(x1 - 1));

        // advance in the input workspace
        x0 = x1 - 1;
      } else {
        // advance in the input workspace
        x0 = x1;
      }

      // store the current sum
      *outY = sum;
    }
  }
}

/**
 * Integrate spectra in inputWS at x-values in integrWS and save the results in
 * y-vectors of integrWS.
 * @param inputWS :: A 2d workspace to integrate.
 * @param integrWS :: A workspace to store the results.
 */
void IntegrateFlux::integrateSpectraPointData(
    const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const {
  size_t nSpec = inputWS.getNumberHistograms();
  assert(nSpec == integrWS.getNumberHistograms());

  auto &X = integrWS.x(0);

  // loop overr the spectra and integrate
  for (size_t sp = 0; sp < nSpec; ++sp) {
    auto &inX = inputWS.x(sp);
    auto &inY = inputWS.y(sp);

    // integral at the first point is always 0
    auto outY = integrWS.mutableY(sp).begin();
    *outY = 0.0;
    ++outY;
    // initialize summation
    double sum = 0;
    // cache some iterators
    auto inXbegin = inX.begin();
    auto inXend = inX.end();
    auto x0 = inXbegin; // iterator over x in input workspace

    // loop over the iteration points starting from the second one
    for (auto outX = X.begin() + 1; outX != X.end(); ++outX, ++outY) {
      // there are no data to integrate
      if (x0 == inXend) {
        *outY = sum;
        continue;
      }

      // in each iteration we find the integral of the input spectrum
      // between bounds [lowerBound,upperBound]
      const double &lowerBound = *(outX - 1);
      double upperBound = *outX;

      // interval [*x0, *x1] is the smalest interval in inX that contains
      // the integration interval [lowerBound,upperBound]
      auto x1 = std::lower_bound(x0, inXend, upperBound);

      // reached end of input data
      if (x1 == inXend) {
        --x1;
        if (x1 == x0) {
          *outY = sum;
          x0 = inXend;
          continue;
        }
        upperBound = *x1;
      }

      // if starting point in input x is smaller (not equal) than the lower
      // integration bound
      // then there is a partial "bin" at the beginning of the interval
      if (*x0 < lowerBound) {
        // first find the part of "bin" [*x0,*(x0+1)] which hasn't been
        // integrated yet
        // the left boundary == lowerBound
        // the right boundary == min( upperBound, *(x0+1) )
        const double leftX = lowerBound;
        const double rightX = std::min(upperBound, *(x0 + 1));

        auto i = static_cast<size_t>(std::distance(inXbegin, x0));
        // gradient of "bin" [*x0,*(x0+1)]
        double dy_dx = (inY[i + 1] - inY[i]) / (*(x0 + 1) - *x0);

        // add the area under the line between leftX and rightX
        sum += (inY[i] + 0.5 * dy_dx * (leftX + rightX - 2 * (*(x0)))) *
               (rightX - leftX);

        // if rightX == upperBound there is nothing left to integrate, move to
        // the next integration point
        if (rightX == upperBound) {
          *outY = sum;
          continue;
        }

        ++x0;
      }

      // accumulate values in bins that fit entirely into the integration
      // interval [lowerBound,upperBound]
      auto i0 = static_cast<size_t>(std::distance(inXbegin, x0));
      auto i1 = static_cast<size_t>(std::distance(inXbegin, x1));
      if (*x1 > upperBound)
        --i1;

      for (auto i = i0; i < i1; ++i) {
        sum += (inY[i] + inY[i + 1]) / 2 * (inX[i + 1] - inX[i]);
      }

      // if x1 is greater than upperBound there is a partial "bin" that has to
      // be added
      if (*x1 > upperBound) {
        // find the part of "bin" [*(x1-1),*x1] which needs to be integrated
        // the left boundary == *(x1-1)
        // the right boundary == upperBound
        const double leftX = *(x1 - 1);
        const double rightX = upperBound;

        auto i = static_cast<size_t>(std::distance(inXbegin, x1));
        // gradient of "bin" [*(x1-1),*x1]
        double dy_dx = (inY[i] - inY[i - 1]) / (*x1 - *(x1 - 1));

        // add the area under the line between leftX and rightX
        sum += (inY[i - 1] + 0.5 * dy_dx * (rightX - *(x1 - 1))) *
               (rightX - leftX);

        // advance in the input workspace
        x0 = x1 - 1;
      } else {
        // advance in the input workspace
        x0 = x1;
      }

      // store the current sum
      *outY = sum;
    }
  }
}

/**
 * Calculate the maximun number of points in the integration grid.
 * @param inputWS :: An input workspace.
 */
size_t
IntegrateFlux::getMaxNumberOfPoints(const API::MatrixWorkspace &inputWS) const {
  // if it's events we shouldn't care about binning
  auto eventWS = dynamic_cast<const DataObjects::EventWorkspace *>(&inputWS);
  if (eventWS) {
    return eventWS->getSpectrum(0).getNumberEvents();
  }

  return inputWS.blocksize();
}

} // namespace MDAlgorithms
} // namespace Mantid
