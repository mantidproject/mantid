// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveBackground.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/VisibleWhenProperty.h"

using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RemoveBackground)

using namespace Kernel;
using namespace API;

/** Initialization method. Declares properties to be used in algorithm.
 *
 */
void RemoveBackground::init() {
  auto sourceValidator = boost::make_shared<CompositeValidator>();
  sourceValidator->add<InstrumentValidator>();
  sourceValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, sourceValidator),
                  "Workspace containing the input data");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to give the output workspace");

  auto vsValidator = boost::make_shared<CompositeValidator>();
  vsValidator->add<WorkspaceUnitValidator>("TOF");
  vsValidator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("BkgWorkspace", "",
                                            Direction::Input, vsValidator),
      "An optional histogram workspace in the units of TOF defining background "
      "for removal during rebinning."
      "The workspace has to have single value or contain the same number of "
      "spectra as the \"InputWorkspace\" and single Y value per each spectra,"
      "representing flat background in the background time region. "
      "If such workspace is present, the value of the flat background provided "
      "by this workspace is removed "
      "from each spectra of the rebinned workspace. This works for histogram "
      "and event workspace when events are not retained "
      "but actually useful mainly for removing background while rebinning an "
      "event workspace in the units different from TOF.");

  std::vector<std::string> dE_modes = Kernel::DeltaEMode::availableTypes();
  declareProperty("EMode", dE_modes[Kernel::DeltaEMode::Direct],
                  boost::make_shared<Kernel::StringListValidator>(dE_modes),
                  "The energy conversion mode used to define the conversion "
                  "from the units of the InputWorkspace to TOF",
                  Direction::Input);
  declareProperty("NullifyNegativeValues", false,
                  "When background is subtracted, signals in some time "
                  "channels may become negative.\n"
                  "If this option is true, signal in such bins is nullified "
                  "and the module of the removed signal"
                  "is added to the error. If false, the negative signal and "
                  "correspondent errors remain unchanged",
                  Direction::Input);
}

/** Executes the rebin algorithm
 *
 *  @throw runtime_error Thrown if the bin range does not intersect the range of
 *the input workspace
 */
void RemoveBackground::exec() {

  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  API::MatrixWorkspace_const_sptr bkgWksp = getProperty("BkgWorkspace");
  bool nullifyNegative = getProperty("NullifyNegativeValues");

  if (!(bkgWksp->getNumberHistograms() == 1 ||
        inputWS->getNumberHistograms() == bkgWksp->getNumberHistograms())) {
    throw std::invalid_argument(" Background Workspace: " + bkgWksp->getName() +
                                " should have the same number of spectra as "
                                "source workspace or be a single histogram "
                                "workspace");
  }

  int eMode; // in convert units emode is still integer
  const std::string emodeStr = getProperty("EMode");
  eMode = static_cast<int>(Kernel::DeltaEMode::fromString(emodeStr));

  // Removing background in-place
  bool inPlace = (inputWS == outputWS);
  // workspace independent determination of length
  const int histnumber = static_cast<int>(inputWS->getNumberHistograms());

  if (!inPlace) {
    // make the copy of the output Workspace from the input. Also copies
    // X-vectors and axis
    outputWS = API::WorkspaceFactory::Instance().create(inputWS);
  }

  int nThreads = PARALLEL_GET_MAX_THREADS;
  m_BackgroundHelper.initialize(bkgWksp, inputWS, eMode, &g_log, nThreads,
                                inPlace, nullifyNegative);

  Progress prog(this, 0.0, 1.0, histnumber);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int hist = 0; hist < histnumber; ++hist) {
    PARALLEL_START_INTERUPT_REGION
    // get references to output Workspace X-arrays.
    auto &XValues = outputWS->mutableX(hist);
    // get references to output workspace data and error. If it is new
    // workspace, data will be copied there, if old, modified in-place
    auto &YValues = outputWS->mutableY(hist);
    auto &YErrors = outputWS->mutableE(hist);

    // output data arrays are implicitly filled by function
    int id = PARALLEL_THREAD_NUMBER;
    m_BackgroundHelper.removeBackground(hist, XValues, YValues, YErrors, id);

    prog.report(name());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWS);
}
//-------------------------------------------------------------------------------------------------------------------------------
//----------------     BACKGROUND HELPER
//---------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
/// Constructor
BackgroundHelper::BackgroundHelper()
    : m_WSUnit(), m_bgWs(), m_wkWS(), m_spectrumInfo(nullptr), m_pgLog(nullptr),
      m_inPlace(true), m_singleValueBackground(false), m_NBg(0), m_dtBg(1),
      m_ErrSq(0), m_Emode(0), m_Efix(0), m_nullifyNegative(false),
      m_previouslyRemovedBkgMode(false) {}

/** Initialization method:
*@param bkgWS    -- shared pointer to the workspace which contains background
*@param sourceWS -- shared pointer to the workspace to remove background from
*@param emode    -- energy conversion mode used during internal units conversion
*           (0 -- elastic, 1-direct, 2 indirect, as defined in Units conversion
*@param pLog     -- pointer to the logger class which would report errors
*@param nThreads -- number of threads to be used for background removal
*@param inPlace  -- if the background removal occurs from the existing workspace
*@param nullifyNegative -- if true, negative signals are nullified and error is
*                          modified appropriately
or target workspace has to be cloned.
*/
void BackgroundHelper::initialize(const API::MatrixWorkspace_const_sptr &bkgWS,
                                  const API::MatrixWorkspace_sptr &sourceWS,
                                  int emode, Kernel::Logger *pLog, int nThreads,
                                  bool inPlace, bool nullifyNegative) {
  m_bgWs = bkgWS;
  m_wkWS = sourceWS;
  m_Emode = emode;
  m_pgLog = pLog;
  m_inPlace = inPlace;
  m_nullifyNegative = nullifyNegative;

  std::string bgUnits = bkgWS->getAxis(0)->unit()->unitID();
  if (bgUnits != "TOF")
    throw std::invalid_argument(" Background Workspace: " + bkgWS->getName() +
                                " should be in the units of TOF");

  if (!(bkgWS->getNumberHistograms() == 1 ||
        sourceWS->getNumberHistograms() == bkgWS->getNumberHistograms()))
    throw std::invalid_argument(" Background Workspace: " + bkgWS->getName() +
                                " should have the same number of spectra as "
                                "source workspace or be a single histogram "
                                "workspace");

  auto WSUnit = sourceWS->getAxis(0)->unit();
  if (!WSUnit)
    throw std::invalid_argument(" Source Workspace: " + sourceWS->getName() +
                                " should have units");

  m_spectrumInfo = &sourceWS->spectrumInfo();

  // allocate the array of units converters to avoid units reallocation within a
  // loop
  m_WSUnit.clear();
  m_WSUnit.reserve(nThreads);
  for (int i = 0; i < nThreads; i++) {
    m_WSUnit.emplace_back(std::unique_ptr<Kernel::Unit>(WSUnit->clone()));
  }

  m_singleValueBackground = false;
  if (bkgWS->getNumberHistograms() <= 1)
    m_singleValueBackground = true;

  auto &dataX = bkgWS->x(0);
  auto &dataY = bkgWS->y(0);
  auto &dataE = bkgWS->e(0);
  m_NBg = dataY[0];
  m_dtBg = dataX[1] - dataX[0];
  m_ErrSq = dataE[0] * dataE[0]; // needs further clarification

  m_previouslyRemovedBkgMode = false;
  if (m_singleValueBackground) {
    if (m_NBg < 1.e-7 && m_ErrSq < 1.e-7)
      m_previouslyRemovedBkgMode = true;
  }

  m_Efix = this->getEi(sourceWS);
}
/**Method removes background from vectors which represent a histogram data for a
 * single spectra
 * @param nHist   -- number (workspaceID) of the spectra in the workspace, where
 * background going to be removed
 * @param x_data  -- the spectra x-values (presumably not in TOF units)
 * @param y_data  -- the spectra signal
 * @param e_data  -- the spectra errors
 * @param threadNum -- the number of thread doing conversion (by default 0,
 * single thread, in multithreading -- result of
 *                      omp_get_thread_num() )
 */
void BackgroundHelper::removeBackground(int nHist, HistogramX &x_data,
                                        HistogramY &y_data, HistogramE &e_data,
                                        int threadNum) const {

  double dtBg, IBg, ErrBgSq;
  if (m_singleValueBackground) {
    dtBg = m_dtBg;
    ErrBgSq = m_ErrSq;
    IBg = m_NBg;
  } else {
    auto &dataX = m_bgWs->x(nHist);
    auto &dataY = m_bgWs->y(nHist);
    auto &dataE = m_bgWs->e(nHist);
    dtBg = (dataX[1] - dataX[0]);
    IBg = dataY[0];
    ErrBgSq = dataE[0] * dataE[0]; // Needs further clarification
  }

  try {
    double twoTheta = m_spectrumInfo->twoTheta(nHist);
    double L1 = m_spectrumInfo->l1();
    double L2 = m_spectrumInfo->l2(nHist);
    double delta(std::numeric_limits<double>::quiet_NaN());
    // get access to source workspace in case if target is different from source
    auto &XValues = m_wkWS->x(nHist);
    auto &YValues = m_wkWS->y(nHist);
    auto &YErrors = m_wkWS->e(nHist);

    // use thread-specific unit conversion class to avoid multithreading issues
    Kernel::Unit *unitConv = m_WSUnit[threadNum].get();
    unitConv->initialize(L1, L2, twoTheta, m_Emode, m_Efix, delta);

    x_data[0] = XValues[0];
    double tof1 = unitConv->singleToTOF(x_data[0]);
    for (size_t i = 0; i < y_data.size(); i++) {
      double X = XValues[i + 1];
      double tof2 = unitConv->singleToTOF(X);
      double Jack = std::fabs((tof2 - tof1) / dtBg);
      double normBkgrnd = IBg * Jack;
      double errBkgSq = ErrBgSq * Jack * Jack;
      tof1 = tof2;
      if (m_inPlace) {
        y_data[i] -= normBkgrnd;
        // e_data[i]  =std::sqrt(ErrSq*Jack*Jack+e_data[i]*e_data[i]); //
        // needs further clarification -- Gaussian error summation?
        //--> assume error for background is sqrt(signal):
        e_data[i] = std::sqrt((errBkgSq + e_data[i] * e_data[i]));
      } else {
        x_data[i + 1] = X;
        y_data[i] = YValues[i] - normBkgrnd;
        e_data[i] = std::sqrt((errBkgSq + YErrors[i] * YErrors[i]));
      }
      if (m_nullifyNegative && y_data[i] < 0) {
        if (m_previouslyRemovedBkgMode) {
          // background have been removed
          // and not we remove negative signal and estimate errors
          double prev_rem_bkg = -y_data[i];
          e_data[i] = e_data[i] > prev_rem_bkg ? e_data[i] : prev_rem_bkg;
        } else {
          /** The error estimate must go up in this nonideal situation and the
              value of background is a good estimate for it. However, don't
              reduce the error if it was already more than that */
          e_data[i] = e_data[i] > normBkgrnd ? e_data[i] : normBkgrnd;
        }
        y_data[i] = 0;
      }
    }

  } catch (...) {
    // no background removal for this spectra as it does not have a detector or
    // other reason
    if (m_pgLog)
      m_pgLog->debug()
          << " Can not remove background for the spectra with number (id)"
          << nHist;
  }
}
/** Method returns the efixed or Ei value stored in properties of the input
 *workspace.
 *  Indirect instruments can have eFxed and Direct instruments can have Ei
 *defined as the properties of the workspace.
 *
 *  This method provide guess for efixed for all other kind of instruments.
 *Correct indirect instrument will overwrite
 *  this value while wrongly defined or different types of instruments will
 *provide the value of "Ei" property (log value)
 *  or undefined if "Ei" property is not found.
 *
 */
double
BackgroundHelper::getEi(const API::MatrixWorkspace_const_sptr &inputWS) const {
  double Efi = std::numeric_limits<double>::quiet_NaN();

  // is Ei on workspace properties? (it can be defined for some reason if
  // detectors do not have one, and then it would exist as Ei)
  bool EiFound(false);
  try {
    Efi = inputWS->run().getPropertyValueAsType<double>("Ei");
    EiFound = true;
  } catch (Kernel::Exception::NotFoundError &) {
  }
  // try to get Efixed as property on a workspace, obtained for indirect
  // instrument
  // bool eFixedFound(false);
  if (!EiFound) {
    try {
      Efi = inputWS->run().getPropertyValueAsType<double>("eFixed");
      // eFixedFound = true;
    } catch (Kernel::Exception::NotFoundError &) {
    }
  }

  // if (!(EiFound||eFixedFound))
  //  g_log.debug()<<" Ei/eFixed requested but have not been found\n";

  return Efi;
}

} // namespace Algorithms
} // namespace Mantid
