// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CompressEvents.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"

#include "tbb/parallel_for.h"

#include <numeric>
#include <set>

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(CompressEvents)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

namespace {
const std::vector<std::string> binningModeNames{"Default", "Linear", "Logarithmic"};
enum class BinningMode { DEFAULT, LINEAR, LOGARITHMIC, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;
} // namespace

void CompressEvents::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the EventWorkspace on which to perform the algorithm");

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output EventWorkspace.");

  // Tolerance must be >= 0.0
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(std::make_unique<PropertyWithValue<double>>("Tolerance", 1e-5, Direction::Input),
                  "The tolerance on each event's X value (normally TOF, but may be a "
                  "different unit if you have used ConvertUnits).\n"
                  "Any events within Tolerance will be summed into a single event. When compressing where positive is "
                  "linear tolerance, negative is logorithmic tolerance, and zero indicates that time-of-flight must be "
                  "identical to compress.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("WallClockTolerance", EMPTY_DBL(), mustBePositive, Direction::Input),
      "The tolerance (in seconds) on the wall-clock time for comparison. Unset "
      "means compressing all wall-clock times together disabling pulsetime "
      "resolution.");

  auto dateValidator = std::make_shared<DateTimeValidator>();
  dateValidator->allowEmpty(true);
  declareProperty("StartTime", "", dateValidator,
                  "An ISO formatted date/time string specifying the timestamp for "
                  "starting filtering. Ignored if WallClockTolerance is not specified. "
                  "Default is start of run",
                  Direction::Input);

  declareProperty("BinningMode", binningModeNames[size_t(BinningMode::DEFAULT)],
                  std::make_shared<Mantid::Kernel::StringListValidator>(binningModeNames),
                  "Binning behavior can be specified in the usual way through sign of tolerance and other properties "
                  "('Default'); or can be set to one of the allowed binning modes. This will override all other "
                  "specification or default behavior.");
  declareProperty("SortFirst", true,
                  "If false a different method, that will not sort events first, will be used to compress events which "
                  "is faster when you have a large number of events per compress tolerance");
}

void CompressEvents::exec() {
  // Get the input workspace
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  double toleranceTof = getProperty("Tolerance");
  const double toleranceWallClock = getProperty("WallClockTolerance");
  const bool compressFat = !isEmpty(toleranceWallClock);
  const bool sortFirst = getProperty("SortFirst");

  BINMODE mode = getPropertyValue("BinningMode");
  if (mode == BinningMode::LINEAR)
    toleranceTof = std::fabs(toleranceTof);
  else if (mode == BinningMode::LOGARITHMIC)
    toleranceTof = -1. * std::fabs(toleranceTof);

  Types::Core::DateAndTime startTime;

  if (compressFat) {
    std::string startTimeProp = getProperty("StartTime");
    if (startTimeProp.empty()) {
      startTime = inputWS->run().startTime();
    } else {
      // the property returns ISO8601
      startTime = DateAndTimeHelpers::createFromSanitizedISO8601(startTimeProp);
    }
  }

  // Some starting things
  bool inplace = (inputWS == outputWS);
  const size_t noSpectra = inputWS->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, noSpectra * 2);

  // Sort the input workspace in-place by TOF. This can be faster if there are
  // few event lists. Compressing with wall clock does the sorting internally
  if (!compressFat && sortFirst) {
    const auto timerStart = std::chrono::high_resolution_clock::now();
    inputWS->sortAll(TOF_SORT, &prog);
    addTimer("sortByTOF", timerStart, std::chrono::high_resolution_clock::now());
  }

  // created required variables if using unsorted methdo
  auto histogram_bin_edges = std::make_shared<std::vector<double>>();
  size_t num_edges{0};
  if (!compressFat && !sortFirst && !(inputWS->getSortType() == TOF_SORT)) {
    // only initialize if needed
    double tof_min_fixed;
    double tof_max_fixed;
    inputWS->getEventXMinMax(tof_min_fixed, tof_max_fixed);
    Mantid::Kernel::VectorHelper::createAxisFromRebinParams(
        {tof_min_fixed, toleranceTof, (tof_max_fixed + std::abs(toleranceTof))}, *histogram_bin_edges, true, true);
    num_edges = histogram_bin_edges->size();
  }

  // Are we making a copy of the input workspace?
  if (!inplace) {
    outputWS = create<EventWorkspace>(*inputWS, HistogramData::BinEdges(2));
    // We DONT copy the data though
    // Loop over the histograms (detector spectra)
    tbb::parallel_for(tbb::blocked_range<size_t>(0, noSpectra),
                      [compressFat, sortFirst, toleranceTof, startTime, toleranceWallClock, num_edges,
                       &histogram_bin_edges, &inputWS, &outputWS, &prog](const tbb::blocked_range<size_t> &range) {
                        for (size_t index = range.begin(); index < range.end(); ++index) {
                          // The input event list
                          EventList &input_el = inputWS->getSpectrum(index);
                          // And on the output side
                          EventList &output_el = outputWS->getSpectrum(index);
                          // Copy other settings into output
                          output_el.setX(input_el.ptrX());
                          // The EventList method does the work.
                          if (compressFat)
                            input_el.compressFatEvents(toleranceTof, startTime, toleranceWallClock, &output_el);
                          else if (sortFirst || input_el.isSortedByTof() || input_el.getNumberEvents() <= num_edges)
                            input_el.compressEvents(toleranceTof, &output_el);
                          else
                            input_el.compressEvents(toleranceTof, &output_el, histogram_bin_edges);
                          prog.report("Compressing");
                        }
                      });
  } else { // inplace
    tbb::parallel_for(tbb::blocked_range<size_t>(0, noSpectra),
                      [compressFat, sortFirst, toleranceTof, startTime, toleranceWallClock, num_edges,
                       &histogram_bin_edges, &outputWS, &prog](const tbb::blocked_range<size_t> &range) {
                        for (size_t index = range.begin(); index < range.end(); ++index) {
                          // The input (also output) event list
                          auto &output_el = outputWS->getSpectrum(index);
                          // The EventList method does the work.
                          if (compressFat)
                            output_el.compressFatEvents(toleranceTof, startTime, toleranceWallClock, &output_el);
                          else if (sortFirst || output_el.isSortedByTof() || output_el.getNumberEvents() <= num_edges)
                            output_el.compressEvents(toleranceTof, &output_el);
                          else
                            output_el.compressEvents(toleranceTof, &output_el, histogram_bin_edges);
                          prog.report("Compressing");
                        }
                      });
  }

  // Cast to the matrixOutputWS and save it
  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::DataHandling
