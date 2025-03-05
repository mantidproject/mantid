// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LogarithmicGenerator.h"
#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <cfloat>
#include <iterator>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using std::vector;

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void DiffractionFocussing2::init() {

  auto wsValidator = std::make_shared<API::RawCountValidator>();
  declareProperty(
      std::make_unique<API::WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "A 2D workspace with X values of d-spacing, Q or TOF (TOF support deprecated on 29/04/21)");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The result of diffraction focussing of InputWorkspace");

  declareProperty(std::make_unique<FileProperty>("GroupingFileName", "", FileProperty::OptionalLoad, ".cal"),
                  "Optional: The name of the CalFile with grouping data.");

  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "Optional: GroupingWorkspace to use instead of a grouping file.");

  declareProperty("PreserveEvents", true,
                  "Keep the output workspace as an EventWorkspace, if the "
                  "input has events (default).\n"
                  "If false, then the workspace gets converted to a "
                  "Workspace2D histogram.");
  declareProperty(std::make_unique<ArrayProperty<double>>("DMin"),
                  "Minimum x values, one value for each output specta or single value which is common to all");
  declareProperty(std::make_unique<ArrayProperty<double>>("DMax"),
                  "Maximum x values, one value for each output specta or single value which is common to all");
  declareProperty(std::make_unique<ArrayProperty<double>>("Delta"),
                  "Step parameters for rebin, positive values are constant step-size, negative are logorithmic. One "
                  "value for each output specta or single value which is common to all");
}

std::map<std::string, std::string> DiffractionFocussing2::validateInputs() {
  std::map<std::string, std::string> issues;

  // can only specify grouping in a single way
  const bool hasGroupingFilename = !isDefault("GroupingFileName");
  const bool hasGroupingWksp = !isDefault("GroupingWorkspace");
  if (hasGroupingFilename && hasGroupingWksp) {
    const std::string msg = "You must enter a GroupingFileName or a GroupingWorkspace, not both!";
    issues["GroupingFileName"] = msg;
    issues["GroupingWorkspace"] = msg;
  } else if (!(hasGroupingFilename || hasGroupingWksp)) {
    const std::string msg = "You must enter a GroupingFileName or a GroupingWorkspace!";
    issues["GroupingFileName"] = msg;
    issues["GroupingWorkspace"] = msg;
  }

  // validate input workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // Validate UnitID (spacing)
  const std::string unitid = inputWS->getAxis(0)->unit()->unitID();
  if (unitid == "TOF") {
    g_log.error(
        "Support for TOF data in DiffractionFocussing is deprecated (on 29/04/21) - use GroupDetectors instead)");
  } else if (unitid != "dSpacing" && unitid != "MomentumTransfer") {
    std::stringstream msg;
    msg << "UnitID " << unitid << " is not a supported spacing";
    issues["InputWorkspace"] = msg.str();
  }

  if (isDefault("DMin") && isDefault("DMax") && isDefault("Delta"))
    return issues;

  if (isDefault("DMin") || isDefault("DMax") || isDefault("Delta")) {
    issues["DMin"] = "Must specify values for XMin, XMax and Delta or none of them";
    issues["DMax"] = "Must specify values for XMin, XMax and Delta or none of them";
    issues["Delta"] = "Must specify values for XMin, XMax and Delta or none of them";
    return issues;
  }

  // check that delta is finite and non-zero, mins and maxs are finite and min is less than max
  const std::vector<double> xmins = getProperty("DMin");
  const std::vector<double> xmaxs = getProperty("DMax");
  const std::vector<double> deltas = getProperty("Delta");

  if (std::any_of(deltas.cbegin(), deltas.cend(), [](double d) { return !std::isfinite(d); }))
    issues["Delta"] = "All must be finite";
  else if (std::any_of(deltas.cbegin(), deltas.cend(), [](double d) { return d == 0; }))
    issues["Delta"] = "All must be nonzero";

  if (std::any_of(xmins.cbegin(), xmins.cend(), [](double x) { return !std::isfinite(x); }))
    issues["DMin"] = "All must be finite";

  if (std::any_of(xmaxs.cbegin(), xmaxs.cend(), [](double x) { return !std::isfinite(x); }))
    issues["DMax"] = "All must be finite";

  bool min_less_than_max = true;
  if (xmins.size() == 1) {
    if (xmins[0] >= *std::min_element(xmaxs.cbegin(), xmaxs.cend())) {
      min_less_than_max = false;
    }
  } else if (xmaxs.size() == 1) {
    if (xmaxs[0] <= *std::max_element(xmins.cbegin(), xmins.cend())) {
      min_less_than_max = false;
    }
  } else if (xmins.size() != xmaxs.size()) {
    issues["DMin"] = "DMin is different length to DMax";
    issues["DMax"] = "DMin is different length to DMax";
  } else {
    for (size_t i{0}; i < xmins.size(); i++) {
      if (xmins[i] >= xmaxs[i]) {
        min_less_than_max = false;
        break;
      }
    }
  }

  if (!min_less_than_max) {
    issues["DMin"] = "DMin must be less than corresponding DMax";
    issues["DMax"] = "DMin must be less than corresponding DMax";
  }

  return issues;
}

//=============================================================================
/** Perform clean-up of memory after execution but before destructor.
 * Private method
 */
void DiffractionFocussing2::cleanup() {
  // Clear maps and vectors to free up memory.
  groupAtWorkspaceIndex.clear();
  std::vector<int>().swap(groupAtWorkspaceIndex);
  group2xvector.clear();
  group2xvector = std::map<int, HistogramData::BinEdges>();
  group2xstep.clear();
  group2xstep = std::map<int, double>();
  group2wgtvector.clear();
  this->m_validGroups.clear();
  std::vector<Indexing::SpectrumNumber>().swap(m_validGroups);
  this->m_wsIndices.clear();
  std::vector<std::vector<std::size_t>>().swap(this->m_wsIndices);
}

//=============================================================================
/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void DiffractionFocussing2::exec() {
  // Get the input workspace
  m_matrixInputW = getProperty("InputWorkspace");
  nPoints = static_cast<int>(m_matrixInputW->blocksize());
  nHist = static_cast<int>(m_matrixInputW->getNumberHistograms());

  this->getGroupingWorkspace();

  const bool autoBinning = isDefault("DMin");

  // Fill the map
  progress(0.2, "Determine Rebin Params");
  {                              // keep variables in relatively small scope
    std::vector<int> udet2group; // map from udet to group
    g_log.debug() << "(1) nGroups " << nGroups << "\n";
    m_groupWS->makeDetectorIDToGroupVector(udet2group, nGroups);
    if (nGroups <= 0)
      throw std::runtime_error("No groups were specified.");
    g_log.debug() << "(2) nGroups " << nGroups << "\n";

    // This finds the rebin parameters (used in both versions)
    // It also initializes the groupAtWorkspaceIndex[] array.
    if (autoBinning)
      determineRebinParameters(udet2group);
    else {
      determineRebinParametersFromParameters(udet2group);
      nPoints = 1; // only needed for workspace init, histogram will be replaced
    }
  }

  size_t totalHistProcess = this->setupGroupToWSIndices();

  // determine event workspace min/max tof
  double eventXMin = 0.;
  double eventXMax = 0.;

  const auto eventInputWS = std::dynamic_pointer_cast<const EventWorkspace>(m_matrixInputW);
  if (eventInputWS) {
    if (getProperty("PreserveEvents")) {
      // Input workspace is an event workspace. Use the other exec method
      this->execEvent();
      this->cleanup();
      return; // <- return early!!!!!!!!!!!!!
    } else {
      // get the full d-spacing range
      m_matrixInputW->getXMinMax(eventXMin, eventXMax);
    }
  }

  // Check valida detectors are found in the .Cal file
  if (nGroups <= 0) {
    throw std::runtime_error("No selected Detectors found in .cal file for "
                             "input range. Please ensure spectra range has "
                             "atleast one selected detector.");
  }
  // Check the number of points
  if (nPoints <= 0) {
    throw std::runtime_error("No points found in the data range.");
  }
  API::MatrixWorkspace_sptr out =
      API::WorkspaceFactory::Instance().create(m_matrixInputW, m_validGroups.size(), nPoints + 1, nPoints);
  // Caching containers that are either only read from or unused. Initialize
  // them once.
  // Helgrind will show a race-condition but the data is completely unused so it
  // is irrelevant
  MantidVec weights_default(1, 1.0), emptyVec(1, 0.0);

  Progress prog(this, 0.2, 1.0, static_cast<int>(totalHistProcess) + nGroups);

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_matrixInputW, *out))
  for (int outWorkspaceIndex = 0; outWorkspaceIndex < static_cast<int>(m_validGroups.size()); outWorkspaceIndex++) {
    PARALLEL_START_INTERRUPT_REGION
    auto group = static_cast<int>(m_validGroups[outWorkspaceIndex]);

    // Get the group
    auto &Xout = group2xvector.at(group);

    // Assign the new X axis only once (i.e when this group is encountered the
    // first time)
    int nPoints_local(nPoints);
    if (!autoBinning) {
      nPoints_local = static_cast<int>(Xout.size() - 1);
      out->resizeHistogram(outWorkspaceIndex, nPoints_local);
    }

    out->setBinEdges(outWorkspaceIndex, Xout);

    // This is the output spectrum
    auto &outSpec = out->getSpectrum(outWorkspaceIndex);
    outSpec.setSpectrumNo(group);

    // Get the references to Y and E output and rebin
    // TODO can only be changed once rebin implemented in HistogramData
    auto &Yout = outSpec.dataY();
    auto &Eout = outSpec.dataE();

    // Initialize the group's weight vector here and the dummy vector used for
    // accumulating errors.
    MantidVec EOutDummy(nPoints_local);
    MantidVec groupWgt(nPoints_local, 0.0);

    // loop through the contributing histograms
    const std::vector<size_t> &indices = m_wsIndices[outWorkspaceIndex];
    const size_t groupSize = indices.size();
    for (size_t i = 0; i < groupSize; i++) {
      size_t inWorkspaceIndex = indices[i];
      // This is the input spectrum
      const auto &inSpec = m_matrixInputW->getSpectrum(inWorkspaceIndex);
      // Get reference to its old X,Y,and E.
      auto &Xin = inSpec.x();

      // copy over detector ids
      outSpec.addDetectorIDs(inSpec.getDetectorIDs());

      // get histogram version of the data
      if (eventInputWS) {
        const EventList &el = eventInputWS->getSpectrum(inWorkspaceIndex);
        // generateHistogram overwrites the data in Y and E so write to a temporary vector
        MantidVec Ytemp;
        MantidVec Etemp;
        el.generateHistogram(group2xstep.at(group), Xout.rawData(), Ytemp, Etemp);
        // accumulate the histogram into the output
        std::transform(Ytemp.cbegin(), Ytemp.cend(), Yout.begin(), Yout.begin(),
                       [](const auto &left, const auto &right) { return left + right; });
        // accumulate the square of the error
        std::transform(Etemp.cbegin(), Etemp.cend(), Eout.begin(), Eout.begin(),
                       [](const auto &left, const auto &right) { return left * left + right; });
      } else {
        auto &Yin = inSpec.y();
        auto &Ein = inSpec.e();

        try {
          // TODO This should be implemented in Histogram as rebin
          Mantid::Kernel::VectorHelper::rebinHistogram(Xin.rawData(), Yin.rawData(), Ein.rawData(), Xout.rawData(),
                                                       Yout, Eout, true);
        } catch (...) {
          // Should never happen because Xout is constructed to envelop all of the
          // Xin vectors
          std::ostringstream mess;
          mess << "Error in rebinning process for spectrum:" << inWorkspaceIndex;
          throw std::runtime_error(mess.str());
        }
      }

      // Check for masked bins in this spectrum
      if (m_matrixInputW->hasMaskedBins(i)) {
        MantidVec weight_bins, weights;
        weight_bins.emplace_back(Xin.front());
        // If there are masked bins, get a reference to the list of them
        const API::MatrixWorkspace::MaskList &mask = m_matrixInputW->maskedBins(i);
        // Now iterate over the list, adjusting the weights for the affected
        // bins
        for (const auto &bin : mask) {
          const double currentX = Xin[bin.first];
          // Add an intermediate bin with full weight if masked bins aren't
          // consecutive
          if (weight_bins.back() != currentX) {
            weights.emplace_back(1.0);
            weight_bins.emplace_back(currentX);
          }
          // The weight for this masked bin is 1 - the degree to which this bin
          // is masked
          weights.emplace_back(1.0 - bin.second);
          weight_bins.emplace_back(Xin[bin.first + 1]);
        }
        // Add on a final bin with full weight if masking doesn't go up to the
        // end
        if (weight_bins.back() != Xin.back()) {
          weights.emplace_back(1.0);
          weight_bins.emplace_back(Xin.back());
        }

        // Create a zero vector for the errors because we don't care about them
        // here
        const MantidVec zeroes(weights.size(), 0.0);
        // Rebin the weights - note that this is a distribution
        VectorHelper::rebin(weight_bins, weights, zeroes, Xout.rawData(), groupWgt, EOutDummy, true, true);
      } else // If no masked bins we want to add 1 to the weight of the output
             // bins that this input covers
      {
        // Initialized within the loop to avoid having to wrap writing to it
        // with a PARALLEL_CRITICAL sections
        MantidVec limits(2);

        if (eventXMin > 0. && eventXMax > 0.) {
          limits[0] = eventXMin;
          limits[1] = eventXMax;
        } else {
          limits[0] = Xin.front();
          limits[1] = Xin.back();
        }

        // Rebin the weights - note that this is a distribution
        VectorHelper::rebin(limits, weights_default, emptyVec, Xout.rawData(), groupWgt, EOutDummy, true, true);
      }
      prog.report();
    } // end of loop for input spectra

    // Calculate the bin widths
    std::vector<double> widths(Xout.size());
    std::adjacent_difference(Xout.begin(), Xout.end(), widths.begin());

    // Take the square root of the errors
    std::transform(Eout.begin(), Eout.end(), Eout.begin(), static_cast<double (*)(double)>(sqrt));

    // Multiply the data and errors by the bin widths because the rebin
    // function, when used
    // in the fashion above for the weights, doesn't put it back in
    std::transform(Yout.begin(), Yout.end(), widths.begin() + 1, Yout.begin(), std::multiplies<double>());
    std::transform(Eout.begin(), Eout.end(), widths.begin() + 1, Eout.begin(), std::multiplies<double>());

    // Now need to normalise the data (and errors) by the weights
    std::transform(Yout.begin(), Yout.end(), groupWgt.begin(), Yout.begin(), std::divides<double>());
    std::transform(Eout.begin(), Eout.end(), groupWgt.begin(), Eout.begin(), std::divides<double>());
    // Now multiply by the number of spectra in the group
    std::for_each(Yout.begin(), Yout.end(), [groupSize](double &val) { val *= static_cast<double>(groupSize); });
    std::for_each(Eout.begin(), Eout.end(), [groupSize](double &val) { val *= static_cast<double>(groupSize); });

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of loop for groups
  PARALLEL_CHECK_INTERRUPT_REGION

  setProperty("OutputWorkspace", out);

  this->cleanup();
}

//=============================================================================
/** Executes the algorithm in the case of an Event input workspace
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void DiffractionFocussing2::execEvent() {
  // Create a new outputworkspace with not much in it - bin boundaries will be replaced later
  auto eventOutputW = create<EventWorkspace>(*m_matrixInputW, m_validGroups.size(), HistogramData::BinEdges(2));

  // determine if this is an in-place operation so events can be deleted from the in put while running
  bool inPlace;
  {
    MatrixWorkspace_const_sptr outputWS = getProperty("OutputWorkspace");
    inPlace = bool(m_matrixInputW == outputWS);
  }
  if (inPlace) {
    g_log.debug("Focussing EventWorkspace in-place.");
  }
  g_log.debug() << nGroups << " groups found in .cal file (counting group 0).\n";

  const auto eventinputWS = std::dynamic_pointer_cast<const EventWorkspace>(m_matrixInputW);
  const EventType eventWtype = eventinputWS->getEventType();
  if (inPlace) {
    // MRU isn't needed since the workspace will be deleted soon
    std::const_pointer_cast<EventWorkspace>(eventinputWS)->clearMRU();
  }

  std::unique_ptr<Progress> prog = std::make_unique<Progress>(this, 0.2, 0.25, nGroups);

  // determine precount size
  vector<size_t> size_required(this->m_validGroups.size(), 0);
  int totalHistProcess = 0;
  for (size_t iGroup = 0; iGroup < this->m_validGroups.size(); iGroup++) {
    const vector<size_t> &indices = this->m_wsIndices[iGroup];

    totalHistProcess += static_cast<int>(indices.size());
    for (auto index : indices) {
      size_required[iGroup] += eventinputWS->getSpectrum(index).getNumberEvents();
    }
    prog->report(1, "Pre-counting");
  }

  // ------------- Pre-allocate Event Lists ----------------------------
  prog.reset();
  prog = std::make_unique<Progress>(this, 0.25, 0.3, totalHistProcess);

  // This creates and reserves the space required
  for (size_t iGroup = 0; iGroup < this->m_validGroups.size(); iGroup++) {
    const auto group = static_cast<int>(m_validGroups[iGroup]);
    EventList &groupEL = eventOutputW->getSpectrum(iGroup);
    groupEL.switchTo(eventWtype);
    groupEL.clear(true); // remove detector ids
    groupEL.reserve(size_required[iGroup]);
    groupEL.setSpectrumNo(group);
    prog->reportIncrement(1, "Allocating");
  }

  // ----------- Focus ---------------
  prog.reset();
  prog = std::make_unique<Progress>(this, 0.3, 0.9, totalHistProcess);

  if (this->m_validGroups.size() == 1) {
    g_log.information() << "Performing focussing on a single group\n";
    // Special case of a single group - parallelize differently
    EventList &groupEL = eventOutputW->getSpectrum(0);
    const std::vector<size_t> &indices = this->m_wsIndices[0];

    constexpr int chunkSize{200};

    const int end = (totalHistProcess / chunkSize) + 1;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int wiChunk = 0; wiChunk < end; wiChunk++) {
      PARALLEL_START_INTERRUPT_REGION

      // Perform in chunks for more efficiency
      int max = (wiChunk + 1) * chunkSize;
      if (max > totalHistProcess)
        max = totalHistProcess;

      // Make a blank EventList that will accumulate the chunk.
      EventList chunkEL;
      chunkEL.switchTo(eventWtype);
      // chunkEL.reserve(numEventsInChunk);

      // process the chunk
      for (int i = wiChunk * chunkSize; i < max; i++) {
        // Accumulate the chunk
        size_t wi = indices[i];
        chunkEL += eventinputWS->getSpectrum(wi);
      }

      // Rejoin the chunk with the rest.
      PARALLEL_CRITICAL(DiffractionFocussing2_JoinChunks) { groupEL += chunkEL; }

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } else {
    // ------ PARALLELIZE BY GROUPS -------------------------

    auto nValidGroups = static_cast<int>(this->m_validGroups.size());
    PARALLEL_FOR_IF(Kernel::threadSafe(*eventinputWS))
    for (int iGroup = 0; iGroup < nValidGroups; iGroup++) {
      PARALLEL_START_INTERRUPT_REGION
      const std::vector<size_t> &indices = this->m_wsIndices[iGroup];
      for (auto wi : indices) {
        // In workspace index iGroup, put what was in the OLD workspace index wi
        eventOutputW->getSpectrum(iGroup) += eventinputWS->getSpectrum(wi);

        prog->reportIncrement(1, "Appending Lists");

        // When focussing in place, you can clear out old memory from the input
        // one!
        if (inPlace) {
          std::const_pointer_cast<EventWorkspace>(eventinputWS)->getSpectrum(wi).clear(true);
        }
      }
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } // (done with parallel by groups)

  // Now that the data is cleaned up, go through it and set the X vectors to the
  // input workspace we first talked about.
  prog.reset();
  prog = std::make_unique<Progress>(this, 0.9, 1.0, nGroups);
  for (size_t workspaceIndex = 0; workspaceIndex < this->m_validGroups.size(); workspaceIndex++) {
    const auto group = static_cast<int>(m_validGroups[workspaceIndex]);
    // Now this is the workspace index of that group; simply 1 offset
    prog->reportIncrement(1, "Setting X");

    if (workspaceIndex >= eventOutputW->getNumberHistograms()) {
      g_log.warning() << "Warning! Invalid workspace index found for group # " << group
                      << ". Histogram will be empty.\n";
      continue;
    }

    // Now you set the X axis to the X you saved before.
    if (!group2xvector.empty()) {
      auto git = group2xvector.find(group);
      if (git != group2xvector.end())
        // Reset Histogram instead of BinEdges, the latter forbids size change.
        eventOutputW->setHistogram(workspaceIndex, BinEdges(git->second.cowData()));
      else
        // Just use the 1st X vector it found, instead of nothin.
        // Reset Histogram instead of BinEdges, the latter forbids size change.
        eventOutputW->setHistogram(workspaceIndex, BinEdges(group2xvector.begin()->second.cowData()));
    } else
      g_log.warning() << "Warning! No X histogram bins were found for any "
                         "groups. Histogram will be empty.\n";
  }
  eventOutputW->clearMRU();
  setProperty("OutputWorkspace", std::move(eventOutputW));
}

//=============================================================================
/** Verify that all the contributing detectors to a spectrum belongs to the same group
 *
 *  @param udet2group Map from udet to group
 *  @param wi :: The workspace index in the workspace
 *
 *  @return Group number if successful otherwise return -1
 */
int DiffractionFocussing2::validateSpectrumInGroup(const std::vector<int> &udet2group, size_t wi) {
  const auto &dets = m_matrixInputW->getSpectrum(wi).getDetectorIDs();
  if (dets.empty()) // Not in group
  {
    g_log.debug() << wi << " <- this workspace index is empty!\n";
    return -1;
  }

  auto it = dets.cbegin();
  if (*it < 0) // bad pixel id
    return -1;

  try { // what if index out of range?
    const int group = udet2group.at(*it);
    if (group <= 0)
      return -1;
    it++;
    for (; it != dets.end(); ++it) // Loop other all other udets
    {
      if (udet2group.at(*it) != group)
        return -1;
    }
    return group;
  } catch (...) {
  }

  return -1;
}

//=============================================================================
/** Determine the rebinning parameters, i.e Xmin, Xmax and logarithmic step for
 *each group
 * Looks for the widest range of X bins (lowest min and highest max) of
 *  all the spectra in a group and sets the output group X bin boundaries to use
 *  those limits.
 *  The X histogram is set to log binning with the same # of points between max
 *and min
 *  as the input spectra.
 *
 *  @param udet2group Map from udet to group
 *
 * The X vectors are saved in group2xvector.
 * It also initializes the groupAtWorkspaceIndex[] array.
 *
 */
void DiffractionFocussing2::determineRebinParameters(const std::vector<int> &udet2group) {
  std::ostringstream mess;

  // typedef for the storage of the group ranges
  using group2minmaxmap = std::map<int, std::pair<double, double>>;
  // Map from group number to its associated range parameters <Xmin,Xmax,step>
  group2minmaxmap group2minmax;
  group2minmaxmap::iterator gpit;

  const double BIGGEST = std::numeric_limits<double>::max();

  // whether or not to bother checking for a mask
  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = m_matrixInputW->getInstrument();
  if (instrument != nullptr) {
    checkForMask = ((instrument->getSource() != nullptr) && (instrument->getSample() != nullptr));
  }
  const auto &spectrumInfo = m_matrixInputW->spectrumInfo();

  groupAtWorkspaceIndex.resize(nHist);
  for (int wi = 0; wi < nHist; wi++) //  Iterate over all histograms to find X boundaries for each group
  {
    const int group = validateSpectrumInGroup(udet2group, static_cast<size_t>(wi));
    groupAtWorkspaceIndex[wi] = group;
    if (group == -1)
      continue;

    if (checkForMask) {
      if (spectrumInfo.isMasked(wi)) {
        groupAtWorkspaceIndex[wi] = -1;
        continue;
      }
    }
    gpit = group2minmax.find(group);

    // Create the group range in the map if it isn't already there
    if (gpit == group2minmax.end()) {
      gpit = group2minmax.emplace(group, std::make_pair(BIGGEST, -1. * BIGGEST)).first;
    }
    const double min = (gpit->second).first;
    const double max = (gpit->second).second;
    auto &X = m_matrixInputW->x(wi);
    double temp = X.front();
    if (temp < (min)) // New Xmin found
      (gpit->second).first = temp;
    temp = X.back();
    if (temp > (max)) // New Xmax found
      (gpit->second).second = temp;
  }

  nGroups = group2minmax.size(); // Number of unique groups

  const int64_t xPoints = nPoints + 1;
  // Iterator over all groups to create the new X vectors
  for (gpit = group2minmax.begin(); gpit != group2minmax.end(); ++gpit) {
    double Xmin, Xmax, step;
    Xmin = (gpit->second).first;
    Xmax = (gpit->second).second;

    // Make sure that Xmin is not 0 - since it is not possible to do log binning
    // from 0.0.
    if (Xmin <= 0)
      Xmin = Xmax / nPoints;
    if (Xmin <= 0)
      Xmin = 1.0;
    if (Xmin == Xmax)
      Xmin = Xmax / 2.0;

    if (Xmax < Xmin) // Should never happen
    {
      mess << "Fail to determine X boundaries for group:" << gpit->first << "\n";
      mess << "The boundaries are (Xmin,Xmax):" << Xmin << " " << Xmax;
      throw std::runtime_error(mess.str());
    }
    // This log step size will give the right # of points
    step = expm1((log(Xmax) - log(Xmin)) / nPoints);
    mess << "Found Group:" << gpit->first << "(Xmin,Xmax,log step):" << (gpit->second).first << ","
         << (gpit->second).second << "," << step;
    // g_log.information(mess.str());
    mess.str("");

    HistogramData::BinEdges xnew(xPoints, HistogramData::LogarithmicGenerator(Xmin, step));
    group2xvector[gpit->first] = xnew; // Register this vector in the map
    group2xstep[gpit->first] = -step;
  }
}

void DiffractionFocussing2::determineRebinParametersFromParameters(const std::vector<int> &udet2group) {
  std::set<int> groups;

  // whether or not to bother checking for a mask
  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = m_matrixInputW->getInstrument();
  if (instrument != nullptr) {
    checkForMask = ((instrument->getSource() != nullptr) && (instrument->getSample() != nullptr));
  }
  const auto &spectrumInfo = m_matrixInputW->spectrumInfo();

  groupAtWorkspaceIndex.resize(nHist);
  for (int wi = 0; wi < nHist; wi++) //  Iterate over all histograms to find which groups are actually used
  {
    const int group = validateSpectrumInGroup(udet2group, static_cast<size_t>(wi));
    groupAtWorkspaceIndex[wi] = group;
    if (group == -1)
      continue;

    if (checkForMask) {
      if (spectrumInfo.isMasked(wi)) {
        groupAtWorkspaceIndex[wi] = -1;
        continue;
      }
    }
    groups.insert(group);
  }

  nGroups = groups.size(); // Number of unique groups

  // only now can we check that the length of rebin parameters are correct
  std::vector<double> xmins = getProperty("DMin");
  std::vector<double> xmaxs = getProperty("DMax");
  std::vector<double> deltas = getProperty("Delta");

  const int64_t numMin = xmins.size();
  const int64_t numMax = xmaxs.size();
  const int64_t numDelta = deltas.size();
  if (numMin > 1 && numMin != nGroups)
    throw std::runtime_error("DMin must have length 1 or equal to number of output groups which is " +
                             std::to_string(nGroups));
  if (numMax > 1 && numMax != nGroups)
    throw std::runtime_error("DMax must have length 1 or equal to number of output groups which is " +
                             std::to_string(nGroups));
  if (numDelta > 1 && numDelta != nGroups)
    throw std::runtime_error("Delta must have length 1 or equal to number of output groups which is " +
                             std::to_string(nGroups));

  // resize vectors with only one value
  if (numMin == 1)
    xmins.resize(nGroups, xmins[0]);
  if (numMax == 1)
    xmaxs.resize(nGroups, xmaxs[0]);
  if (numDelta == 1)
    deltas.resize(nGroups, deltas[0]);

  // Iterator over all groups to create the new X vectors
  size_t i = 0;
  for (auto group : groups) {
    HistogramData::BinEdges xnew(0);
    static_cast<void>(VectorHelper::createAxisFromRebinParams({xmins[i], deltas[i], xmaxs[i]}, xnew.mutableRawData()));
    group2xvector[group] = xnew; // Register this vector in the map
    group2xstep[group] = deltas[i];
    i++;
  }
}

/**
 * Initialize the pointer to the grouping workspace based on input properties
 */
void DiffractionFocussing2::getGroupingWorkspace() {
  m_groupWS = getProperty("GroupingWorkspace");

  // Do we need to read the grouping workspace from a file?
  if (!m_groupWS) {
    const std::string groupingFileName = getProperty("GroupingFileName");
    progress(0.01, "Reading grouping file");
    auto childAlg = createChildAlgorithm("CreateGroupingWorkspace");
    childAlg->setProperty("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(m_matrixInputW));
    childAlg->setProperty("OldCalFilename", groupingFileName);
    childAlg->executeAsChildAlg();
    m_groupWS = childAlg->getProperty("OutputWorkspace");
  }
}

/***
 * Configure the mapping of output group to list of input workspace
 * indices, and the list of valid group numbers.
 *
 * @return the total number of input histograms that will be read.
 */
size_t DiffractionFocussing2::setupGroupToWSIndices() {
  // set up the mapping of group to input workspace index
  std::vector<std::vector<std::size_t>> wsIndices;
  wsIndices.reserve(this->nGroups + 1);
  auto nHist_st = static_cast<size_t>(nHist);
  for (size_t wi = 0; wi < nHist_st; wi++) {
    // wi is the workspace index (of the input)
    const int group = groupAtWorkspaceIndex[wi];
    if (group < 1) // Not in a group, or invalid group #
      continue;

    // resize the ws_indices if it is not big enough
    if (wsIndices.size() < static_cast<size_t>(group + 1)) {
      wsIndices.resize(group + 1);
    }

    // Also record a list of workspace indices
    wsIndices[group].emplace_back(wi);
  }

  // initialize a vector of the valid group numbers
  size_t totalHistProcess = 0;
  for (const auto &item : group2xvector) {
    const auto group = item.first;
    m_validGroups.emplace_back(group);
    totalHistProcess += wsIndices[group].size();
  }

  std::transform(m_validGroups.cbegin(), m_validGroups.cend(), std::back_inserter(m_wsIndices),
                 [&wsIndices](const auto &group) { return wsIndices[static_cast<int>(group)]; });

  return totalHistProcess;
}

} // namespace Mantid::Algorithms
