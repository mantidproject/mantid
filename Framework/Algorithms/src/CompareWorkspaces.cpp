#include "MantidAlgorithms/CompareWorkspaces.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Unit.h"
#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CompareWorkspaces)

/** Initialize the algorithm's properties.
 */
void CompareWorkspaces::init() {
  declareProperty(make_unique<WorkspaceProperty<Workspace>>("Workspace1", "",
                                                            Direction::Input),
                  "The name of the first input workspace.");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>("Workspace2", "",
                                                            Direction::Input),
                  "The name of the second input workspace.");

  declareProperty(
      "Tolerance", 0.0,
      "The maximum amount by which values may differ between the workspaces.");

  declareProperty("CheckType", true,
                  "Whether to check that the data types "
                  "(Workspace2D vs EventWorkspace) match.");
  declareProperty("CheckAxes", true, "Whether to check that the axes match.");
  declareProperty("CheckSpectraMap", true,
                  "Whether to check that the spectra-detector maps match. ");
  declareProperty("CheckInstrument", true,
                  "Whether to check that the instruments match. ");
  declareProperty("CheckMasking", true,
                  "Whether to check that the bin masking matches. ");

  // Have this one false by default - the logs are brittle
  declareProperty("CheckSample", false,
                  "Whether to check that the sample (e.g. logs).");

  declareProperty(
      "ToleranceRelErr", false,
      "Treat tolerance as relative error rather then the absolute error.\n"
      "This is only applicable to Matrix workspaces.");

  // Have this one false by default - it can be a lot of printing.
  declareProperty("CheckAllData", false,
                  "Usually checking data ends when first mismatch occurs. This "
                  "forces algorithm to check all data and print mismatch to "
                  "the debug log.\n"
                  "Very often such logs are huge so making it true should be "
                  "the last option.");

  declareProperty("NumberMismatchedSpectraToPrint", 1,
                  "Number of mismatched spectra from lowest to be listed. ");

  declareProperty("DetailedPrintIndex", EMPTY_INT(),
                  "Mismatched spectra that will be printed out in details. ");

  declareProperty("Result", false, Direction::Output);
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          "Messages", "compare_msgs", Direction::Output),
      "TableWorkspace containing messages about any mismatches detected");

  m_messages = WorkspaceFactory::Instance().createTable("TableWorkspace");
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

  this->doComparison();

  if (!m_result) {
    std::string message = m_messages->cell<std::string>(0, 0);
    g_log.notice() << "The workspaces did not match: " << message << '\n';
  } else {
    std::string ws1 =
        Workspace_const_sptr(getProperty("Workspace1"))->getName();
    std::string ws2 =
        Workspace_const_sptr(getProperty("Workspace2"))->getName();
    g_log.notice() << "The workspaces \"" << ws1 << "\" and \"" << ws2
                   << "\" matched!\n";
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
  WorkspaceGroup_const_sptr ws1 =
      boost::dynamic_pointer_cast<const WorkspaceGroup>(w1);
  WorkspaceGroup_const_sptr ws2 =
      boost::dynamic_pointer_cast<const WorkspaceGroup>(w2);

  if (ws1 && ws2) { // Both are groups
    processGroups(ws1, ws2);
  } else if (!ws1 && !ws2) { // Neither are groups (shouldn't happen)
    m_result = false;
    throw std::runtime_error("CompareWorkspaces::processGroups - Neither "
                             "input is a WorkspaceGroup. This is a logical "
                             "error in the code.");
  } else if (!ws1 || !ws2) {
    recordMismatch(
        "Type mismatch. One workspace is a group, the other is not.");
  }

  if (m_result && ws1 && ws2) {
    g_log.notice() << "All workspaces in workspace groups \"" << ws1->getName()
                   << "\" and \"" << ws2->getName() << "\" matched!\n";
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
void CompareWorkspaces::processGroups(
    boost::shared_ptr<const API::WorkspaceGroup> groupOne,
    boost::shared_ptr<const API::WorkspaceGroup> groupTwo) {

  // Check their sizes
  const size_t totalNum = static_cast<size_t>(groupOne->getNumberOfEntries());
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
    nonDefaultProps.push_back(p);
  }
  const size_t numNonDefault = nonDefaultProps.size();

  const double progressFraction = 1.0 / static_cast<double>(totalNum);
  std::vector<std::string> namesOne = groupOne->getNames();
  std::vector<std::string> namesTwo = groupTwo->getNames();
  for (size_t i = 0; i < totalNum; ++i) {
    // We should use an algorithm for each so that the output properties are
    // reset properly
    Algorithm_sptr checker = this->createChildAlgorithm(
        this->name(), progressFraction * static_cast<double>(i),
        progressFraction * static_cast<double>(i + 1), false, this->version());
    checker->setPropertyValue("Workspace1", namesOne[i]);
    checker->setPropertyValue("Workspace2", namesTwo[i]);
    for (size_t j = 0; j < numNonDefault; ++j) {
      Property *p = nonDefaultProps[j];
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

  // Check that both workspaces are the same type
  PeaksWorkspace_sptr pws1 = boost::dynamic_pointer_cast<PeaksWorkspace>(w1);
  PeaksWorkspace_sptr pws2 = boost::dynamic_pointer_cast<PeaksWorkspace>(w2);
  if ((pws1 && !pws2) || (!pws1 && pws2)) {
    recordMismatch("One workspace is a PeaksWorkspace and the other is not.");
    return;
  }
  // Check some peak-based stuff
  if (pws1 && pws2) {
    doPeaksComparison(pws1, pws2);
    return;
  }

  // ==============================================================================
  // Table workspaces
  // ==============================================================================

  // Check that both workspaces are the same type
  auto tws1 = boost::dynamic_pointer_cast<const ITableWorkspace>(w1);
  auto tws2 = boost::dynamic_pointer_cast<const ITableWorkspace>(w2);
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
  IMDEventWorkspace_const_sptr mdews1 =
      boost::dynamic_pointer_cast<const IMDEventWorkspace>(w1);
  IMDEventWorkspace_const_sptr mdews2 =
      boost::dynamic_pointer_cast<const IMDEventWorkspace>(w2);
  if ((mdews1 && !mdews2) || (!mdews1 && mdews2)) {
    recordMismatch(
        "One workspace is an IMDEventWorkspace and the other is not.");
    return;
  }
  // Check things for IMDHistoWorkspaces
  IMDHistoWorkspace_const_sptr mdhws1 =
      boost::dynamic_pointer_cast<const IMDHistoWorkspace>(w1);
  IMDHistoWorkspace_const_sptr mdhws2 =
      boost::dynamic_pointer_cast<const IMDHistoWorkspace>(w2);
  if ((mdhws1 && !mdhws2) || (!mdhws1 && mdhws2)) {
    recordMismatch(
        "One workspace is an IMDHistoWorkspace and the other is not.");
    return;
  }

  if (mdhws1 ||
      mdews1) // The '2' workspaces must match because of the checks above
  {
    this->doMDComparison(w1, w2);
    return;
  }

  // ==============================================================================
  // Event workspaces
  // ==============================================================================

  // These casts must succeed or there's a logical problem in the code
  MatrixWorkspace_const_sptr ws1 =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(w1);
  MatrixWorkspace_const_sptr ws2 =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(w2);

  EventWorkspace_const_sptr ews1 =
      boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  EventWorkspace_const_sptr ews2 =
      boost::dynamic_pointer_cast<const EventWorkspace>(ws2);
  if (getProperty("CheckType")) {
    if ((ews1 && !ews2) || (!ews1 && ews2)) {
      recordMismatch(
          "One workspace is an EventWorkspace and the other is not.");
      return;
    }
  }

  size_t numhist = ws1->getNumberHistograms();

  // Fewer steps if not events
  if (ews1 && ews2) {
    // we have to create the progress before the call to compareEventWorkspaces,
    // because it uses the m_progress and it will segfault if not created
    m_progress = make_unique<Progress>(this, 0.0, 1.0, numhist * 5);
    // Compare event lists to see whether 2 event workspaces match each other
    if (!compareEventWorkspaces(*ews1, *ews2))
      return;
  } else {
    m_progress = make_unique<Progress>(this, 0.0, 1.0, numhist * 2);
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
  if (static_cast<bool>(getProperty("CheckSpectraMap")) &&
      !checkSpectraMap(ws1, ws2))
    return;
  m_progress->reportIncrement(numhist / 5, "Instrument");
  if (static_cast<bool>(getProperty("CheckInstrument")) &&
      !checkInstrument(ws1, ws2))
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
bool CompareWorkspaces::compareEventWorkspaces(
    const DataObjects::EventWorkspace &ews1,
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
    g_log.warning() << "Event workspace has unit as "
                    << ews1.getAxis(0)->unit()->label().ascii() << " and "
                    << ews2.getAxis(0)->unit()->label().ascii()
                    << ".  Tolerance of TOF is set to 0.05 still. "
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

  std::vector<int> vec_mismatchedwsindex;
  PARALLEL_FOR_IF(m_parallelComparison && ews1.threadSafe() &&
                  ews2.threadSafe())
  for (int i = 0; i < static_cast<int>(ews1.getNumberHistograms()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    m_progress->report("EventLists");
    if (!mismatchedEvent ||
        checkallspectra) // This guard will avoid checking unnecessarily
    {
      const EventList &el1 = ews1.getSpectrum(i);
      const EventList &el2 = ews2.getSpectrum(i);
      bool printdetail = (i == wsindex2print);
      if (printdetail) {
        g_log.information()
            << "Spectrum " << i << " is set to print out in details. "
            << "\n";
      }

      if (!el1.equals(el2, toleranceTOF, toleranceWeight, tolerancePulse)) {
        size_t tempNumTof = 0;
        size_t tempNumPulses = 0;
        size_t tempNumBoth = 0;

        int tempNumUnequal = 0;

        if (el1.getNumberEvents() != el2.getNumberEvents()) {
          // Number of events are different
          tempNumUnequal = -1;
        } else {
          tempNumUnequal = compareEventsListInDetails(
              el1, el2, toleranceTOF, toleranceWeight, tolerancePulse,
              printdetail, tempNumPulses, tempNumTof, tempNumBoth);
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
          }

          vec_mismatchedwsindex.push_back(i);
        } // Parallel critical region

      } // If elist 1 is not equal to elist 2
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  bool wsmatch;
  if (mismatchedEvent) {
    std::ostringstream mess;
    if (checkallspectra) {
      if (numUnequalNumEventsSpectra > 0)
        mess << "Total " << numUnequalNumEventsSpectra
             << " spectra have different number of events. "
             << "\n";

      mess << "Total " << numUnequalEvents << " (in " << ews1.getNumberEvents()
           << ") events are differrent. " << numUnequalTOFEvents
           << " have different TOF; " << numUnequalPulseEvents
           << " have different pulse time; " << numUnequalBothEvents
           << " have different in both TOF and pulse time. "
           << "\n";

      mess << "Mismatched event lists include " << vec_mismatchedwsindex.size()
           << " of "
           << "total " << ews1.getNumberHistograms() << " spectra. "
           << "\n";

      std::sort(vec_mismatchedwsindex.begin(), vec_mismatchedwsindex.end());
      numspec2print = std::min(numspec2print,
                               static_cast<int>(vec_mismatchedwsindex.size()));
      for (int i = 0; i < numspec2print; ++i) {
        mess << vec_mismatchedwsindex[i] << ", ";
        if ((i + 1) % 10 == 0)
          mess << "\n";
      }
    } else {
      mess << "Quick comparison shows 2 workspaces do not match. "
           << "First found mismatched event list is at workspace index "
           << mismatchedEventWI;
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
bool CompareWorkspaces::checkData(API::MatrixWorkspace_const_sptr ws1,
                                  API::MatrixWorkspace_const_sptr ws2) {
  // Cache a few things for later use
  const size_t numHists = ws1->getNumberHistograms();
  const size_t numBins = ws1->blocksize();
  const bool histogram = ws1->isHistogramData();
  const bool checkAllData = getProperty("CheckAllData");
  const bool RelErr = getProperty("ToleranceRelErr");

  // First check that the workspace are the same size
  if (numHists != ws2->getNumberHistograms() || numBins != ws2->blocksize()) {
    recordMismatch("Size mismatch");
    return false;
  }

  // Check that both are either histograms or point-like data
  if (histogram != ws2->isHistogramData()) {
    recordMismatch("Histogram/point-like mismatch");
    return false;
  }

  const double tolerance = getProperty("Tolerance");
  bool resultBool = true;

  // Now check the data itself
  PARALLEL_FOR_IF(m_parallelComparison && ws1->threadSafe() &&
                  ws2->threadSafe())
  for (long i = 0; i < static_cast<long>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    m_progress->report("Histograms");

    if (resultBool || checkAllData) // Avoid checking unnecessarily
    {
      // Get references to the current spectrum
      const MantidVec &X1 = ws1->readX(i);
      const MantidVec &Y1 = ws1->readY(i);
      const MantidVec &E1 = ws1->readE(i);
      const MantidVec &X2 = ws2->readX(i);
      const MantidVec &Y2 = ws2->readY(i);
      const MantidVec &E2 = ws2->readE(i);

      for (int j = 0; j < static_cast<int>(numBins); ++j) {
        bool err;
        if (RelErr) {
          err = (relErr(X1[j], X2[j], tolerance) ||
                 relErr(Y1[j], Y2[j], tolerance) ||
                 relErr(E1[j], E2[j], tolerance));
        } else
          err = (std::fabs(X1[j] - X2[j]) > tolerance ||
                 std::fabs(Y1[j] - Y2[j]) > tolerance ||
                 std::fabs(E1[j] - E2[j]) > tolerance);

        if (err) {
          g_log.debug() << "Data mismatch at cell (hist#,bin#): (" << i << ","
                        << j << ")\n";
          g_log.debug() << " Dataset #1 (X,Y,E) = (" << X1[j] << "," << Y1[j]
                        << "," << E1[j] << ")\n";
          g_log.debug() << " Dataset #2 (X,Y,E) = (" << X2[j] << "," << Y2[j]
                        << "," << E2[j] << ")\n";
          g_log.debug() << " Difference (X,Y,E) = (" << std::fabs(X1[j] - X2[j])
                        << "," << std::fabs(Y1[j] - Y2[j]) << ","
                        << std::fabs(E1[j] - E2[j]) << ")\n";
          PARALLEL_CRITICAL(resultBool)
          resultBool = false;
        }
      }

      // Extra one for histogram data
      if (histogram && std::fabs(X1.back() - X2.back()) > tolerance) {
        g_log.debug() << " Data ranges mismatch for spectra N: (" << i << ")\n";
        g_log.debug() << " Last bin ranges (X1_end vs X2_end) = (" << X1.back()
                      << "," << X2.back() << ")\n";
        PARALLEL_CRITICAL(resultBool)
        resultBool = false;
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (!resultBool)
    recordMismatch("Data mismatch");
  // If all is well, return true
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
bool CompareWorkspaces::checkAxes(API::MatrixWorkspace_const_sptr ws1,
                                  API::MatrixWorkspace_const_sptr ws2) {
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

    if ((ax1_unit == nullptr && ax2_unit != nullptr) ||
        (ax1_unit != nullptr && ax2_unit == nullptr) ||
        (ax1_unit && ax1_unit->unitID() != ax2_unit->unitID())) {
      recordMismatch(axis_name + " unit mismatch");
      return false;
    }

    // Use Axis's equality operator to check length and values
    // Don't check spectra axis as that just takes it values from the ISpectrum
    // (see checkSpectraMap)
    if (ax1->isNumeric() && ax2->isNumeric()) {
      const NumericAxis *na1 = static_cast<const NumericAxis *>(ax1);
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
    g_log.debug() << "YUnit strings : WS1 = " << ws1->YUnit()
                  << " WS2 = " << ws2->YUnit() << "\n";
    recordMismatch("YUnit mismatch");
    return false;
  }

  // Check both have the same distribution flag
  if (ws1->isDistribution() != ws2->isDistribution()) {
    g_log.debug() << "Distribution flags: WS1 = " << ws1->isDistribution()
                  << " WS2 = " << ws2->isDistribution() << "\n";
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
bool CompareWorkspaces::checkSpectraMap(MatrixWorkspace_const_sptr ws1,
                                        MatrixWorkspace_const_sptr ws2) {
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
      out << "Number of detector IDs mismatch: "
          << spec1.getDetectorIDs().size() << " vs "
          << spec2.getDetectorIDs().size() << " at workspace index " << i;
      recordMismatch(out.str());
      return false;
    }
    auto it2 = spec2.getDetectorIDs().cbegin();
    for (auto it1 = spec1.getDetectorIDs().cbegin();
         it1 != spec1.getDetectorIDs().cend(); ++it1, ++it2) {
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
/// Checks that the instruments match
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The instruments match
/// @retval false The instruments do not match
bool CompareWorkspaces::checkInstrument(API::MatrixWorkspace_const_sptr ws1,
                                        API::MatrixWorkspace_const_sptr ws2) {
  // First check the name matches
  if (ws1->getInstrument()->getName() != ws2->getInstrument()->getName()) {
    g_log.debug() << "Instrument names: WS1 = "
                  << ws1->getInstrument()->getName()
                  << " WS2 = " << ws2->getInstrument()->getName() << "\n";
    recordMismatch("Instrument name mismatch");
    return false;
  }

  if (!ws1->detectorInfo().isEquivalent(ws2->detectorInfo())) {
    recordMismatch("DetectorInfo mismatch (position differences larger than "
                   "1e-9 m or other difference found)");
    return false;
  }

  const Geometry::ParameterMap &ws1_parmap = ws1->constInstrumentParameters();
  const Geometry::ParameterMap &ws2_parmap = ws2->constInstrumentParameters();

  if (ws1_parmap != ws2_parmap) {
    g_log.debug()
        << "Here information to help understand parameter map differences:\n";
    g_log.debug() << ws1_parmap.diff(ws2_parmap);
    recordMismatch(
        "Instrument ParameterMap mismatch (differences in ordering ignored)");
    return false;
  }

  // All OK if we're here
  return true;
}

//------------------------------------------------------------------------------------------------
/// Checks that the masking matches
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The masking matches
/// @retval false The masking does not match
bool CompareWorkspaces::checkMasking(API::MatrixWorkspace_const_sptr ws1,
                                     API::MatrixWorkspace_const_sptr ws2) {
  const int numHists = static_cast<int>(ws1->getNumberHistograms());

  for (int i = 0; i < numHists; ++i) {
    const bool ws1_masks = ws1->hasMaskedBins(i);
    if (ws1_masks != ws2->hasMaskedBins(i)) {
      g_log.debug() << "Only one workspace has masked bins for spectrum " << i
                    << "\n";
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
bool CompareWorkspaces::checkSample(const API::Sample &sample1,
                                    const API::Sample &sample2) {
  if (sample1.getName() != sample2.getName()) {
    g_log.debug() << "WS1 sample name: \"" << sample1.getName() << "\"\n";
    g_log.debug() << "WS2 sample name: \"" << sample2.getName() << "\"\n";
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
bool CompareWorkspaces::checkRunProperties(const API::Run &run1,
                                           const API::Run &run2) {
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

  const std::vector<Kernel::Property *> &ws1logs = run1.getLogData();
  const std::vector<Kernel::Property *> &ws2logs = run2.getLogData();
  // Check that the number of separate logs is the same
  if (ws1logs.size() != ws2logs.size()) {
    g_log.debug() << "WS1 number of logs: " << ws1logs.size() << "\n";
    g_log.debug() << "WS2 number of logs: " << ws2logs.size() << "\n";
    recordMismatch("Different numbers of logs");
    return false;
  }

  // Now loop over the individual logs
  bool matched(true);
  int64_t length(static_cast<int64_t>(ws1logs.size()));
  PARALLEL_FOR_IF(true)
  for (int64_t i = 0; i < length; ++i) {
    PARALLEL_START_INTERUPT_REGION
    if (matched) {
      if (*(ws1logs[i]) != *(ws2logs[i])) {
        matched = false;
        recordMismatch("Log mismatch");
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return matched;
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
int CompareWorkspaces::compareEventsListInDetails(
    const EventList &el1, const EventList &el2, double tolTof, double tolWeight,
    int64_t tolPulse, bool printdetails, size_t &numdiffpulse,
    size_t &numdifftof, size_t &numdiffboth) const {
  // Check
  if (el1.getNumberEvents() != el2.getNumberEvents())
    throw std::runtime_error(
        "compareEventsListInDetails only work on 2 event lists with same "
        "number of events.");

  // Initialize
  numdiffpulse = 0;
  numdifftof = 0;
  numdiffboth = 0;
  int returnint = 0;

  // Compare event by event including all events
  const std::vector<TofEvent> &events1 = el1.getEvents();
  const std::vector<TofEvent> &events2 = el2.getEvents();

  size_t numdiffweight = 0;

  EventType etype = el1.getEventType();

  size_t numevents = events1.size();
  for (size_t i = 0; i < numevents; ++i) {
    // Compare 2 individual events
    const TofEvent &e1 = events1[i];
    const TofEvent &e2 = events2[i];

    bool diffpulse = false;
    bool difftof = false;
    if (std::abs(e1.pulseTime().totalNanoseconds() -
                 e2.pulseTime().totalNanoseconds()) > tolPulse) {
      diffpulse = true;
      ++numdiffpulse;
    }
    if (fabs(e1.tof() - e2.tof()) > tolTof) {
      difftof = true;
      ++numdifftof;
    }
    if (diffpulse && difftof)
      ++numdiffboth;

    if (etype == WEIGHTED) {
      if (fabs(e1.weight() - e2.weight()) > tolWeight)
        ++numdiffweight;
    }

    bool same = (!diffpulse) && (!difftof);
    if (!same) {
      returnint += 1;
      if (printdetails) {
        std::stringstream outss;
        outss << "Spectrum ? Event " << i << ": ";
        if (diffpulse)
          outss << "Diff-Pulse: " << e1.pulseTime() << " vs. " << e2.pulseTime()
                << "; ";
        if (difftof)
          outss << "Diff-TOF: " << e1.tof() << " vs. " << e2.tof() << ";";

        g_log.information(outss.str());
      }
    }
  } // End of loop on all events

  if (numdiffweight > 0) {
    throw std::runtime_error(
        "Detected mismatched events in weight.  Implement this branch ASAP.");
  }

  // Anything that gets this far is equal within tolerances
  return returnint;
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doPeaksComparison(PeaksWorkspace_sptr tws1,
                                          PeaksWorkspace_sptr tws2) {
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
    tws1 = sortPeaks->getProperty("OutputWorkspace");

    sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
    sortPeaks->setProperty("InputWorkspace", tws2);
    sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
    sortPeaks->setProperty("SortAscending", true);
    sortPeaks->executeAsChildAlg();
    tws2 = sortPeaks->getProperty("OutputWorkspace");
  }

  const double tolerance = getProperty("Tolerance");
  for (int i = 0; i < tws1->getNumberPeaks(); i++) {
    const IPeak &peak1 = tws1->getPeak(i);
    const IPeak &peak2 = tws2->getPeak(i);
    for (size_t j = 0; j < tws1->columnCount(); j++) {
      boost::shared_ptr<const API::Column> col = tws1->getColumn(j);
      std::string name = col->name();
      double s1 = 0.0;
      double s2 = 0.0;
      if (name == "runnumber") {
        s1 = double(peak1.getRunNumber());
        s2 = double(peak2.getRunNumber());
      } else if (name == "detid") {
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
      } else if (name == "wavelength") {
        s1 = peak1.getWavelength();
        s2 = peak2.getWavelength();
      } else if (name == "energy") {
        s1 = peak1.getInitialEnergy();
        s2 = peak2.getInitialEnergy();
      } else if (name == "tof") {
        s1 = peak1.getTOF();
        s2 = peak2.getTOF();
      } else if (name == "dspacing") {
        s1 = peak1.getDSpacing();
        s2 = peak2.getDSpacing();
      } else if (name == "intens") {
        s1 = peak1.getIntensity();
        s2 = peak2.getIntensity();
      } else if (name == "sigint") {
        s1 = peak1.getSigmaIntensity();
        s2 = peak2.getSigmaIntensity();
      } else if (name == "bincount") {
        s1 = peak1.getBinCount();
        s2 = peak2.getBinCount();
      } else if (name == "row") {
        s1 = peak1.getRow();
        s2 = peak2.getRow();
      } else if (name == "col") {
        s1 = peak1.getCol();
        s2 = peak2.getCol();
      }
      if (std::fabs(s1 - s2) > tolerance) {
        g_log.debug() << "Data mismatch at cell (row#,col#): (" << i << "," << j
                      << ")\n";
        recordMismatch("Data mismatch");
        return;
      }
    }
  }
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doTableComparison(
    API::ITableWorkspace_const_sptr tws1,
    API::ITableWorkspace_const_sptr tws2) {
  // First the easy things
  const auto numCols = tws1->columnCount();
  if (numCols != tws2->columnCount()) {
    g_log.debug() << "Number of columns mismatch (" << numCols << " vs "
                  << tws2->columnCount() << ")\n";
    recordMismatch("Number of columns mismatch");
    return;
  }
  const auto numRows = tws1->rowCount();
  if (numRows != tws2->rowCount()) {
    g_log.debug() << "Number of rows mismatch (" << numRows << " vs "
                  << tws2->rowCount() << ")\n";
    recordMismatch("Number of rows mismatch");
    return;
  }

  for (size_t i = 0; i < numCols; ++i) {
    auto c1 = tws1->getColumn(i);
    auto c2 = tws2->getColumn(i);

    if (c1->name() != c2->name()) {
      g_log.debug() << "Column name mismatch at column " << i << " ("
                    << c1->name() << " vs " << c2->name() << ")\n";
      recordMismatch("Column name mismatch");
      return;
    }
    if (c1->type() != c2->type()) {
      g_log.debug() << "Column type mismatch at column " << i << " ("
                    << c1->type() << " vs " << c2->type() << ")\n";
      recordMismatch("Column type mismatch");
      return;
    }
  }

  const bool checkAllData = getProperty("CheckAllData");

  for (size_t i = 0; i < numRows; ++i) {
    const TableRow r1 =
        boost::const_pointer_cast<ITableWorkspace>(tws1)->getRow(i);
    const TableRow r2 =
        boost::const_pointer_cast<ITableWorkspace>(tws2)->getRow(i);
    // Easiest, if not the fastest, way to compare is via strings
    std::stringstream r1s, r2s;
    r1s << r1;
    r2s << r2;
    if (r1s.str() != r2s.str()) {
      g_log.debug() << "Table data mismatch at row " << i << " (" << r1s.str()
                    << " vs " << r2s.str() << ")\n";
      recordMismatch("Table data mismatch");
      if (!checkAllData)
        return;
    }
  } // loop over columns
}

//------------------------------------------------------------------------------------------------
void CompareWorkspaces::doMDComparison(Workspace_sptr w1, Workspace_sptr w2) {
  IMDWorkspace_sptr mdws1, mdws2;
  mdws1 = boost::dynamic_pointer_cast<IMDWorkspace>(w1);
  mdws2 = boost::dynamic_pointer_cast<IMDWorkspace>(w2);

  IAlgorithm_sptr alg = this->createChildAlgorithm("CompareMDWorkspaces");
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
void CompareWorkspaces::recordMismatch(std::string msg, std::string ws1,
                                       std::string ws2) {
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
/** Function which calculates relative error between two values and analyses if
this error is within the limits
* requested. When the absolute value of the difference is smaller then the value
of the error requested,
* absolute error is used instead of relative error.

@param x1       -- first value to check difference
@param x2       -- second value to check difference
@param errorVal -- the value of the error, to check against. Should  be large
then 0

@returns true if error or false if the value is within the limits requested
*/
bool CompareWorkspaces::relErr(double x1, double x2, double errorVal) const {
  double num = std::fabs(x1 - x2);
  // how to treat x1<0 and x2 > 0 ?  probably this way
  double den = 0.5 * (std::fabs(x1) + std::fabs(x2));
  if (den < errorVal)
    return (num > errorVal);

  return (num / den > errorVal);
}

Parallel::ExecutionMode CompareWorkspaces::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  using namespace Parallel;
  if (storageModes.at("Workspace1") == StorageMode::Cloned) {
    if (storageModes.at("Workspace2") == StorageMode::Cloned)
      return getCorrespondingExecutionMode(StorageMode::Cloned);
    if (storageModes.at("Workspace2") == StorageMode::MasterOnly)
      return getCorrespondingExecutionMode(StorageMode::MasterOnly);
  }
  if (storageModes.at("Workspace1") == StorageMode::MasterOnly) {
    if (storageModes.at("Workspace2") != StorageMode::Distributed)
      return getCorrespondingExecutionMode(StorageMode::MasterOnly);
  }
  return ExecutionMode::Invalid;
}

void CompareWorkspaces::execMasterOnly() {
  if (communicator().rank() == 0)
    exec();
  else
    setProperty("Result", true);
}

} // namespace Algorithms
} // namespace Mantid
