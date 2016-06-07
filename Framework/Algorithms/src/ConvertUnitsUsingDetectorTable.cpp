#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <algorithm>
#include <cfloat>
#include <limits>

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using namespace Kernel;
using namespace API;
using namespace DataObjects;
using boost::function;
using boost::bind;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertUnitsUsingDetectorTable)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertUnitsUsingDetectorTable::ConvertUnitsUsingDetectorTable()
    : Algorithm(), m_numberOfSpectra(0), m_distribution(false),
      m_inputEvents(false) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertUnitsUsingDetectorTable::~ConvertUnitsUsingDetectorTable() = default;

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertUnitsUsingDetectorTable::name() const {
  return "ConvertUnitsUsingDetectorTable";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertUnitsUsingDetectorTable::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertUnitsUsingDetectorTable::category() const {
  return "Utility\\Development";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertUnitsUsingDetectorTable::summary() const {
  return " *** Warning - This Routine is under development *** \n"
         "Performs a unit change on the X values of a workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertUnitsUsingDetectorTable::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "An output workspace.");
  declareProperty("Target", "", boost::make_shared<StringListValidator>(
                                    UnitFactory::Instance().getKeys()),
                  "The name of the units to convert to (must be one of those "
                  "registered in\n"
                  "the Unit Factory)");
  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "DetectorParameters", "", Direction::Input, PropertyMode::Optional),
      "Name of a TableWorkspace containing the detector parameters "
      "to use instead of the IDF.");

  // TODO: Do we need this ?
  declareProperty("AlignBins", false,
                  "If true (default is false), rebins after conversion to "
                  "ensure that all spectra in the output workspace\n"
                  "have identical bin boundaries. This option is not "
                  "recommended (see "
                  "http://www.mantidproject.org/ConvertUnits).");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertUnitsUsingDetectorTable::exec() {
  // Get the workspaces
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  this->setupMemberVariables(inputWS);

  if (m_inputUnit->unitID() == m_outputUnit->unitID()) {
    const std::string outputWSName = getPropertyValue("OutputWorkspace");
    const std::string inputWSName = getPropertyValue("InputWorkspace");
    if (outputWSName == inputWSName) {
      // If it does, just set the output workspace to point to the input one and
      // be done.
      g_log.information() << "Input workspace already has target unit ("
                          << m_outputUnit->unitID()
                          << "), so just pointing the output workspace "
                             "property to the input workspace." << std::endl;
      setProperty("OutputWorkspace",
                  boost::const_pointer_cast<MatrixWorkspace>(inputWS));
      return;
    } else {
      // Clone the workspace.
      IAlgorithm_sptr duplicate =
          createChildAlgorithm("CloneWorkspace", 0.0, 0.6);
      duplicate->initialize();
      duplicate->setProperty("InputWorkspace", inputWS);
      duplicate->execute();
      Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
      auto outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
      setProperty("OutputWorkspace", outputWs);
      return;
    }
  }

  if (inputWS->dataX(0).size() < 2) {
    std::stringstream msg;
    msg << "Input workspace has invalid X axis binning parameters. Should have "
           "at least 2 values. Found " << inputWS->dataX(0).size() << ".";
    throw std::runtime_error(msg.str());
  }
  if (inputWS->dataX(0).front() > inputWS->dataX(0).back() ||
      inputWS->dataX(m_numberOfSpectra / 2).front() >
          inputWS->dataX(m_numberOfSpectra / 2).back())
    throw std::runtime_error("Input workspace has invalid X axis binning "
                             "parameters. X values should be increasing.");

  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);

  // Check whether there is a quick conversion available
  double factor, power;
  if (m_inputUnit->quickConversion(*m_outputUnit, factor, power))
  // If test fails, could also check whether a quick conversion in the opposite
  // direction has been entered
  {
    this->convertQuickly(outputWS, factor, power);
  } else {
    this->convertViaTOF(m_inputUnit, outputWS);
  }

  // If the units conversion has flipped the ascending direction of X, reverse
  // all the vectors
  if (!outputWS->dataX(0).empty() &&
      (outputWS->dataX(0).front() > outputWS->dataX(0).back() ||
       outputWS->dataX(m_numberOfSpectra / 2).front() >
           outputWS->dataX(m_numberOfSpectra / 2).back())) {
    this->reverse(outputWS);
  }

  // Need to lop bins off if converting to energy transfer.
  // Don't do for EventWorkspaces, where you can easily rebin to recover the
  // situation without losing information
  /* This is an ugly test - could be made more general by testing for DBL_MAX
     values at the ends of all spectra, but that would be less efficient */
  if (m_outputUnit->unitID().find("Delta") == 0 && !m_inputEvents)
    outputWS = this->removeUnphysicalBins(outputWS);

  // Rebin the data to common bins if requested, and if necessary
  bool alignBins = getProperty("AlignBins");
  if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS))
    outputWS = this->alignBins(outputWS);

  // If appropriate, put back the bin width division into Y/E.
  if (m_distribution && !m_inputEvents) // Never do this for event workspaces
  {
    this->putBackBinWidth(outputWS);
  }

  // Point the output property to the right place.
  // Do right at end (workspace could could change in removeUnphysicalBins or
  // alignBins methods)
  setProperty("OutputWorkspace", outputWS);
  return;
}

/** Initialise the member variables
 *  @param inputWS The input workspace
 */
void ConvertUnitsUsingDetectorTable::setupMemberVariables(
    const API::MatrixWorkspace_const_sptr inputWS) {
  m_numberOfSpectra = inputWS->getNumberHistograms();
  // In the context of this algorithm, we treat things as a distribution if the
  // flag is set
  // AND the data are not dimensionless
  m_distribution = inputWS->isDistribution() && !inputWS->YUnit().empty();
  // Check if its an event workspace
  m_inputEvents =
      (boost::dynamic_pointer_cast<const EventWorkspace>(inputWS) != nullptr);

  m_inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  m_outputUnit = UnitFactory::Instance().create(targetUnit);
}

/** Create an output workspace of the appropriate (histogram or event) type and
 * copy over the data
 *  @param inputWS The input workspace
 */
API::MatrixWorkspace_sptr ConvertUnitsUsingDetectorTable::setupOutputWorkspace(
    const API::MatrixWorkspace_const_sptr inputWS) {
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // If input and output workspaces are NOT the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }

  if (!m_inputEvents && m_distribution) {
    // Loop over the histograms (detector spectra)
    Progress prog(this, 0.0, 0.2, m_numberOfSpectra);
    PARALLEL_FOR1(outputWS)
    for (int64_t i = 0; i < static_cast<int64_t>(m_numberOfSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION
      // Take the bin width dependency out of the Y & E data
      const auto &X = outputWS->dataX(i);
      auto &Y = outputWS->dataY(i);
      auto &E = outputWS->dataE(i);
      for (size_t j = 0; j < outputWS->blocksize(); ++j) {
        const double width = std::abs(X[j + 1] - X[j]);
        Y[j] *= width;
        E[j] *= width;
      }

      prog.report("Convert to " + m_outputUnit->unitID());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = m_outputUnit;

  return outputWS;
}

/** Convert the workspace units using TOF as an intermediate step in the
 * conversion
 * @param fromUnit :: The unit of the input workspace
 * @param outputWS :: The output workspace
 */
void ConvertUnitsUsingDetectorTable::convertViaTOF(
    Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS) {
  using namespace Geometry;

  // Let's see if we are using a TableWorkspace to override parameters
  TableWorkspace_sptr paramWS = getProperty("DetectorParameters");

  // See if we have supplied a DetectorParameters Workspace
  // TODO: Check if paramWS is NULL and if so throw an exception

  //      const std::string l1ColumnLabel("l1");

  // Let's check all the columns exist and are readable
  try {
    auto spectraColumnTmp = paramWS->getColumn("spectra");
    auto l1ColumnTmp = paramWS->getColumn("l1");
    auto l2ColumnTmp = paramWS->getColumn("l2");
    auto twoThetaColumnTmp = paramWS->getColumn("twotheta");
    auto efixedColumnTmp = paramWS->getColumn("efixed");
    auto emodeColumnTmp = paramWS->getColumn("emode");
  } catch (...) {
    throw Exception::InstrumentDefinitionError(
        "DetectorParameter TableWorkspace is not defined correctly.");
  }

  // Now let's read them into some vectors.
  auto l1Column = paramWS->getColVector<double>("l1");
  auto l2Column = paramWS->getColVector<double>("l2");
  auto twoThetaColumn = paramWS->getColVector<double>("twotheta");
  auto efixedColumn = paramWS->getColVector<double>("efixed");
  auto emodeColumn = paramWS->getColVector<int>("emode");
  auto spectraColumn = paramWS->getColVector<int>("spectra");

  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  // Get the unit object for each workspace
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

  std::vector<double> emptyVec;
  int failedDetectorCount = 0;

  // ConstColumnVector<int> spectraNumber = paramWS->getVector("spectra");

  // TODO: Check why this parallel stuff breaks
  // Loop over the histograms (detector spectra)
  // PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {

    // Lets find what row this spectrum Number appears in our detector table.

    // PARALLEL_START_INTERUPT_REGION

    std::size_t wsid = i;

    try {

      double deg2rad = M_PI / 180.;

      auto det = outputWS->getDetector(i);
      int specNo = det->getID();

      // int spectraNumber = static_cast<int>(spectraColumn->toDouble(i));
      // wsid = outputWS->getIndexFromSpectrumNumber(spectraNumber);
      g_log.debug() << "###### Spectra #" << specNo
                    << " ==> Workspace ID:" << wsid << std::endl;

      // Now we need to find the row that contains this spectrum
      std::vector<int>::iterator specIter;

      specIter = std::find(spectraColumn.begin(), spectraColumn.end(), specNo);
      if (specIter != spectraColumn.end()) {
        size_t detectorRow = std::distance(spectraColumn.begin(), specIter);
        double l1 = l1Column[detectorRow];
        double l2 = l2Column[detectorRow];
        double twoTheta = twoThetaColumn[detectorRow] * deg2rad;
        double efixed = efixedColumn[detectorRow];
        int emode = emodeColumn[detectorRow];

        g_log.debug() << "specNo from detector table = "
                      << spectraColumn[detectorRow] << std::endl;

        // l1 = l1Column->toDouble(detectorRow);
        // l2 = l2Column->toDouble(detectorRow);
        // twoTheta = deg2rad * twoThetaColumn->toDouble(detectorRow);
        // efixed = efixedColumn->toDouble(detectorRow);
        // emode = static_cast<int>(emodeColumn->toDouble(detectorRow));

        g_log.debug() << "###### Spectra #" << specNo
                      << " ==> Det Table Row:" << detectorRow << std::endl;

        g_log.debug() << "\tL1=" << l1 << ",L2=" << l2 << ",TT=" << twoTheta
                      << ",EF=" << efixed << ",EM=" << emode << std::endl;

        // Make local copies of the units. This allows running the loop in
        // parallel
        Unit *localFromUnit = fromUnit->clone();
        Unit *localOutputUnit = outputUnit->clone();
        /// @todo Don't yet consider hold-off (delta)
        const double delta = 0.0;
        // Convert the input unit to time-of-flight
        localFromUnit->toTOF(outputWS->dataX(wsid), emptyVec, l1, l2, twoTheta,
                             emode, efixed, delta);
        // Convert from time-of-flight to the desired unit
        localOutputUnit->fromTOF(outputWS->dataX(wsid), emptyVec, l1, l2,
                                 twoTheta, emode, efixed, delta);
        // EventWorkspace part, modifying the EventLists.
        if (m_inputEvents) {
          eventWS->getEventList(wsid)
              .convertUnitsViaTof(localFromUnit, localOutputUnit);
        }
        // Clear unit memory
        delete localFromUnit;
        delete localOutputUnit;

      } else {
        // Not found
        g_log.debug() << "Spectrum " << specNo << " not found!" << std::endl;
        failedDetectorCount++;
        outputWS->maskWorkspaceIndex(wsid);
      }

    } catch (Exception::NotFoundError &) {
      // Get to here if exception thrown when calculating distance to detector
      failedDetectorCount++;
      // Since you usually (always?) get to here when there's no attached
      // detectors, this call is
      // the same as just zeroing out the data (calling clearData on the
      // spectrum)
      outputWS->maskWorkspaceIndex(i);
    }

    prog.report("Convert to " + m_outputUnit->unitID());
    // PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  // PARALLEL_CHECK_INTERUPT_REGION

  if (failedDetectorCount != 0) {
    g_log.information() << "Something went wrong for " << failedDetectorCount
                        << " spectra. Masking spectrum." << std::endl;
  }
  if (m_inputEvents)
    eventWS->clearMRU();
}

/** Convert the workspace units according to a simple output = a * (input^b)
 * relationship
 *  @param outputWS :: the output workspace
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 */
void ConvertUnitsUsingDetectorTable::convertQuickly(
    API::MatrixWorkspace_sptr outputWS, const double &factor,
    const double &power) {
  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  // See if the workspace has common bins - if so the X vector can be common
  // First a quick check using the validator
  CommonBinsValidator sameBins;
  bool commonBoundaries = false;
  if (sameBins.isValid(outputWS) == "") {
    commonBoundaries = WorkspaceHelpers::commonBoundaries(outputWS);
    // Only do the full check if the quick one passes
    if (commonBoundaries) {
      // Calculate the new (common) X values
      MantidVec::iterator iter;
      for (iter = outputWS->dataX(0).begin(); iter != outputWS->dataX(0).end();
           ++iter) {
        *iter = factor *std::pow(*iter, power);
      }

      MantidVecPtr xVals;
      xVals.access() = outputWS->dataX(0);

      PARALLEL_FOR1(outputWS)
      for (int64_t j = 1; j < numberOfSpectra_i; ++j) {
        PARALLEL_START_INTERUPT_REGION
        outputWS->setX(j, xVals);
        prog.report("Convert to " + m_outputUnit->unitID());
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
      if (!m_inputEvents) // if in event mode the work is done
        return;
    }
  }

  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  // If we get to here then the bins weren't aligned and each spectrum is unique
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int64_t k = 0; k < numberOfSpectra_i; ++k) {
    PARALLEL_START_INTERUPT_REGION
    if (!commonBoundaries) {
      MantidVec::iterator it;
      for (it = outputWS->dataX(k).begin(); it != outputWS->dataX(k).end();
           ++it) {
        *it = factor *std::pow(*it, power);
      }
    }
    // Convert the events themselves if necessary. Inefficiently.
    if (m_inputEvents) {
      eventWS->getEventList(k).convertUnitsQuickly(factor, power);
    }
    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (m_inputEvents)
    eventWS->clearMRU();
  return;
}

/// Calls Rebin as a Child Algorithm to align the bins
API::MatrixWorkspace_sptr
ConvertUnitsUsingDetectorTable::alignBins(API::MatrixWorkspace_sptr workspace) {
  // Create a Rebin child algorithm
  IAlgorithm_sptr childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  // Next line for EventWorkspaces - needed for as long as in/out set same keeps
  // as events.
  childAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", workspace);
  childAlg->setProperty<std::vector<double>>(
      "Params", this->calculateRebinParams(workspace));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/// The Rebin parameters should cover the full range of the converted unit, with
/// the same number of bins
const std::vector<double> ConvertUnitsUsingDetectorTable::calculateRebinParams(
    const API::MatrixWorkspace_const_sptr workspace) const {
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  const size_t numSpec = workspace->getNumberHistograms();
  for (size_t i = 0; i < numSpec; ++i) {
    try {
      Geometry::IDetector_const_sptr det = workspace->getDetector(i);
      if (!det->isMasked()) {
        const MantidVec &XData = workspace->readX(i);
        double xfront = XData.front();
        double xback = XData.back();
        if (boost::math::isfinite(xfront) && boost::math::isfinite(xback)) {
          if (xfront < XMin)
            XMin = xfront;
          if (xback > XMax)
            XMax = xback;
        }
      }
    } catch (Exception::NotFoundError &) {
    } // Do nothing
  }
  const double step =
      (XMax - XMin) / static_cast<double>(workspace->blocksize());

  return {XMin, step, XMax};
}

/** Reverses the workspace if X values are in descending order
 *  @param WS The workspace to operate on
 */
void ConvertUnitsUsingDetectorTable::reverse(API::MatrixWorkspace_sptr WS) {
  if (WorkspaceHelpers::commonBoundaries(WS) && !m_inputEvents) {
    std::reverse(WS->dataX(0).begin(), WS->dataX(0).end());
    std::reverse(WS->dataY(0).begin(), WS->dataY(0).end());
    std::reverse(WS->dataE(0).begin(), WS->dataE(0).end());

    MantidVecPtr xVals;
    xVals.access() = WS->dataX(0);
    for (size_t j = 1; j < m_numberOfSpectra; ++j) {
      WS->setX(j, xVals);
      std::reverse(WS->dataY(j).begin(), WS->dataY(j).end());
      std::reverse(WS->dataE(j).begin(), WS->dataE(j).end());
      if (j % 100 == 0)
        interruption_point();
    }
  } else {
    EventWorkspace_sptr eventWS =
        boost::dynamic_pointer_cast<EventWorkspace>(WS);
    assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

    int m_numberOfSpectra_i = static_cast<int>(m_numberOfSpectra);
    PARALLEL_FOR1(WS)
    for (int j = 0; j < m_numberOfSpectra_i; ++j) {
      PARALLEL_START_INTERUPT_REGION
      if (m_inputEvents) {
        eventWS->getEventList(j).reverse();
      } else {
        std::reverse(WS->dataX(j).begin(), WS->dataX(j).end());
        std::reverse(WS->dataY(j).begin(), WS->dataY(j).end());
        std::reverse(WS->dataE(j).begin(), WS->dataE(j).end());
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }
}

/** Unwieldy method which removes bins which lie in a physically inaccessible
 * region.
 *  This presently only occurs in conversions to energy transfer, where the
 * initial
 *  unit conversion sets them to +/-DBL_MAX. This method removes those bins,
 * leading
 *  to a workspace which is smaller than the input one.
 *  As presently implemented, it unfortunately requires testing for and
 * knowledge of
 *  aspects of the particular units conversion instead of keeping all that in
 * the
 *  units class. It could be made more general, but that would be less
 * efficient.
 *  @param workspace :: The workspace after initial unit conversion
 *  @return The workspace after bins have been removed
 */
API::MatrixWorkspace_sptr ConvertUnitsUsingDetectorTable::removeUnphysicalBins(
    const Mantid::API::MatrixWorkspace_const_sptr workspace) {
  MatrixWorkspace_sptr result;

  const size_t numSpec = workspace->getNumberHistograms();
  const std::string emode = getProperty("Emode");
  if (emode == "Direct") {
    // First the easy case of direct instruments, where all spectra will need
    // the
    // same number of bins removed
    // Need to make sure we don't pick a monitor as the 'reference' X spectrum
    // (X0)
    size_t i = 0;
    for (; i < numSpec; ++i) {
      try {
        Geometry::IDetector_const_sptr det = workspace->getDetector(i);
        if (!det->isMonitor())
          break;
      } catch (Exception::NotFoundError &) { /* Do nothing */
      }
    }
    // Get an X spectrum to search (they're all the same, monitors excepted)
    const MantidVec &X0 = workspace->readX(i);
    auto start = std::lower_bound(X0.cbegin(), X0.cend(), -1.0e-10 * DBL_MAX);
    if (start == X0.cend()) {
      const std::string e("Check the input EFixed: the one given leads to all "
                          "bins being in the physically inaccessible region.");
      g_log.error(e);
      throw std::invalid_argument(e);
    }
    MantidVec::difference_type bins = X0.cend() - start;
    MantidVec::difference_type first = start - X0.cbegin();

    result =
        WorkspaceFactory::Instance().create(workspace, numSpec, bins, bins - 1);

    for (size_t i = 0; i < numSpec; ++i) {
      const MantidVec &X = workspace->readX(i);
      const MantidVec &Y = workspace->readY(i);
      const MantidVec &E = workspace->readE(i);
      result->dataX(i).assign(X.begin() + first, X.end());
      result->dataY(i).assign(Y.begin() + first, Y.end());
      result->dataE(i).assign(E.begin() + first, E.end());
    }
  } else if (emode == "Indirect") {
    // Now the indirect instruments. In this case we could want to keep a
    // different
    // number of bins in each spectrum because, in general L2 is different for
    // each
    // one.
    // Thus, we first need to loop to find largest 'good' range
    std::vector<MantidVec::difference_type> lastBins(numSpec);
    int maxBins = 0;
    for (size_t i = 0; i < numSpec; ++i) {
      const MantidVec &X = workspace->readX(i);
      auto end = std::lower_bound(X.cbegin(), X.cend(), 1.0e-10 * DBL_MAX);
      MantidVec::difference_type bins = end - X.cbegin();
      lastBins[i] = bins;
      if (bins > maxBins)
        maxBins = static_cast<int>(bins);
    }
    g_log.debug() << maxBins << std::endl;
    // Now create an output workspace large enough for the longest 'good' range
    result = WorkspaceFactory::Instance().create(workspace, numSpec, maxBins,
                                                 maxBins - 1);
    // Next, loop again copying in the correct range for each spectrum
    for (int64_t j = 0; j < int64_t(numSpec); ++j) {
      const MantidVec &X = workspace->readX(j);
      const MantidVec &Y = workspace->readY(j);
      const MantidVec &E = workspace->readE(j);
      MantidVec &Xnew = result->dataX(j);
      MantidVec &Ynew = result->dataY(j);
      MantidVec &Enew = result->dataE(j);
      int k;
      for (k = 0; k < lastBins[j] - 1; ++k) {
        Xnew[k] = X[k];
        Ynew[k] = Y[k];
        Enew[k] = E[k];
      }
      Xnew[k] = X[k];
      ++k;
      // If necessary, add on some fake values to the end of the X array (Y&E
      // will be zero)
      if (k < maxBins) {
        for (int l = k; l < maxBins; ++l) {
          Xnew[l] = X[k] + 1 + l - k;
        }
      }
    }
  }

  return result;
}

/** Divide by the bin width if workspace is a distribution
 *  @param outputWS The workspace to operate on
 */
void ConvertUnitsUsingDetectorTable::putBackBinWidth(
    const API::MatrixWorkspace_sptr outputWS) {
  const size_t outSize = outputWS->blocksize();

  for (size_t i = 0; i < m_numberOfSpectra; ++i) {
    for (size_t j = 0; j < outSize; ++j) {
      const double width =
          std::abs(outputWS->dataX(i)[j + 1] - outputWS->dataX(i)[j]);
      outputWS->dataY(i)[j] = outputWS->dataY(i)[j] / width;
      outputWS->dataE(i)[j] = outputWS->dataE(i)[j] / width;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
