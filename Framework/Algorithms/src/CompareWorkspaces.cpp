// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CompareWorkspaces.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Unit.h"

namespace Mantid::Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CompareWorkspaces)

namespace {

template <class ET> const std::vector<ET> &getEventVector(const EventList &el);

template <> const std::vector<Types::Event::TofEvent> &getEventVector(const EventList &el) { return el.getEvents(); }

template <> const std::vector<DataObjects::WeightedEvent> &getEventVector(const EventList &el) {
  return el.getWeightedEvents();
}

template <> const std::vector<DataObjects::WeightedEventNoTime> &getEventVector(const EventList &el) {
  return el.getWeightedEventsNoTime();
}

template <class ET>
int compareEventLists(Kernel::Logger &logger, const EventList &el1, const EventList &el2, double tolTof,
                      double tolWeight, int64_t tolPulse, bool printdetails, size_t &numdiffpulse, size_t &numdifftof,
                      size_t &numdiffboth, size_t &numdiffweight) {

  // Initialize
  numdiffpulse = 0;
  numdifftof = 0;
  numdiffboth = 0;
  numdiffweight = 0;

  // Compare event by event including all events
  const auto &events1 = getEventVector<ET>(el1);
  const auto &events2 = getEventVector<ET>(el2);

  int returnint = 0;
  size_t numevents = events1.size();
  for (size_t i = 0; i < numevents; ++i) {
    // Compare 2 individual events
    const auto &e1 = events1[i];
    const auto &e2 = events2[i];

    bool diffpulse = false;
    bool difftof = false;
    bool diffweight = false;
    if (std::abs(e1.pulseTime().totalNanoseconds() - e2.pulseTime().totalNanoseconds()) > tolPulse) {
      diffpulse = true;
      ++numdiffpulse;
    }
    if (std::abs(e1.tof() - e2.tof()) > tolTof) {
      difftof = true;
      ++numdifftof;
    }
    if (diffpulse && difftof)
      ++numdiffboth;
    if (std::abs(e1.weight() - e2.weight()) > tolWeight) {
      diffweight = true;
      ++numdiffweight;
    }

    bool same = (!diffpulse) && (!difftof) && (!diffweight);
    if (!same) {
      returnint += 1;
      if (printdetails) {
        std::stringstream outss;
        outss << "Spectrum ? Event " << i << ": ";
        if (diffpulse)
          outss << "Diff-Pulse: " << e1.pulseTime() << " vs. " << e2.pulseTime() << "; ";
        if (difftof)
          outss << "Diff-TOF: " << e1.tof() << " vs. " << e2.tof() << ";";
        if (diffweight)
          outss << "Diff-Weight: " << e1.weight() << " vs. " << e2.weight() << ";";

        logger.information(outss.str());
      }
    }
  } // End of loop on all events

  // Anything that gets this far is equal within tolerances
  return returnint;
}
} // namespace

/** Initialize the algorithm's properties.
 */
void CompareWorkspaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace1", "", Direction::Input),
                  "The name of the first input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace2", "", Direction::Input),
                  "The name of the second input workspace.");

  declareProperty("Tolerance", 1e-10, "The maximum amount by which values may differ between the workspaces.");

  declareProperty("CheckUncertainty", true,
                  "Whether to check that the y-value uncertainties (E) match "
                  "(only for matrix workspaces). ");
  declareProperty("CheckType", true,
                  "Whether to check that the data types "
                  "(Workspace2D vs EventWorkspace) match.");
  declareProperty("CheckAxes", true, "Whether to check that the axes match.");
  declareProperty("CheckSpectraMap", true, "Whether to check that the spectra-detector maps match. ");
  declareProperty("CheckInstrument", true, "Whether to check that the instruments match. ");
  declareProperty("CheckMasking", true, "Whether to check that the bin masking matches. ");

  // Have this one false by default - the logs are brittle
  declareProperty("CheckSample", false, "Whether to check that the sample (e.g. logs).");

  declareProperty("ToleranceRelErr", false,
                  "Treat tolerance as relative error rather then the absolute error.\n"
                  "This is only applicable to Matrix workspaces.");

  // Have this one false by default - it can be a lot of printing.
  declareProperty("CheckAllData", false,
                  "Usually checking data ends when first mismatch occurs. This "
                  "forces algorithm to check all data and print mismatch to "
                  "the debug log.\n"
                  "Very often such logs are huge so making it true should be "
                  "the last option.");

  declareProperty("NumberMismatchedSpectraToPrint", 1, "Number of mismatched spectra from lowest to be listed. ");

  declareProperty("DetailedPrintIndex", EMPTY_INT(), "Mismatched spectra that will be printed out in details. ");

  declareProperty("Result", false, Direction::Output);
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("Messages", "compare_msgs", Direction::Output),
                  "TableWorkspace containing messages about any mismatches detected");

  m_messages = std::make_shared<TableWorkspace>();
  m_messages->addColumn("str", "Message");
  m_messages->addColumn("str", "Workspace 1");
  m_messages->addColumn("str", "Workspace 2");
}

/** Execute the algorithm.
 */
void CompareWorkspaces::exec() {
  m_result = true;
  m_messages->setRowCount(0); // Clear table

  if (g_log.is(Logger::Priority::PRIO_DEBUG))
    m_parallelComparison = false;

  double const tolerance = getProperty("Tolerance");
  if (getProperty("ToleranceRelErr")) {
    this->m_compare = [tolerance](double const x1, double const x2) -> bool {
      return CompareWorkspaces::withinRelativeTolerance(x1, x2, tolerance);
    };
  } else {
    this->m_compare = [tolerance](double const x1, double const x2) -> bool {
      return CompareWorkspaces::withinAbsoluteTolerance(x1, x2, tolerance);
    };
  }

  this->doComparison();

  if (!m_result) {
    std::string message = m_messages->cell<std::string>(0, 0);
    g_log.warning() << "The workspaces did not match: " << message << '\n';
  } else {
    std::string ws1 = Workspace_const_sptr(getProperty("Workspace1"))->getName();
    std::string ws2 = Workspace_const_sptr(getProperty("Workspace2"))->getName();
    g_log.notice() << "The workspaces \"" << ws1 << "\" and \"" << ws2 << "\" matched!\n";
  }

  setProperty("Result", m_result);
  setProperty("Messages", m_messages);
}

//----------------------------------------------------------------------------------------------
/**
 * Process two groups and ensure the Result string is set properly on the final
 * algorithm.
 *
 * @return A boolean true if execution was sucessful, false otherwise
 */
bool CompareWorkspaces::processGroups() {
  m_result = true;
  m_messages->setRowCount(0); // Clear table

  // Get workspaces
  Workspace_const_sptr w1 = getProperty("Workspace1");
  Workspace_const_sptr w2 = getProperty("Workspace2");

  // Attempt to cast to WorkspaceGroups (will be nullptr on failure)
  WorkspaceGroup_const_sptr ws1 = std::dynamic_pointer_cast<const WorkspaceGroup>(w1);
  WorkspaceGroup_const_sptr ws2 = std::dynamic_pointer_cast<const WorkspaceGroup>(w2);

  if (ws1 && ws2) { // Both are groups
    processGroups(ws1, ws2);
  } else if (!ws1 && !ws2) { // Neither are groups (shouldn't happen)
    m_result = false;
    throw std::runtime_error("CompareWorkspaces::processGroups - Neither "
                             "input is a WorkspaceGroup. This is a logical "
                             "error in the code.");
  } else if (!ws1 || !ws2) {
    recordMismatch("Type mismatch. One workspace is a group, the other is not.");
  }

  if (m_result && ws1 && ws2) {
    g_log.notice() << "All workspaces in workspace groups \"" << ws1->getName() << "\" and \"" << ws2->getName()
                   << "\" matched!\n";
  }

  setProperty("Result", m_result);
  setProperty("Messages", m_messages);

  return true;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief CompareWorkspaces::processGroups
 * @param groupOne
 * @param groupTwo
 */
void CompareWorkspaces::processGroups(const std::shared_ptr<const API::WorkspaceGroup> &groupOne,
                                      const std::shared_ptr<const API::WorkspaceGroup> &groupTwo) {

  // Check their sizes
  const auto totalNum = static_cast<size_t>(groupOne->getNumberOfEntries());
  if (groupOne->getNumberOfEntries() != groupTwo->getNumberOfEntries()) {
    recordMismatch("GroupWorkspaces size mismatch.");
    return;
  }

  // See if there are any other properties that require setting
  const std::vector<Property *> &allProps = this->getProperties();
  std::vector<Property *> nonDefaultProps;
  nonDefaultProps.reserve(allProps.size());
  for (auto p : allProps) {
    const std::string &propName = p->name();
    // Skip those not set and the input workspaces
    if (p->isDefault() || propName == "Workspace1" || propName == "Workspace2")
      continue;
    nonDefaultProps.emplace_back(p);
  }
  const size_t numNonDefault = nonDefaultProps.size();

  const double progressFraction = 1.0 / static_cast<double>(totalNum);
  std::vector<std::string> namesOne = groupOne->getNames();
  std::vector<std::string> namesTwo = groupTwo->getNames();
  for (size_t i = 0; i < totalNum; ++i) {
    // We should use an algorithm for each so that the output properties are
    // reset properly
    Algorithm_sptr checker =
        this->createChildAlgorithm(this->name(), progressFraction * static_cast<double>(i),
                                   progressFraction * static_cast<double>(i + 1), false, this->version());
    checker->setPropertyValue("Workspace1", namesOne[i]);
    checker->setPropertyValue("Workspace2", namesTwo[i]);
    for (size_t j = 0; j < numNonDefault; ++j) {
      Property const *p = nonDefaultProps[j];
      checker->setPropertyValue(p->name(), p->value());
    }
    checker->execute();

    bool success = checker->getProperty("Result");
    if (!success) {
      ITableWorkspace_sptr table = checker->getProperty("Messages");
      recordMismatch(table->cell<std::string>(0, 0), namesOne[i], namesTwo[i]);
    }
  }
}

//----------------------------------------------------------------------------------------------
/**
 * @brief CompareWorkspaces::doComparison
 */
void CompareWorkspaces::doComparison() {
  Workspace_sptr w1 = getProperty("Workspace1");
  Workspace_sptr w2 = getProperty("Workspace2");

  // ==============================================================================
  // Peaks workspaces
  // ==============================================================================
  if (w1->id() == "PeaksWorkspace" || w2->id() == "PeaksWorkspace") {
    // Check that both workspaces are the same type
    PeaksWorkspace_sptr pws1 = std::dynamic_pointer_cast<PeaksWorkspace>(w1);
    PeaksWorkspace_sptr pws2 = std::dynamic_pointer_cast<PeaksWorkspace>(w2);

    // if any one of the pointer is null, record the error
    // -- meaning at least one of the input workspace cannot be casted
    //    into peakworkspace
    if ((pws1 && !pws2) || (!pws1 && pws2)) {
      recordMismatch("One workspace is a PeaksWorkspace and the other is not.");
      return;
    }

    // Check some peak-based stuff when both pointers are not null
    if (pws1 && pws2) {
      doPeaksComparison(pws1, pws2);
      return;
    }
  }

  // ==============================================================================
  // Lean Elastic Peaks workspaces
  // ==============================================================================
  if (w1->id() == "LeanElasticPeaksWorkspace" || w2->id() == "LeanElasticPeaksWorkspace") {
    auto lpws1 = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(w1);
    auto lpws2 = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(w2);

    if ((lpws1 && !lpws2) || (!lpws1 && lpws2)) {
      recordMismatch("One workspace is a LeanElasticPeaksWorkspace and the other is not.");
    }

    if (lpws1 && lpws2) {
      doLeanElasticPeaksComparison(lpws1, lpws2);
      return;
    }
  }

  // ==============================================================================
  // Table workspaces
  // ==============================================================================

  // Check that both workspaces are the same type
  auto tws1 = std::dynamic_pointer_cast<const ITableWorkspace>(w1);
  auto tws2 = std::dynamic_pointer_cast<const ITableWorkspace>(w2);
  if ((tws1 && !tws2) || (!tws1 && tws2)) {
    recordMismatch("One workspace is a TableWorkspace and the other is not.");
    return;
  }
  if (tws1 && tws2) {
    doTableComparison(tws1, tws2);
    return;
  }

  // ==============================================================================
  // MD workspaces
  // ==============================================================================

  // Check things for IMDEventWorkspaces
  IMDEventWorkspace_const_sptr mdews1 = std::dynamic_pointer_cast<const IMDEventWorkspace>(w1);
  IMDEventWorkspace_const_sptr mdews2 = std::dynamic_pointer_cast<const IMDEventWorkspace>(w2);
  if ((mdews1 && !mdews2) || (!mdews1 && mdews2)) {
    recordMismatch("One workspace is an IMDEventWorkspace and the other is not.");
    return;
  }
  // Check things for IMDHistoWorkspaces
  IMDHistoWorkspace_const_sptr mdhws1 = std::dynamic_pointer_cast<const IMDHistoWorkspace>(w1);
  IMDHistoWorkspace_const_sptr mdhws2 = std::dynamic_pointer_cast<const IMDHistoWorkspace>(w2);
  if ((mdhws1 && !mdhws2) || (!mdhws1 && mdhws2)) {
    recordMismatch("One workspace is an IMDHistoWorkspace and the other is not.");
    return;
  }

  if (mdhws1 || mdews1) // The '2' workspaces must match because of the checks above
  {
    this->doMDComparison(w1, w2);
    return;
  }

  // ==============================================================================
  // Event workspaces
  // ==============================================================================

  // These casts must succeed or there's a logical problem in the code
  MatrixWorkspace_const_sptr ws1 = std::dynamic_pointer_cast<const MatrixWorkspace>(w1);
  MatrixWorkspace_const_sptr ws2 = std::dynamic_pointer_cast<const MatrixWorkspace>(w2);

  EventWorkspace_const_sptr ews1 = std::dynamic_pointer_cast<const EventWorkspace>(ws1);
  EventWorkspace_const_sptr ews2 = std::dynamic_pointer_cast<const EventWorkspace>(ws2);
  if (getProperty("CheckType")) {
    if ((ews1 && !ews2) || (!ews1 && ews2)) {
      recordMismatch("One workspace is an EventWorkspace and the other is not.");
      return;
    } else if (w1 && w2 && (w1->id() != w2->id())) {
      std::stringstream msg;
      msg << "Workspace ids do not match: \"" << w1->id() << "\" != \"" << w2->id() << "\"";
      recordMismatch(msg.str());
      return;
    }
  }

  size_t numhist = ws1->getNumberHistograms();

  // Fewer steps if not events
  if (ews1 && ews2) {
    // we have to create the progress before the call to compareEventWorkspaces,
    // because it uses the m_progress and it will segfault if not created
    m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numhist * 5);
    // Compare event lists to see whether 2 event workspaces match each other
    if (!compareEventWorkspaces(*ews1, *ews2))
      return;
  } else {
    m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numhist * 2);
  }

  // ==============================================================================
  // Matrix workspaces (Event & 2D)
  // ==============================================================================

  // First check the data - always do this
  if (!checkData(ws1, ws2))
    return;

  // Now do the other ones if requested. Bail out as soon as we see a failure.
  m_progress->reportIncrement(numhist / 5, "Axes");
  if (static_cast<bool>(getProperty("CheckAxes")) && !checkAxes(ws1, ws2))
    return;
  m_progress->reportIncrement(numhist / 5, "SpectraMap");
  if (static_cast<bool>(getProperty("CheckSpectraMap")) && !checkSpectraMap(ws1, ws2))
    return;
  m_progress->reportIncrement(numhist / 5, "Instrument");
  if (static_cast<bool>(getProperty("CheckInstrument")) && !checkInstrument(ws1, ws2))
    return;
  m_progress->reportIncrement(numhist / 5, "Masking");
  if (static_cast<bool>(getProperty("CheckMasking")) && !checkMasking(ws1, ws2))
    return;
  m_progress->reportIncrement(numhist / 5, "Sample");
  if (static_cast<bool>(getProperty("CheckSample"))) {
    if (!checkSample(ws1->sample(), ws2->sample()))
      return;
    if (!checkRunProperties(ws1->run(), ws2->run()))
      return;
  }
}

//------------------------------------------------------------------------------------------------
/** Check whether 2 event lists are identical
 */
bool CompareWorkspaces::compareEventWorkspaces(const DataObjects::EventWorkspace &ews1,
                                               const DataObjects::EventWorkspace &ews2) {
  bool checkallspectra = getProperty("CheckAllData");
  int numspec2print = getProperty("NumberMismatchedSpectraToPrint");
  int wsindex2print = getProperty("DetailedPrintIndex");

  // Compare number of spectra
  if (ews1.getNumberHistograms() != ews2.getNumberHistograms()) {
    recordMismatch("Mismatched number of histograms.");
    return false;
  }

  if (ews1.getEventType() != ews2.getEventType()) {
    recordMismatch("Mismatched type of events in the EventWorkspaces.");
    return false;
  }

  // why the hell are you called after progress initialisation......... that's
  // why it segfaults
  // Both will end up sorted anyway
  ews1.sortAll(PULSETIMETOF_SORT, m_progress.get());
  ews2.sortAll(PULSETIMETOF_SORT, m_progress.get());

  if (!m_progress) {
    throw std::runtime_error("The progress pointer was found to be null!");
  }

  // Determine the tolerance for "tof" attribute and "weight" of events
  double toleranceWeight = Tolerance; // Standard tolerance
  int64_t tolerancePulse = 1;
  double toleranceTOF = 0.05;
  if ((ews1.getAxis(0)->unit()->label().ascii() != "microsecond") ||
      (ews2.getAxis(0)->unit()->label().ascii() != "microsecond")) {
    g_log.warning() << "Event workspace has unit as " << ews1.getAxis(0)->unit()->label().ascii() << " and "
                    << ews2.getAxis(0)->unit()->label().ascii() << ".  Tolerance of TOF is set to 0.05 still. "
                    << "\n";
    toleranceTOF = 0.05;
  }
  g_log.notice() << "TOF Tolerance = " << toleranceTOF << "\n";

  bool mismatchedEvent = false;
  int mismatchedEventWI = 0;

  size_t numUnequalNumEventsSpectra = 0;
  size_t numUnequalEvents = 0;
  size_t numUnequalTOFEvents = 0;
  size_t numUnequalPulseEvents = 0;
  size_t numUnequalBothEvents = 0;
  size_t numUnequalWeights = 0;

  std::vector<int> vec_mismatchedwsindex;
  PARALLEL_FOR_IF(m_parallelComparison && ews1.threadSafe() && ews2.threadSafe())
  for (int i = 0; i < static_cast<int>(ews1.getNumberHistograms()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    m_progress->report("EventLists");
    if (!mismatchedEvent || checkallspectra) // This guard will avoid checking unnecessarily
    {
      const EventList &el1 = ews1.getSpectrum(i);
      const EventList &el2 = ews2.getSpectrum(i);
      bool printdetail = (i == wsindex2print);
      if (printdetail) {
        g_log.information() << "Spectrum " << i << " is set to print out in details. "
                            << "\n";
      }

      if (!el1.equals(el2, toleranceTOF, toleranceWeight, tolerancePulse)) {
        size_t tempNumTof = 0;
        size_t tempNumPulses = 0;
        size_t tempNumBoth = 0;
        size_t tempNumWeight = 0;

        int tempNumUnequal = 0;

        if (el1.getNumberEvents() != el2.getNumberEvents()) {
          // Number of events are different
          tempNumUnequal = -1;
        } else {
          tempNumUnequal =
              compareEventsListInDetails(el1, el2, toleranceTOF, toleranceWeight, tolerancePulse, printdetail,
                                         tempNumPulses, tempNumTof, tempNumBoth, tempNumWeight);
        }

        mismatchedEvent = true;
        mismatchedEventWI = i;
        PARALLEL_CRITICAL(CompareWorkspaces) {
          if (tempNumUnequal == -1) {
            // 2 spectra have different number of events
            ++numUnequalNumEventsSpectra;
          } else {
            // 2 spectra have some events different to each other
            numUnequalEvents += static_cast<size_t>(tempNumUnequal);
            numUnequalTOFEvents += tempNumTof;
            numUnequalPulseEvents += tempNumPulses;
            numUnequalBothEvents += tempNumBoth;
            numUnequalWeights += tempNumWeight;
          }

          vec_mismatchedwsindex.emplace_back(i);
        } // Parallel critical region

      } // If elist 1 is not equal to elist 2
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  bool wsmatch;
  if (mismatchedEvent) {
    std::ostringstream mess;
    if (checkallspectra) {
      if (numUnequalNumEventsSpectra > 0)
        mess << "Total " << numUnequalNumEventsSpectra << " spectra have different number of events. "
             << "\n";

      mess << "Total " << numUnequalEvents << " (in " << ews1.getNumberEvents() << ") events are differrent. "
           << numUnequalTOFEvents << " have different TOF; " << numUnequalPulseEvents << " have different pulse time; "
           << numUnequalBothEvents << " have different in both TOF and pulse time; " << numUnequalWeights
           << " have different weights."
           << "\n";

      mess << "Mismatched event lists include " << vec_mismatchedwsindex.size() << " of "
           << "total " << ews1.getNumberHistograms() << " spectra. "
           << "\n";

      std::sort(vec_mismatchedwsindex.begin(), vec_mismatchedwsindex.end());
      numspec2print = std::min(numspec2print, static_cast<int>(vec_mismatchedwsindex.size()));
      for (int i = 0; i < numspec2print; ++i) {
        mess << vec_mismatchedwsindex[i] << ", ";
        if ((i + 1) % 10 == 0)
          mess << "\n";
      }
    } else {
      mess << "Quick comparison shows 2 workspaces do not match. "
           << "First found mismatched event list is at workspace index " << mismatchedEventWI;
    }
    recordMismatch(mess.str());
    wsmatch = false;
  } else {
    wsmatch = true;
  }

  return wsmatch;
}

//------------------------------------------------------------------------------------------------
/** Checks that the data matches
 *  @param ws1 :: the first workspace
 *  @param ws2 :: the second workspace
 *  @retval true The data matches
 *  @retval false The data does not matches
 */
bool CompareWorkspaces::checkData(const API::MatrixWorkspace_const_sptr &ws1,
                                  const API::MatrixWorkspace_const_sptr &ws2) {
  // Cache a few things for later use
  const size_t numHists = ws1->getNumberHistograms();
  bool raggedWorkspace{false};
  size_t numBins(0UL);
  try {
    numBins = ws1->blocksize();
  } catch (std::length_error &) {
    raggedWorkspace = true;
  }
  const bool histogram = ws1->isHistogramData();
  const bool checkAllData = getProperty("CheckAllData");
  const bool checkError = getProperty("CheckUncertainty");

  // First check that the workspace are the same size
  if (numHists != ws2->getNumberHistograms() ||
      (raggedWorkspace ? !ws2->isRaggedWorkspace() : numBins != ws2->blocksize())) {
    recordMismatch("Size mismatch");
    return false;
  }

  // Check that both are either histograms or point-like data
  if (histogram != ws2->isHistogramData()) {
    recordMismatch("Histogram/point-like mismatch");
    return false;
  }

  bool resultBool = true;
  bool logDebug = g_log.is(Logger::Priority::PRIO_DEBUG);

  // Now check the data itself
  PARALLEL_FOR_IF(m_parallelComparison && ws1->threadSafe() && ws2->threadSafe())
  for (long i = 0; i < static_cast<long>(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    m_progress->report("Histograms");

    if (resultBool || checkAllData) // Avoid checking unnecessarily
    {
      // Get references to the current spectrum
      const auto &X1 = ws1->x(i);
      const auto &Y1 = ws1->y(i);
      const auto &E1 = ws1->e(i);
      const auto &X2 = ws2->x(i);
      const auto &Y2 = ws2->y(i);
      const auto &E2 = ws2->e(i);

      if (Y1.size() != Y2.size()) {
        g_log.debug() << "Spectra " << i << " have different lenghts, " << X1.size() << " vs " << X2.size() << "\n";
        recordMismatch("Mismatch in spectra length");
        PARALLEL_CRITICAL(resultBool)
        resultBool = false;
      } else {

        for (int j = 0; j < static_cast<int>(Y1.size()); ++j) {
          bool err = (!m_compare(X1[j], X2[j]) || !m_compare(Y1[j], Y2[j]));
          if (checkError)
            err = err || !m_compare(E1[j], E2[j]);
          if (err) {
            if (logDebug) {
              g_log.debug() << "Data mismatch at cell (hist#,bin#): (" << i << "," << j << ")\n";
              g_log.debug() << " Dataset #1 (X,Y,E) = (" << X1[j] << "," << Y1[j] << "," << E1[j] << ")\n";
              g_log.debug() << " Dataset #2 (X,Y,E) = (" << X2[j] << "," << Y2[j] << "," << E2[j] << ")\n";
              g_log.debug() << " Difference (X,Y,E) = (" << std::abs(X1[j] - X2[j]) << "," << std::abs(Y1[j] - Y2[j])
                            << "," << std::abs(E1[j] - E2[j]) << ")\n";
            }
            PARALLEL_CRITICAL(resultBool)
            resultBool = false;
          }
        }

        // Extra one for histogram data
        if (histogram && !m_compare(X1.back(), X2.back())) {
          if (logDebug) {
            g_log.debug() << " Data ranges mismatch for spectra N: (" << i << ")\n";
            g_log.debug() << " Last bin ranges (X1_end vs X2_end) = (" << X1.back() << "," << X2.back() << ")\n";
          }
          PARALLEL_CRITICAL(resultBool)
          resultBool = false;
        }
      }
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (!resultBool)
    recordMismatch("Data mismatch");
  // return result
  return resultBool;
}

//------------------------------------------------------------------------------------------------
/**
 * Checks that the axes matches
 * @param ws1 :: the first workspace
 * @param ws2 :: the second workspace
 * @retval true The axes match
 * @retval false The axes do not match
 */
bool CompareWorkspaces::checkAxes(const API::MatrixWorkspace_const_sptr &ws1,
                                  const API::MatrixWorkspace_const_sptr &ws2) {
  const int numAxes = ws1->axes();

  if (numAxes != ws2->axes()) {
    recordMismatch("Different numbers of axes");
    return false;
  }

  for (int i = 0; i < numAxes; ++i) {
    std::ostringstream axis_name_ss;
    axis_name_ss << "Axis " << i;
    std::string axis_name = axis_name_ss.str();

    const Axis *const ax1 = ws1->getAxis(i);
    const Axis *const ax2 = ws2->getAxis(i);

    if (ax1->isSpectra() != ax2->isSpectra()) {
      recordMismatch(axis_name + " type mismatch");
      return false;
    }

    if (ax1->title() != ax2->title()) {
      recordMismatch(axis_name + " title mismatch");
      return false;
    }

    Unit_const_sptr ax1_unit = ax1->unit();
    Unit_const_sptr ax2_unit = ax2->unit();

    if ((ax1_unit == nullptr && ax2_unit != nullptr) || (ax1_unit != nullptr && ax2_unit == nullptr) ||
        (ax1_unit && ax1_unit->unitID() != ax2_unit->unitID())) {
      recordMismatch(axis_name + " unit mismatch");
      return false;
    }

    // Use Axis's equality operator to check length and values
    // Don't check spectra axis as that just takes it values from the ISpectrum
    // (see checkSpectraMap)
    if (ax1->isNumeric() && ax2->isNumeric()) {
      const auto *na1 = static_cast<const NumericAxis *>(ax1);
      const double tolerance = getProperty("Tolerance");
      if (!na1->equalWithinTolerance(*ax2, tolerance)) {
        recordMismatch(axis_name + " values mismatch");
        return false;
      }
    } else if (!ax1->isSpectra() && !ax1->operator==(*ax2)) {
      recordMismatch(axis_name + " values mismatch");
      return false;
    }
  }

  if (ws1->YUnit() != ws2->YUnit()) {
    g_log.debug() << "YUnit strings : WS1 = " << ws1->YUnit() << " WS2 = " << ws2->YUnit() << "\n";
    recordMismatch("YUnit mismatch");
    return false;
  }

  // Check both have the same distribution flag
  if (ws1->isDistribution() != ws2->isDistribution()) {
    g_log.debug() << "Distribution flags: WS1 = " << ws1->isDistribution() << " WS2 = " << ws2->isDistribution()
                  << "\n";
    recordMismatch("Distribution flag mismatch");
    return false;
  }

  // Everything's OK with the axes
  return true;
}

//------------------------------------------------------------------------------------------------
/// Checks that the spectra maps match
/// @param ws1 :: the first sp det map
/// @param ws2 :: the second sp det map
/// @retval true The maps match
/// @retval false The maps do not match
bool CompareWorkspaces::checkSpectraMap(const MatrixWorkspace_const_sptr &ws1, const MatrixWorkspace_const_sptr &ws2) {
  if (ws1->getNumberHistograms() != ws2->getNumberHistograms()) {
    recordMismatch("Number of spectra mismatch");
    return false;
  }

  for (size_t i = 0; i < ws1->getNumberHistograms(); i++) {
    const auto &spec1 = ws1->getSpectrum(i);
    const auto &spec2 = ws2->getSpectrum(i);
    if (spec1.getSpectrumNo() != spec2.getSpectrumNo()) {
      recordMismatch("Spectrum number mismatch");
      return false;
    }
    if (spec1.getDetectorIDs().size() != spec2.getDetectorIDs().size()) {
      std::ostringstream out;
      out << "Number of detector IDs mismatch: " << spec1.getDetectorIDs().size() << " vs "
          << spec2.getDetectorIDs().size() << " at workspace index " << i;
      recordMismatch(out.str());
      return false;
    }
    auto it2 = spec2.getDetectorIDs().cbegin();
    for (auto it1 = spec1.getDetectorIDs().cbegin(); it1 != spec1.getDetectorIDs().cend(); ++it1, ++it2) {
      if (*it1 != *it2) {
        recordMismatch("Detector IDs mismatch");
        return false;
      }
    }
  }

  // Everything's OK if we get to here
  return true;
}

//------------------------------------------------------------------------------------------------
/* @brief Checks that the instruments match
 *
 * @details the following checks are performed:
 * - instrument name
 * - positions and rotations of detectors
 * - mask of detectors
 * - position of the source and sample
 * - instrument parameters
 *
 * @param ws1 :: the first workspace
 * @param ws2 :: the second workspace
 * @retval true The instruments match
 *
 * @retval false The instruments do not match
 */
bool CompareWorkspaces::checkInstrument(const API::MatrixWorkspace_const_sptr &ws1,
                                        const API::MatrixWorkspace_const_sptr &ws2) {
  // First check the name matches
  if (ws1->getInstrument()->getName() != ws2->getInstrument()->getName()) {
    g_log.debug() << "Instrument names: WS1 = " << ws1->getInstrument()->getName()
                  << " WS2 = " << ws2->getInstrument()->getName() << "\n";
    recordMismatch("Instrument name mismatch");
    return false;
  }

  if (!ws1->detectorInfo().isEquivalent(ws2->detectorInfo())) {
    recordMismatch("DetectorInfo mismatch (position differences larger than "
                   "1e-9 m or other difference found)");
    return false;
  }

  if (!ws1->componentInfo().hasEquivalentSource(ws2->componentInfo())) {
    recordMismatch("Source mismatch: either one workspace has a source and the "
                   "other does not, or the sources are at different positions");
    return false;
  }

  if (!ws1->componentInfo().hasEquivalentSample(ws2->componentInfo())) {
    recordMismatch("Sample mismatch: either one workspace has a sample and the "
                   "other does not, or the samples are at different positions");
    return false;
  }

  const Geometry::ParameterMap &ws1_parmap = ws1->constInstrumentParameters();
  const Geometry::ParameterMap &ws2_parmap = ws2->constInstrumentParameters();

  const bool checkAllData = getProperty("CheckAllData");
  auto errorStr = ws1_parmap.diff(ws2_parmap, !checkAllData);
  if (!errorStr.empty()) {
    g_log.debug() << "Here information to help understand parameter map differences:\n";
    g_log.debug() << errorStr;
    recordMismatch("Instrument ParameterMap mismatch (differences in ordering ignored)");
    return false;
  }

  // All OK if we're here
  return true;
}

//------------------------------------------------------------------------------------------------
/// Checks that the bin masking matches
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The masking matches
/// @retval false The masking does not match
bool CompareWorkspaces::checkMasking(const API::MatrixWorkspace_const_sptr &ws1,
                                     const API::MatrixWorkspace_const_sptr &ws2) {
  const auto numHists = static_cast<int>(ws1->getNumberHistograms());

  for (int i = 0; i < numHists; ++i) {
    const bool ws1_masks = ws1->hasMaskedBins(i);
    if (ws1_masks != ws2->hasMaskedBins(i)) {
      g_log.debug() << "Only one workspace has masked bins for spectrum " << i << "\n";
      recordMismatch("Masking mismatch");
      return false;
    }

    // If there are masked bins, check that they match
    if (ws1_masks && ws1->maskedBins(i) != ws2->maskedBins(i)) {
      g_log.debug() << "Mask lists for spectrum " << i << " do not match\n";
      recordMismatch("Masking mismatch");
      return false;
    }
  }

  // All OK if here
  return true;
}

//------------------------------------------------------------------------------------------------
/// Checks that the sample matches
/// @param sample1 :: the first sample
/// @param sample2 :: the second sample
/// @retval true The sample matches
/// @retval false The samples does not match
bool CompareWorkspaces::checkSample(const API::Sample &sample1, const API::Sample &sample2) {
  std::string const name1 = sample1.getName();
  std::string const name2 = sample2.getName();
  if (name1 != name2) {
    g_log.debug("WS1 sample name: " + name1);
    g_log.debug("WS2 sample name: " + name2);
    recordMismatch("Sample name mismatch");
    return false;
  }
  // N.B. Sample shape properties are not currently written out to nexus
  // processed files, so omit here

  // All OK if here
  return true;
}

//------------------------------------------------------------------------------------------------
/// Checks that the Run matches
/// @param run1 :: the first run object
/// @param run2 :: the second run object
/// @retval true The sample matches
/// @retval false The samples does not match
bool CompareWorkspaces::checkRunProperties(const API::Run &run1, const API::Run &run2) {
  double run1Charge(-1.0);
  try {
    run1Charge = run1.getProtonCharge();
  } catch (Exception::NotFoundError &) {
  }
  double run2Charge(-1.0);
  try {
    run2Charge = run2.getProtonCharge();
  } catch (Exception::NotFoundError &) {
  }

  if (run1Charge != run2Charge) {
    g_log.debug() << "WS1 proton charge: " << run1Charge << "\n";
    g_log.debug() << "WS2 proton charge: " << run2Charge << "\n";
    recordMismatch("Proton charge mismatch");
    return false;
  }

  std::vector<Kernel::Property *> ws1logs = run1.getLogData();
  std::vector<Kernel::Property *> ws2logs = run2.getLogData();
  // Check that the number of separate logs is the same
  if (ws1logs.size() != ws2logs.size()) {
    g_log.debug() << "WS1 number of logs: " << ws1logs.size() << "\n";
    g_log.debug() << "WS2 number of logs: " << ws2logs.size() << "\n";
    recordMismatch("Different numbers of logs");
    return false;
  } else {
    // Sort logs by name before one-by-one comparison
    auto compareNames = [](Kernel::Property const *p1, Kernel::Property const *p2) { return p1->name() < p2->name(); };
    std::sort(ws1logs.begin(), ws1logs.end(), compareNames);
    std::sort(ws2logs.begin(), ws2logs.end(), compareNames);
    for (size_t i = 0; i < ws1logs.size(); ++i) {
      if (*(ws1logs[i]) != *(ws2logs[i])) {
        if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
          g_log.debug("WS1 log entry mismatch: " + ws1logs[i]->name());
          g_log.debug("WS2 log entry mismatch: " + ws2logs[i]->name());
        }
        recordMismatch("Log mismatch");
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------------------------
/** Compare 2 different events list with detailed information output (Linear)
 * It assumes that the number of events between these 2 are identical
 * el1 :: event list 1
 * el2 :: event list 2
 * tolfTOF :: tolerance of Time-of-flight (in micro-second)
 * tolWeight :: tolerance of weight for weighted neutron events
 * tolPulse :: tolerance of pulse time (in nanosecond)
 * NOTE: there is no need to compare the event type as it has been done by
 * other tjype of check
 * printdetails :: option for comparing. -1: simple, 0: full but no print, 1:
 * full with print
 * @return :: int.  -1: different number of events;  N > 0 : some
 *            events are not same
 */
int CompareWorkspaces::compareEventsListInDetails(const EventList &el1, const EventList &el2, double tolTof,
                                                  double tolWeight, int64_t tolPulse, bool printdetails,
                                                  size_t &numdiffpulse, size_t &numdifftof, size_t &numdiffboth,
                                                  size_t &numdiffweight) const {
  // Check
  if (el1.getNumberEvents() != el2.getNumberEvents())
    throw std::runtime_error("compareEventsListInDetails only work on 2 event lists with same "
                             "number of events.");

  switch (el1.getEventType()) {
  case EventType::TOF:
    return compareEventLists<Types::Event::TofEvent>(g_log, el1, el2, tolTof, tolWeight, tolPulse, printdetails,
                                                     numdiffpulse, numdifftof, numdiffboth, numdiffweight);
  case EventType::WEIGHTED:
    return compareEventLists<DataObjects::WeightedEvent>(g_log, el1, el2, tolTof, tolWeight, tolPulse, printdetails,
                                                         numdiffpulse, numdifftof, numdiffboth, numdiffweight);
  case EventType::WEIGHTED_NOTIME:
    return compareEventLists<DataObjects::WeightedEventNoTime>(g_log, el1, el2, tolTof, tolWeight, tolPulse,
                                                               printdetails, numdiffpulse, numdifftof, numdiffboth,
                                                               numdiffweight);
  default:
    throw std::runtime_error("Cannot compare event lists: unknown event type.");
  }
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doPeaksComparison(PeaksWorkspace_sptr tws1, PeaksWorkspace_sptr tws2) {
  // Check some table-based stuff
  if (tws1->getNumberPeaks() != tws2->getNumberPeaks()) {
    recordMismatch("Mismatched number of rows.");
    return;
  }
  if (tws1->columnCount() != tws2->columnCount()) {
    recordMismatch("Mismatched number of columns.");
    return;
  }

  // sort the workspaces before comparing
  {
    auto sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
    sortPeaks->setProperty("InputWorkspace", tws1);
    sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
    sortPeaks->setProperty("SortAscending", true);
    sortPeaks->executeAsChildAlg();
    IPeaksWorkspace_sptr tmp1 = sortPeaks->getProperty("OutputWorkspace");
    tws1 = std::dynamic_pointer_cast<PeaksWorkspace>(tmp1);

    sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
    sortPeaks->setProperty("InputWorkspace", tws2);
    sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
    sortPeaks->setProperty("SortAscending", true);
    sortPeaks->executeAsChildAlg();
    IPeaksWorkspace_sptr tmp2 = sortPeaks->getProperty("OutputWorkspace");
    tws2 = std::dynamic_pointer_cast<PeaksWorkspace>(tmp2);
  }

  const bool isRelErr = getProperty("ToleranceRelErr");
  for (int i = 0; i < tws1->getNumberPeaks(); i++) {
    const Peak &peak1 = tws1->getPeak(i);
    const Peak &peak2 = tws2->getPeak(i);
    for (size_t j = 0; j < tws1->columnCount(); j++) {
      std::shared_ptr<const API::Column> col = tws1->getColumn(j);
      std::string name = col->name();
      double s1 = 0.0;
      double s2 = 0.0;
      V3D v1(0, 0, 0);
      V3D v2(0, 0, 0);
      if (name == "RunNumber") {
        s1 = double(peak1.getRunNumber());
        s2 = double(peak2.getRunNumber());
      } else if (name == "DetId") {
        s1 = double(peak1.getDetectorID());
        s2 = double(peak2.getDetectorID());
      } else if (name == "h") {
        s1 = peak1.getH();
        s2 = peak2.getH();
      } else if (name == "k") {
        s1 = peak1.getK();
        s2 = peak2.getK();
      } else if (name == "l") {
        s1 = peak1.getL();
        s2 = peak2.getL();
      } else if (name == "Wavelength") {
        s1 = peak1.getWavelength();
        s2 = peak2.getWavelength();
      } else if (name == "Energy") {
        s1 = peak1.getInitialEnergy();
        s2 = peak2.getInitialEnergy();
      } else if (name == "TOF") {
        s1 = peak1.getTOF();
        s2 = peak2.getTOF();
      } else if (name == "DSpacing") {
        s1 = peak1.getDSpacing();
        s2 = peak2.getDSpacing();
      } else if (name == "Intens") {
        s1 = peak1.getIntensity();
        s2 = peak2.getIntensity();
      } else if (name == "SigInt") {
        s1 = peak1.getSigmaIntensity();
        s2 = peak2.getSigmaIntensity();
      } else if (name == "BinCount") {
        s1 = peak1.getBinCount();
        s2 = peak2.getBinCount();
      } else if (name == "Row") {
        s1 = peak1.getRow();
        s2 = peak2.getRow();
      } else if (name == "Col") {
        s1 = peak1.getCol();
        s2 = peak2.getCol();
      } else if (name == "IntHKL") {
        v1 = peak1.getIntHKL();
        v2 = peak2.getIntHKL();
      } else if (name == "IntMNP") {
        v1 = peak1.getIntMNP();
        v2 = peak2.getIntMNP();
      } else {
        g_log.information() << "Column " << name << " is not compared\n";
      }
      bool mismatch = false;
      if (isRelErr) {
        mismatch = !m_compare(s1, s2);
        // Q: why should we not also compare the vectors?
      } else {
        mismatch = !m_compare(s1, s2) ||       //
                   !m_compare(v1[0], v2[0]) || //
                   !m_compare(v1[1], v2[1]) || //
                   !m_compare(v1[2], v2[2]);   //
      }
      if (mismatch) {
        g_log.notice(name);
        g_log.notice() << "data mismatch in column name = " << name << "\n"
                       << "cell (row#, col#): (" << i << "," << j << ")\n"
                       << "value1 = " << s1 << "\n"
                       << "value2 = " << s2 << "\n";
        recordMismatch("Data mismatch");
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doLeanElasticPeaksComparison(const LeanElasticPeaksWorkspace_sptr &tws1,
                                                     const LeanElasticPeaksWorkspace_sptr &tws2) {
  // Check some table-based stuff
  if (tws1->getNumberPeaks() != tws2->getNumberPeaks()) {
    recordMismatch("Mismatched number of rows.");
    return;
  }
  if (tws1->columnCount() != tws2->columnCount()) {
    recordMismatch("Mismatched number of columns.");
    return;
  }

  // sort the workspaces before comparing
  auto sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
  sortPeaks->setProperty("InputWorkspace", tws1);
  sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
  sortPeaks->setProperty("SortAscending", true);
  sortPeaks->executeAsChildAlg();
  IPeaksWorkspace_sptr ipws1 = sortPeaks->getProperty("OutputWorkspace");

  sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
  sortPeaks->setProperty("InputWorkspace", tws2);
  sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
  sortPeaks->setProperty("SortAscending", true);
  sortPeaks->executeAsChildAlg();
  IPeaksWorkspace_sptr ipws2 = sortPeaks->getProperty("OutputWorkspace");

  const double tolerance = getProperty("Tolerance");
  const bool isRelErr = getProperty("ToleranceRelErr");
  for (int peakIndex = 0; peakIndex < ipws1->getNumberPeaks(); peakIndex++) {
    for (size_t j = 0; j < ipws1->columnCount(); j++) {
      std::shared_ptr<const API::Column> col = ipws1->getColumn(j);
      const std::string name = col->name();
      double s1 = 0.0;
      double s2 = 0.0;
      if (name == "RunNumber") {
        s1 = double(ipws1->getPeak(peakIndex).getRunNumber());
        s2 = double(ipws2->getPeak(peakIndex).getRunNumber());
      } else if (name == "h") {
        s1 = ipws1->getPeak(peakIndex).getH();
        s2 = ipws2->getPeak(peakIndex).getH();
      } else if (name == "k") {
        s1 = ipws1->getPeak(peakIndex).getK();
        s2 = ipws2->getPeak(peakIndex).getK();
      } else if (name == "l") {
        s1 = ipws1->getPeak(peakIndex).getL();
        s2 = ipws2->getPeak(peakIndex).getL();
      } else if (name == "Wavelength") {
        s1 = ipws1->getPeak(peakIndex).getWavelength();
        s2 = ipws2->getPeak(peakIndex).getWavelength();
      } else if (name == "DSpacing") {
        s1 = ipws1->getPeak(peakIndex).getDSpacing();
        s2 = ipws2->getPeak(peakIndex).getDSpacing();
      } else if (name == "Intens") {
        s1 = ipws1->getPeak(peakIndex).getIntensity();
        s2 = ipws2->getPeak(peakIndex).getIntensity();
      } else if (name == "SigInt") {
        s1 = ipws1->getPeak(peakIndex).getSigmaIntensity();
        s2 = ipws2->getPeak(peakIndex).getSigmaIntensity();
      } else if (name == "BinCount") {
        s1 = ipws1->getPeak(peakIndex).getBinCount();
        s2 = ipws2->getPeak(peakIndex).getBinCount();
      } else if (name == "QLab") {
        V3D q1 = ipws1->getPeak(peakIndex).getQLabFrame();
        V3D q2 = ipws2->getPeak(peakIndex).getQLabFrame();
        // using s1 here as the diff
        for (int i = 0; i < 3; ++i) {
          s1 += (q1[i] - q2[i]) * (q1[i] - q2[i]);
        }
        s1 = std::sqrt(s1);
        if (isRelErr) {
          // divide diff by avg |Q| and then compare to 0 using absolute tol
          s1 /= 0.5 * (q1.norm() + q2.norm());
        }
      } else if (name == "QSample") {
        V3D q1 = ipws1->getPeak(peakIndex).getQSampleFrame();
        V3D q2 = ipws2->getPeak(peakIndex).getQSampleFrame();
        // using s1 here as the diff
        for (int i = 0; i < 3; ++i) {
          s1 += (q1[i] - q2[i]) * (q1[i] - q2[i]);
        }
        s1 = std::sqrt(s1);
        if (isRelErr) {
          // divide diff by avg |Q| and then compare to 0 using absolute tol
          s1 /= 0.5 * (q1.norm() + q2.norm());
        }
      } else {
        g_log.information() << "Column " << name << " is not compared\n";
      }
      bool mismatch = false;
      // Q: why does it not perform the user-specified operation for QLab and QSample?
      // if this is not necessary, then
      //   bool mismatch = !m_compare(s1, s2)
      // can replace this if/else, and isRelErr and tolerance can be deleted
      if (isRelErr && name != "QLab" && name != "QSample") {
        if (!withinRelativeTolerance(s1, s2, tolerance)) {
          mismatch = true;
        }
      } else if (!withinAbsoluteTolerance(s1, s2, tolerance)) {
        mismatch = true;
      }
      if (mismatch) {
        g_log.notice(name);
        g_log.notice() << "data mismatch in column name = " << name << "\n"
                       << "cell (row#, col#): (" << peakIndex << "," << j << ")\n"
                       << "value1 = " << s1 << "\n"
                       << "value2 = " << s2 << "\n";
        recordMismatch("Data mismatch");
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doTableComparison(const API::ITableWorkspace_const_sptr &tws1,
                                          const API::ITableWorkspace_const_sptr &tws2) {
  // First the easy things
  const auto numCols = tws1->columnCount();
  if (numCols != tws2->columnCount()) {
    g_log.debug() << "Number of columns mismatch (" << numCols << " vs " << tws2->columnCount() << ")\n";
    recordMismatch("Number of columns mismatch");
    return;
  }
  const auto numRows = tws1->rowCount();
  if (numRows != tws2->rowCount()) {
    g_log.debug() << "Number of rows mismatch (" << numRows << " vs " << tws2->rowCount() << ")\n";
    recordMismatch("Number of rows mismatch");
    return;
  }

  for (size_t i = 0; i < numCols; ++i) {
    auto c1 = tws1->getColumn(i);
    auto c2 = tws2->getColumn(i);

    if (c1->name() != c2->name()) {
      g_log.debug() << "Column name mismatch at column " << i << " (" << c1->name() << " vs " << c2->name() << ")\n";
      recordMismatch("Column name mismatch");
      return;
    }
    if (c1->type() != c2->type()) {
      g_log.debug() << "Column type mismatch at column " << i << " (" << c1->type() << " vs " << c2->type() << ")\n";
      recordMismatch("Column type mismatch");
      return;
    }
  }

  const bool checkAllData = getProperty("CheckAllData");
  const bool isRelErr = getProperty("ToleranceRelErr");
  const double tolerance = getProperty("Tolerance");
  bool mismatch;
  for (size_t i = 0; i < numCols; ++i) {
    const auto c1 = tws1->getColumn(i);
    const auto c2 = tws2->getColumn(i);

    if (isRelErr) {
      mismatch = !c1->equalsRelErr(*c2, tolerance);
    } else {
      mismatch = !c1->equals(*c2, tolerance);
    }
    if (mismatch) {
      g_log.debug() << "Table data mismatch at column " << i << "\n";
      recordMismatch("Table data mismatch");
      if (!checkAllData) {
        return;
      }
    }
  } // loop over columns
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doMDComparison(const Workspace_sptr &w1, const Workspace_sptr &w2) {
  IMDWorkspace_sptr mdws1, mdws2;
  mdws1 = std::dynamic_pointer_cast<IMDWorkspace>(w1);
  mdws2 = std::dynamic_pointer_cast<IMDWorkspace>(w2);

  auto alg = createChildAlgorithm("CompareMDWorkspaces");
  alg->setProperty<IMDWorkspace_sptr>("Workspace1", mdws1);
  alg->setProperty<IMDWorkspace_sptr>("Workspace2", mdws2);
  const double tolerance = getProperty("Tolerance");
  alg->setProperty("Tolerance", tolerance);
  alg->executeAsChildAlg();
  bool doesMatch = alg->getProperty("Equals");
  std::string algResult = alg->getProperty("Result");
  if (!doesMatch) {
    recordMismatch(algResult);
  }
}

//------------------------------------------------------------------------------------------------
/**
 * Records a mismatch that has occurred in the output workspace and sets the
 * Result to indicate that the input workspaces did not match.
 *
 * @param msg Mismatch message to be logged in output workspace
 * @param ws1 Name of first workspace being compared
 * @param ws2 Name of second workspace being compared
 */
void CompareWorkspaces::recordMismatch(const std::string &msg, std::string ws1, std::string ws2) {
  // Workspace names default to the workspaces currently being compared
  if (ws1.empty()) {
    Workspace_const_sptr w1 = getProperty("Workspace1");
    ws1 = w1->getName();
  }
  if (ws2.empty()) {
    Workspace_const_sptr w2 = getProperty("Workspace2");
    ws2 = w2->getName();
  }

  // Add new row and flag this comparison as a mismatch
  TableRow row = m_messages->appendRow();
  row << msg << ws1 << ws2;
  m_result = false;
}

//------------------------------------------------------------------------------------------------
/** Function which calculates absolute error between two values and analyses if
this error is within the limits requested.

@param x1    -- first value to check difference
@param x2    -- second value to check difference
@param atol  -- the tolerance of the comparison. Must be nonnegative

@returns true if absolute difference is within the tolerance; false otherwise
*/
bool CompareWorkspaces::withinAbsoluteTolerance(double const x1, double const x2, double const atol) {
  // NOTE !(|x1-x2| > atol) is not the same as |x1-x2| <= atol
  return !(std::abs(x1 - x2) > atol);
}

//------------------------------------------------------------------------------------------------
/** Function which calculates relative error between two values and analyses if
this error is within the limits requested.

@param x1    -- first value to check difference
@param x2    -- second value to check difference
@param rtol  -- the tolerance of the comparison. Must be nonnegative

@returns true if relative difference is within the tolerance; false otherwise
@returns true if error or false if the relative value is within the limits requested
*/
bool CompareWorkspaces::withinRelativeTolerance(double const x1, double const x2, double const rtol) {
  // calculate difference
  double const num = std::abs(x1 - x2);
  // return early if the values are equal
  if (num == 0.0)
    return true;
  // compare the difference to the midpoint value -- could lead to issues for negative values
  double const den = 0.5 * (std::abs(x1) + std::abs(x2));
  if (den <= 1.0 && num > rtol)
    return false;
  // NOTE !(num > rtol*den) is not the same as (num <= rtol*den)
  return !(num > (rtol * den));
}
} // namespace Mantid::Algorithms
