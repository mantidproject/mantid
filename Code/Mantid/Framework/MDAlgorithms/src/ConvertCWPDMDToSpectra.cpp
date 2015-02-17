#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ExperimentInfo.h" // /ExpInfo.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

double calculate2Theta(const Kernel::V3D &detpos,
                       const Kernel::V3D &samplepos) {
  return detpos.angle(samplepos);
}

DECLARE_ALGORITHM(ConvertCWPDMDToSpectra)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertCWPDMDToSpectra::ConvertCWPDMDToSpectra() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertCWPDMDToSpectra::~ConvertCWPDMDToSpectra() {}

//----------------------------------------------------------------------------------------------
void ConvertCWPDMDToSpectra::init() {

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "Name of the input MDEventWorkspace that stores detectors "
                  "counts from a constant-wave powder diffraction experiment.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "InputMonitorWorkspace", "", Direction::Input),
                  "Name of the input MDEventWorkspace that stores monitor "
                  "counts from a constant-wave powder diffraciton experiment.");

  declareProperty(
      new ArrayProperty<double>("BinningParams"),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally\n"
      "this can be followed by a comma and more widths and last boundary "
      "pairs.\n"
      "Negative width values indicate logarithmic binning.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output workspace for reduced data.");

  std::vector<std::string> vecunits;
  vecunits.push_back("2-theta");
  vecunits.push_back("Momenum Transfer (Q)");
  auto unitval = boost::make_shared<ListValidator<std::string> >(vecunits);
  declareProperty("UnitOutput", "2-theta", unitval,
                  "Unit of the output workspace.");

  declareProperty("ScaleFactor", 1.0,
                  "Scaling factor on the normalized counts.");

  declareProperty("LinearInterpolateZeroCounts", true,
                  "If set to true and if a bin has zero count, a linear "
                  "interpolation will be made to set the value of this bin. It "
                  "is applied to the case that the bin size is small. ");
}

//----------------------------------------------------------------------------------------------
void ConvertCWPDMDToSpectra::exec() {
  // Process input workspaces
  // input data workspace
  IMDEventWorkspace_sptr inputDataWS = getProperty("InputWorkspace");
  // input monitor workspace
  IMDEventWorkspace_sptr inputMonitorWS = getProperty("InputMonitorWorkspace");
  // input binning parameters
  const std::vector<double> binParams = getProperty("BinningParams");
  // scale factor
  double scaleFactor = getProperty("ScaleFactor");
  // do linear interpolation
  bool doLinearInterpolation = getProperty("LinearInterpolateZeroCounts");

  // Validate inputs
  size_t numdataevents = inputDataWS->getNEvents();
  size_t nummonitorevents = inputMonitorWS->getNEvents();
  if (numdataevents != nummonitorevents)
    throw std::runtime_error("Input data workspace and monitor workspace have "
                             "different number of MDEvents.");

  if (binParams.size() != 3)
    throw std::runtime_error("Binning parameters must have 3 items.");
  if (binParams[0] >= binParams[2])
    throw std::runtime_error(
        "Min value of the bin must be smaller than maximum value.");

  // Rebin
  API::MatrixWorkspace_sptr outws =
      reducePowderData(inputDataWS, inputMonitorWS, binParams[0], binParams[2],
                       binParams[1], doLinearInterpolation);

  // Scale
  scaleMatrixWorkspace(outws, scaleFactor);

  // Set up the sample logs
  setupSampleLogs(outws, inputDataWS);

  // Return
  setProperty("OutputWorkspace", outws);
}

//---------------------------------------------------------------------------------
/** Reduce the 2 MD workspaces to a workspace2D for powder diffraction pattern
 * Reduction procedure
 * 1. set up bins
 * 2. loop around all the MD event
 * 3. For each MD event, find out its 2theta value and add its signal and
 * monitor counts to the correct bin
 * 4. For each bin, normalize the sum of the signal by sum of monitor counts
 * @brief LoadHFIRPDD::reducePowderData
 * @param dataws
 * @param monitorws
 */
API::MatrixWorkspace_sptr ConvertCWPDMDToSpectra::reducePowderData(
    API::IMDEventWorkspace_const_sptr dataws,
    IMDEventWorkspace_const_sptr monitorws, const double min2theta,
    const double max2theta, const double binsize, bool dolinearinterpolation) {
  // Get some information
  int64_t numevents = dataws->getNEvents();
  g_log.notice() << "[DB] Number of events = " << numevents << "\n";

  // Create bins in 2theta (degree)
  size_t sizex, sizey;
  sizex = static_cast<size_t>((max2theta - min2theta) / binsize + 0.5);
  sizey = sizex - 1;
  g_log.notice() << "[DB] "
                 << "SizeX = " << sizex << ", "
                 << "SizeY = " << sizey << "\n";
  std::vector<double> vecx(sizex), vecy(sizex - 1, 0), vecm(sizex - 1, 0),
      vece(sizex - 1, 0);
  std::vector<bool> veczerocounts(sizex - 1, false);

  for (size_t i = 0; i < sizex; ++i)
    vecx[i] = min2theta + static_cast<double>(i) * binsize;

  binMD(dataws, vecx, vecy);
  binMD(monitorws, vecx, vecm);

  // Normalize by division
  for (size_t i = 0; i < vecm.size(); ++i) {
    if (vecy[i] < 1.0E-5)
      veczerocounts[i] = true;
    if (vecm[i] >= 1.) {
      double y = vecy[i];
      double ey = sqrt(y);
      double m = vecm[i];
      double em = sqrt(m);
      vecy[i] = y / m;
      // using standard deviation's error propagation
      vece[i] = vecy[i] * sqrt((ey / y) * (ey / y) + (em / m) * (em / m));
    } else {
      vecy[i] = 0.0;
      vece[i] = 1.0;
    }
  }
  // error
  g_log.error("How to calculate the error bar?");

  /*
  coord_t pos0 = mditer->getInnerPosition(0, 0);
  coord_t posn = mditer->getInnerPosition(numev2 - 1, 0);
  g_log.notice() << "[DB] "
                 << " pos0 = " << pos0 << ", "
                 << " pos1 = " << posn << "\n";
                 */

  // Create workspace
  API::MatrixWorkspace_sptr pdws =
      WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);

  // Interpolation
  if (dolinearinterpolation)
    linearInterpolation(pdws, veczerocounts);

  return pdws;
}

//----------------------------------------------------------------------------------------------
/** Bin MD Workspace for detector's position at 2theta
 * @brief LoadHFIRPDD::binMD
 * @param mdws
 * @param vecx
 * @param vecy
 */
void ConvertCWPDMDToSpectra::binMD(API::IMDEventWorkspace_const_sptr mdws,
                                   const std::vector<double> &vecx,
                                   std::vector<double> &vecy) {
  // Check whether MD workspace has proper instrument and experiment Info
  if (mdws->getNumExperimentInfo() == 0)
    throw std::runtime_error(
        "There is no ExperimentInfo object that has been set to "
        "input MDEventWorkspace!");
  else
    g_log.information()
        << "Number of ExperimentInfo objects of MDEventWrokspace is "
        << mdws->getNumExperimentInfo() << "\n";

  // Get sample position
  ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(0);
  Geometry::IComponent_const_sptr sample =
      expinfo->getInstrument()->getSample();
  const V3D samplepos = sample->getPos();
  g_log.notice() << "[DB] Sample position is " << samplepos.X() << ", "
                 << samplepos.Y() << ", " << samplepos.Z() << "\n";

  Geometry::IComponent_const_sptr source =
      expinfo->getInstrument()->getSource();
  const V3D sourcepos = source->getPos();
  g_log.notice() << "[DB] Source position is " << sourcepos.X() << ","
                 << sourcepos.Y() << ", " << sourcepos.Z() << "\n";

  // Go through all events to find out their positions
  IMDIterator *mditer = mdws->createIterator();
  bool scancell = true;
  size_t nextindex = 1;
  while (scancell) {
    // get the number of events of this cell
    size_t numev2 = mditer->getNumEvents();
    g_log.notice() << "[DB] Cell " << nextindex - 1
                   << ": Number of events = " << numev2
                   << " Does NEXT cell exist = " << mditer->next() << "\n";

    // loop over all the events in current cell
    for (size_t iev = 0; iev < numev2; ++iev) {
      // get detector position in 2theta and signal
      double tempx = mditer->getInnerPosition(iev, 0);
      double tempy = mditer->getInnerPosition(iev, 1);
      double tempz = mditer->getInnerPosition(iev, 2);
      Kernel::V3D detpos(tempx, tempy, tempz);
      Kernel::V3D v_det_sample = detpos - samplepos;
      Kernel::V3D v_sample_src = samplepos - sourcepos;

      double twotheta =
          calculate2Theta(v_det_sample, v_sample_src) / M_PI * 180.;
      double signal = mditer->getInnerSignal(iev);

      // assign signal to bin
      std::vector<double>::const_iterator vfiter =
          std::lower_bound(vecx.begin(), vecx.end(), twotheta);
      int xindex = static_cast<int>(vfiter - vecx.begin());
      if (xindex < 0)
        g_log.warning("xindex < 0");
      if (xindex >= static_cast<int>(vecy.size()) - 1) {
        g_log.error() << "This is the bug! "
                      << "xindex = " << xindex << " 2theta = " << twotheta
                      << " out of [" << vecx.front() << ", " << vecx.back()
                      << "]. dep pos = " << detpos.X() << ", " << detpos.Y()
                      << ", " << detpos.Z()
                      << "; sample pos = " << samplepos.X() << ", "
                      << samplepos.Y() << ", " << samplepos.Z() << "\n";
        continue;
      }

      if (xindex > 0 && twotheta < *vfiter)
        xindex -= 1;
      vecy[xindex] += signal;
    }

    // Advance to next cell
    if (mditer->next()) {
      // advance to next cell
      mditer->jumpTo(nextindex);
      ++nextindex;
    } else {
      // break the loop
      scancell = false;
    }
  } // ENDOF(while)

  return;
}

/** Do linear interpolation to bins with zero counts
 * @brief ConvertCWPDMDToSpectra::doLinearInterpolation
 * @param matrixws
 */
void
ConvertCWPDMDToSpectra::linearInterpolation(API::MatrixWorkspace_sptr matrixws,
                                            std::vector<bool> &vec0count) {
  throw std::runtime_error("Need to implement ASAP.");

  return;
}

/** Set up sample logs from input data MDWorkspace
 * @brief ConvertCWPDMDToSpectra::setupSampleLogs
 * @param matrixws
 * @param inputmdws
 */
void ConvertCWPDMDToSpectra::setupSampleLogs(
    API::MatrixWorkspace_sptr matrixws,
    API::IMDEventWorkspace_const_sptr inputmdws) {
  // get hold of the last experiment info from md workspace to copy over
  size_t numexpinfo = inputmdws->getNumExperimentInfo();
  ExperimentInfo_const_sptr lastexpinfo =
      inputmdws->getExperimentInfo(numexpinfo - 1);

  // get hold of experiment info from matrix ws
  Run targetrun = matrixws->mutableRun();
  const Run &srcrun = lastexpinfo->run();

  const std::vector<Kernel::Property *> &vec_srcprop = srcrun.getProperties();
  for (size_t i = 0; i < vec_srcprop.size(); ++i) {
    Property *p = vec_srcprop[i];
    targetrun.addProperty(p->clone());
  }

  return;
}

/** Scale up the values of matrix workspace
 * @brief ConvertCWPDMDToSpectra::scaleMatrixWorkspace
 * @param matrixws
 * @param scalefactor
 */
void
ConvertCWPDMDToSpectra::scaleMatrixWorkspace(API::MatrixWorkspace_sptr matrixws,
                                             const double &scalefactor) {
  size_t numspec = matrixws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    MantidVec &datay = matrixws->dataY(iws);
    MantidVec &datae = matrixws->dataE(iws);
    size_t numelements = datay.size();
    for (size_t i = 0; i < numelements; ++i) {
      datay[i] *= scalefactor;
      datae[i] *= sqrt(scalefactor);
    }
  } // FOR(iws)

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
