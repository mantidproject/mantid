#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ExperimentInfo.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

DECLARE_ALGORITHM(ConvertCWPDMDToSpectra)

const double BIGNUMBER = 1.0E100;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertCWPDMDToSpectra::ConvertCWPDMDToSpectra() : m_infitesimal(1.0E-10) {}

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
  vecunits.push_back("2theta");
  vecunits.push_back("dSpacing");
  vecunits.push_back("Momentum Transfer (Q)");
  auto unitval = boost::make_shared<ListValidator<std::string> >(vecunits);
  declareProperty("UnitOutput", "2theta", unitval,
                  "Unit of the output workspace.");

  declareProperty("NeutronWaveLength", EMPTY_DBL(),
                  "Constant wavelength of the neutrons from reactor source.");

  declareProperty(
      "NeutornWaveLengthPropertyName", "wavelength",
      "Property name of the neutron wavelength in the sample log."
      "If output unit is other than 2theta and NeutronWaveLength is not given,"
      "then the neutron wavelength will be searched in sample logs by "
      "name specified by this property.");

  declareProperty("ScaleFactor", 1.0,
                  "Scaling factor on the normalized counts.");

  declareProperty(new ArrayProperty<int>("ExcludedDetectorIDs"),
                  "A comma separated list of integers to indicate the IDs of "
                  "the detectors that will be excluded from binning.");

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
  // unit
  std::string outputunit = getProperty("UnitOutput");
  double wavelength = getProperty("NeutronWaveLength");

  std::vector<detid_t> excluded_detids = getProperty("ExcludedDetectorIDs");

  // Validate inputs
  // input data workspace and monitor workspace should match
  size_t numdataevents = inputDataWS->getNEvents();
  size_t nummonitorevents = inputMonitorWS->getNEvents();
  if (numdataevents != nummonitorevents)
    throw std::runtime_error("Input data workspace and monitor workspace have "
                             "different number of MDEvents.");

  // output unit: make a map for wavelength
  std::map<int, double> map_runWavelength;
  if (outputunit.compare("2theta")) {
    // set up runid and wavelength  map
    std::string wavelengthpropertyname =
        getProperty("NeutornWaveLengthPropertyName");

    uint16_t numexpinfo = inputDataWS->getNumExperimentInfo();
    for (uint16_t iexp = 0; iexp < numexpinfo; ++iexp) {
      int runid = atoi(inputDataWS->getExperimentInfo(iexp)
                           ->run()
                           .getProperty("run_number")
                           ->value()
                           .c_str());
      // skip if run id is not a valid one
      if (runid < 0)
        continue;
      double thislambda = wavelength;
      if (inputDataWS->getExperimentInfo(iexp)->run().hasProperty(
              wavelengthpropertyname))
        thislambda = atof(inputDataWS->getExperimentInfo(iexp)
                              ->run()
                              .getProperty(wavelengthpropertyname)
                              ->value()
                              .c_str());
      else if (wavelength == EMPTY_DBL()) {
        std::stringstream errss;
        errss << "In order to convert unit to " << outputunit
              << ", either NeutronWaveLength "
                 " is to be specified or property " << wavelengthpropertyname
              << " must exist for run " << runid << ".";
        throw std::runtime_error(errss.str());
      }
      map_runWavelength.insert(std::make_pair(runid, thislambda));
    }
  }

  // bin parameters
  double xmin, xmax, binsize;
  xmin = xmax = binsize = -1;
  if (binParams.size() == 1) {
    binsize = binParams[0];
    g_log.warning()
        << "Only bin size " << binParams[0]
        << " is specified.  Xmin and Xmax "
           " will be calcualted from motor positions and wavelength.  "
           "More CPU time will be used."
        << "\n";
  } else if (binParams.size() == 3) {
    xmin = binParams[0];
    binsize = binParams[1];
    xmax = binParams[2];
    if (xmin >= xmax)
      throw std::runtime_error(
          "Min value of the bin must be smaller than maximum value.");
  } else {
    // Either 1 or 3 parameters.  Throw exception
    throw std::runtime_error(
        "Binning parameters must have either 1 or 3 items.");
  }

  // Rebin
  API::MatrixWorkspace_sptr outws = reducePowderData(
      inputDataWS, inputMonitorWS, outputunit, map_runWavelength, xmin, xmax,
      binsize, doLinearInterpolation, excluded_detids);

  // Scale
  scaleMatrixWorkspace(outws, scaleFactor, m_infitesimal);

  // Set up the sample logs
  setupSampleLogs(outws, inputDataWS);

  // Return
  setProperty("OutputWorkspace", outws);
}

//----------------------------------------------------------------------------------------------
/** Reduce the 2 MD workspaces to a workspace2D for powder diffraction pattern
 * Reduction procedure
 * 1. set up bins
 * 2. loop around all the MD event
 * 3. For each MD event, find out its 2theta value and add its signal and
 * monitor counts to the correct bin
 * 4. For each bin, normalize the sum of the signal by sum of monitor counts
 * @brief ConvertCWPDMDToSpectra::reducePowderData
 * @param dataws
 * @param monitorws
 * @param targetunit
 * @param map_runwavelength
 * @param xmin
 * @param xmax
 * @param binsize
 * @param dolinearinterpolation
 * @param vec_excludeddets :: vector of IDs of detectors to be excluded
 * @return
 */
API::MatrixWorkspace_sptr ConvertCWPDMDToSpectra::reducePowderData(
    API::IMDEventWorkspace_const_sptr dataws,
    IMDEventWorkspace_const_sptr monitorws, const std::string targetunit,
    const std::map<int, double> &map_runwavelength, const double xmin,
    const double xmax, const double binsize, bool dolinearinterpolation,
    const std::vector<detid_t> &vec_excludeddets) {
  // Get some information
  int64_t numevents = dataws->getNEvents();

  // check xmin and xmax
  double lowerboundary, upperboundary;
  if (xmin < 0 || xmax < 0) {
    // xmin or xmax cannot be negative (2theta, dspace and q are always
    // positive)
    findXBoundary(dataws, targetunit, map_runwavelength, lowerboundary,
                  upperboundary);
  } else {
    lowerboundary = xmin;
    upperboundary = xmax;
  }

  g_log.debug() << "Binning  from " << lowerboundary << " to " << upperboundary
                << "\n";

  // Create bins in 2theta (degree)
  size_t sizex, sizey;
  sizex = static_cast<size_t>((upperboundary - lowerboundary) / binsize + 1);
  if (lowerboundary + static_cast<double>(sizex)*binsize < upperboundary)
    ++ lowerboundary;

  sizey = sizex - 1;
  g_log.debug() << "Number of events = " << numevents
                << ", bin size = " << binsize << ", SizeX = " << sizex << ", "
                << ", SizeY = " << sizey
                << ", Delta = " << upperboundary - lowerboundary
                << ", Bin size = " << binsize << ", sizex_d = "
                << (upperboundary - lowerboundary) / binsize + 2 << "\n";
  std::vector<double> vecx(sizex), vecy(sizex - 1, 0), vecm(sizex - 1, 0),
      vece(sizex - 1, 0);

  for (size_t i = 0; i < sizex; ++i) {
    vecx[i] = lowerboundary + static_cast<double>(i) * binsize;
  }

  // Convert unit to unit char bit
  char unitchar = 't'; // default 2theta
  if (targetunit.compare("dSpacing") == 0)
    unitchar = 'd';
  else if (targetunit.compare("Momentum Transfer (Q)") == 0)
    unitchar = 'q';

  binMD(dataws, unitchar, map_runwavelength, vecx, vecy, vec_excludeddets);
  binMD(monitorws, unitchar, map_runwavelength, vecx, vecm, vec_excludeddets);

  // Normalize by division
  double maxmonitorcounts = 0;
  for (size_t i = 0; i < vecm.size(); ++i) {
    if (vecm[i] >= 1.) {
      double y = vecy[i];
      double ey = sqrt(y);
      double m = vecm[i];
      double em = sqrt(m);
      vecy[i] = y / m;
      // using standard deviation's error propagation
      vece[i] = vecy[i] * sqrt((ey / y) * (ey / y) + (em / m) * (em / m));
      // maximum monitor counts
      if (m > maxmonitorcounts)
        maxmonitorcounts = m;
    } else {
      vecy[i] = 0.0;
      vece[i] = 1.0;
    }
  }

  // Create workspace and set values
  API::MatrixWorkspace_sptr pdws =
      WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
  // Set unit
  pdws->setYUnitLabel("Intensity");
  if (unitchar == 'd')
    pdws->getAxis(0)->setUnit("dSpacing");
  else if (unitchar == 'q')
    pdws->getAxis(0)->setUnit("MomentumTransfer");
  else {
    // Twotheta
    pdws->getAxis(0)->setUnit("Degrees");
  }

  MantidVec &dataX = pdws->dataX(0);
  for (size_t i = 0; i < sizex; ++i)
    dataX[i] = vecx[i];
  MantidVec &dataY = pdws->dataY(0);
  MantidVec &dataE = pdws->dataE(0);
  for (size_t i = 0; i < sizey; ++i) {
    dataY[i] = vecy[i];
    dataE[i] = vece[i];
  }

  // Interpolation
  m_infitesimal = 0.1 / (maxmonitorcounts);

  if (dolinearinterpolation)
    linearInterpolation(pdws, m_infitesimal);

  return pdws;
}

//----------------------------------------------------------------------------------------------
/** Find the binning boundaries for 2theta (det position), d-spacing or Q.
 * @brief ConvertCWPDMDToSpectra::findXBoundary
 * @param dataws
 * @param targetunit
 * @param wavelength
 * @param xmin :: (output) lower binning boundary
 * @param xmax :: (output) upper binning boundary
 */
void ConvertCWPDMDToSpectra::findXBoundary(
    API::IMDEventWorkspace_const_sptr dataws, const std::string &targetunit,
    const std::map<int, double> &map_runwavelength, double &xmin,
    double &xmax) {
  // Go through all instruments
  uint16_t numruns = dataws->getNumExperimentInfo();

  xmin = BIGNUMBER;
  xmax = -1;

  for (uint16_t irun = 0; irun < numruns; ++irun) {
    // Skip the Experiment Information does not have run
    if (!dataws->getExperimentInfo(irun)->getInstrument()) {
      g_log.warning() << "iRun = " << irun << " of total " << numruns
                      << " does not have instrument associated"
                      << "\n";
      continue;
    }

    // Get run number
    int runnumber = dataws->getExperimentInfo(irun)->getRunNumber();
    g_log.debug() << "Run " << runnumber << ": ";
    std::map<int, double>::const_iterator miter =
        map_runwavelength.find(runnumber);
    double wavelength = -1;
    if (miter != map_runwavelength.end()) {
      wavelength = miter->second;
      g_log.debug() << " wavelength = " << wavelength << "\n";
    } else {
      g_log.debug() << " no matched wavelength."
                     << "\n";
    }

    // Get source and sample position
    std::vector<detid_t> vec_detid =
        dataws->getExperimentInfo(irun)->getInstrument()->getDetectorIDs(true);
    if (vec_detid.size() == 0) {
      g_log.information() << "Run " << runnumber << " has no detectors."
                          << "\n";
      continue;
    }
    const V3D samplepos =
        dataws->getExperimentInfo(irun)->getInstrument()->getSample()->getPos();
    const V3D sourcepos =
        dataws->getExperimentInfo(irun)->getInstrument()->getSource()->getPos();

    // Get all detectors
    // std::vector<detid_t> vec_detid =
    // dataws->getExperimentInfo(irun)->getInstrument()->getDetectorIDs(true);
    std::vector<Geometry::IDetector_const_sptr> vec_det =
        dataws->getExperimentInfo(irun)->getInstrument()->getDetectors(
            vec_detid);
    size_t numdets = vec_det.size();
    g_log.debug() << "Run = " << runnumber
                  << ": Number of detectors = " << numdets << "\n";

    // Scan all the detectors to get Xmin and Xmax
    for (size_t idet = 0; idet < numdets; ++idet) {
      Geometry::IDetector_const_sptr tmpdet = vec_det[idet];
      const V3D detpos = tmpdet->getPos();

      double R, theta, phi;
      detpos.getSpherical(R, theta, phi);
      if (R < 0.0001)
        g_log.error("Invalid detector position");

      Kernel::V3D v_det_sample = detpos - samplepos;
      Kernel::V3D v_sample_src = samplepos - sourcepos;
      double twotheta =
          calculate2Theta(v_det_sample, v_sample_src) / M_PI * 180.;

      // convert unit optionally
      double outx = -1;
      if (targetunit.compare("2theta") == 0)
        outx = twotheta;
      else {
        if (wavelength <= 0)
          throw std::runtime_error("Wavelength is not defined!");

        if (targetunit.compare("dSpacing") == 0)
          outx = calculateDspaceFrom2Theta(twotheta, wavelength);
        else if (targetunit.compare("Momentum Transfer (Q)") == 0)
          outx = calculateQFrom2Theta(twotheta, wavelength);
        else
          throw std::runtime_error("Unrecognized unit.");
      }

      // Compare with xmin and xmax
      if (outx < xmin)
        xmin = outx;
      if (outx > xmax)
        xmax = outx;
    }
  }


  g_log.debug() << "Find boundary for unit " << targetunit << ": [" << xmin << ", " << xmax << "]"
                 << "\n";
}

//----------------------------------------------------------------------------------------------
/** Bin MD Workspace for detector's position at 2theta
 * @brief ConvertCWPDMDToSpectra::binMD
 * @param mdws
 * @param unitbit
 * @param map_runlambda
 * @param vecx
 * @param vecy
 * @param vec_excludedet
 */
void ConvertCWPDMDToSpectra::binMD(API::IMDEventWorkspace_const_sptr mdws,
                                   const char &unitbit,
                                   const std::map<int, double> &map_runlambda,
                                   const std::vector<double> &vecx,
                                   std::vector<double> &vecy,
                                   const std::vector<detid_t> &vec_excludedet) {
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
  g_log.debug() << "Sample position is " << samplepos.X() << ", "
                << samplepos.Y() << ", " << samplepos.Z() << "\n";

  Geometry::IComponent_const_sptr source =
      expinfo->getInstrument()->getSource();
  const V3D sourcepos = source->getPos();
  g_log.debug() << "Source position is " << sourcepos.X() << ","
                << sourcepos.Y() << ", " << sourcepos.Z() << "\n";

  // Go through all events to find out their positions
  IMDIterator *mditer = mdws->createIterator();
  bool scancell = true;
  size_t nextindex = 1;
  int currRunIndex = -1;
  double currWavelength = -1;
  while (scancell) {
    // get the number of events of this cell
    size_t numev2 = mditer->getNumEvents();
    g_log.debug() << "MDWorkspace " << mdws->name() << " Cell " << nextindex - 1
                  << ": Number of events = " << numev2
                  << " Does NEXT cell exist = " << mditer->next() << "\n";

    // loop over all the events in current cell
    for (size_t iev = 0; iev < numev2; ++iev) {
      // get detector position for 2theta
      detid_t detid = mditer->getInnerDetectorID(iev);
      if (isExcluded(vec_excludedet, detid))
        continue;

      double tempx = mditer->getInnerPosition(iev, 0);
      double tempy = mditer->getInnerPosition(iev, 1);
      double tempz = mditer->getInnerPosition(iev, 2);
      Kernel::V3D detpos(tempx, tempy, tempz);
      Kernel::V3D v_det_sample = detpos - samplepos;
      Kernel::V3D v_sample_src = samplepos - sourcepos;
      double twotheta =
          calculate2Theta(v_det_sample, v_sample_src) / M_PI * 180.;

      // convert unit optionally
      int temprun = static_cast<int>(mditer->getInnerRunIndex(iev));
      double outx;
      if (unitbit == 't')
        outx = twotheta;
      else {
        if (temprun != currRunIndex) {
          // use map to find a new wavelength
          std::map<int, double>::const_iterator miter =
              map_runlambda.find(temprun);
          if (miter == map_runlambda.end()) {
            std::stringstream errss;
            errss << "Event " << iev << " has run ID as " << temprun << ". "
                  << "It has no corresponding ExperimentInfo in MDWorkspace "
                  << mdws->name() << ".";
            throw std::runtime_error(errss.str());
          }
          currWavelength = miter->second;
        }
        if (unitbit == 'd')
          outx = calculateDspaceFrom2Theta(twotheta, currWavelength);
        else
          outx = calculateQFrom2Theta(twotheta, currWavelength);
      }

      // get signal and assign signal to bin
      int xindex;
      const double SMALL = 1.0E-5;
      if (outx+SMALL < vecx.front())
      {
        // Significantly out of left boundary
        xindex = -1;
      }
      else if (fabs(outx - vecx.front()) < SMALL)
      {
        // Almost on the left boundary
        xindex = 0;
      }
      else if (outx-SMALL > vecx.back())
      {
        // Significantly out of right boundary
        xindex = static_cast<int>(vecx.size());
      }
      else if (fabs(outx-vecx.back()) < SMALL)
      {
        // Right on the right boundary
        xindex = static_cast<int>(vecy.size())-1;
      }
      else
      {
        // Other situation
        std::vector<double>::const_iterator vfiter =
            std::lower_bound(vecx.begin(), vecx.end(), outx);
        xindex = static_cast<int>(vfiter - vecx.begin());
        if ( (xindex < static_cast<int>(vecx.size())) && (outx + 1.0E-5 < vecx[xindex]) )
        {
          // assume the bin's boundaries are of [...) and consider numerical error
          xindex -= 1;
        }
        else
        {
          g_log.debug() << "Case for almost same.  Event X = " << outx
                        << ", Boundary = " << vecx[xindex] << "\n";
        }
        if (xindex < 0 || xindex >= static_cast<int>(vecy.size()))
        {
          g_log.warning() << "Case unexpected:  Event X = " << outx
                          << ", Boundary = " << vecx[xindex] << "\n";
        }
      }

      // add signal
      if (xindex < 0)
      {
        // Out of left boundary
        int32_t detid = mditer->getInnerDetectorID(iev);
        uint16_t runid = mditer->getInnerRunIndex(iev);
        g_log.debug() << "Event is out of user-specified range by " << (outx-vecx.front())
                       << ", xindex = " << xindex << ", " << unitbit << " = "
                       << outx << " out of left boundeary [" << vecx.front() << ", "
                       << vecx.back() << "]. dep pos = " << detpos.X()
                       << ", " << detpos.Y() << ", " << detpos.Z()
                       << ", Run = " << runid << ", DetectorID = " << detid << "\n";
        continue;
      }
      else if (xindex >= static_cast<int>(vecy.size())) {
        // Out of right boundary
        int32_t detid = mditer->getInnerDetectorID(iev);
        uint16_t runid = mditer->getInnerRunIndex(iev);
        g_log.debug() << "Event is out of user-specified range "
                      << "xindex = " << xindex << ", " << unitbit << " = "
                      << outx << " out of [" << vecx.front() << ", "
                      << vecx.back() << "]. dep pos = " << detpos.X()
                      << ", " << detpos.Y() << ", " << detpos.Z()
                      << "; sample pos = " << samplepos.X() << ", "
                       << samplepos.Y() << ", " << samplepos.Z()
                       << ", Run = " << runid << ", DetectorID = " << detid << "\n";
        continue;
      }
      else
      {
        double signal = mditer->getInnerSignal(iev);
        vecy[xindex] += signal;
      }
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

//----------------------------------------------------------------------------------------------
/** Do linear interpolation to bins with zero counts.
 * It is applied to those bins with zero value but their neighbor has non-zero
 * values
 * @brief ConvertCWPDMDToSpectra::linearInterpolation
 * @param matrixws
 * @param infinitesimal
 */
void
ConvertCWPDMDToSpectra::linearInterpolation(API::MatrixWorkspace_sptr matrixws,
                                            const double &infinitesimal) {
  g_log.debug() << "Number of spectrum = " << matrixws->getNumberHistograms()
                << " Infinitesimal = " << infinitesimal << "\n";
  size_t numspec = matrixws->getNumberHistograms();
  for (size_t i = 0; i < numspec; ++i) {
    // search for the first nonzero value and last nonzero value
    bool onsearch = true;
    size_t minNonZeroIndex = 0;
    while (onsearch) {
      if (matrixws->readY(i)[minNonZeroIndex] > infinitesimal)
        onsearch = false;
      else
        ++minNonZeroIndex;

      if (minNonZeroIndex == matrixws->readY(i).size())
        onsearch = false;
    }
    size_t maxNonZeroIndex = matrixws->readY(i).size() - 1;
    onsearch = true;
    while (onsearch) {
      if (matrixws->readY(i)[maxNonZeroIndex] > infinitesimal)
        onsearch = false;
      else if (maxNonZeroIndex == 0)
        onsearch = false;
      else
        --maxNonZeroIndex;
    }
    g_log.debug() << "iMinNonZero = " << minNonZeroIndex
                  << ", iMaxNonZero = " << maxNonZeroIndex
                  << " Spectrum index = " << i
                  << ", Y size = " << matrixws->readY(i).size() << "\n";
    if (minNonZeroIndex >= maxNonZeroIndex)
      throw std::runtime_error("It is not right!");


    // Do linear interpolation for zero count values
    for (size_t j = minNonZeroIndex + 1; j < maxNonZeroIndex; ++j) {
      if (matrixws->readY(i)[j] < infinitesimal) {
        // Do interpolation
        // gives y = y_0 + (y_1-y_0)\frac{x - x_0}{x_1-x_0}

        double leftx = matrixws->readX(i)[j - 1];
        double lefty = matrixws->readY(i)[j - 1];
        bool findnonzeroy = true;
        size_t iright = j + 1;
        while (findnonzeroy) {
          if (matrixws->readY(i)[iright] > infinitesimal)
            findnonzeroy = false;
          else
            ++iright;
        }
        double rightx = matrixws->readX(i)[iright];
        double righty = matrixws->readY(i)[iright];
        double curx = matrixws->readX(i)[j];
        double curinterpoy =
            lefty + (righty - lefty) * (curx - leftx) / (rightx - leftx);
        matrixws->dataY(i)[j] = curinterpoy;
        matrixws->dataE(i)[j] = sqrt(curinterpoy);
      }
    }

    return;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Set up sample logs from input data MDWorkspace
 * @brief ConvertCWPDMDToSpectra::setupSampleLogs
 * @param matrixws
 * @param inputmdws
 */
void ConvertCWPDMDToSpectra::setupSampleLogs(
    API::MatrixWorkspace_sptr matrixws,
    API::IMDEventWorkspace_const_sptr inputmdws) {
  // get hold of the last experiment info from md workspace to copy over
  uint16_t lastindex = static_cast<uint16_t>(inputmdws->getNumExperimentInfo()-1);
  ExperimentInfo_const_sptr lastexpinfo =
      inputmdws->getExperimentInfo(lastindex);

  // get hold of experiment info from matrix ws
  Run &targetrun = matrixws->mutableRun();
  const Run &srcrun = lastexpinfo->run();

  const std::vector<Kernel::Property *> &vec_srcprop = srcrun.getProperties();
  for (size_t i = 0; i < vec_srcprop.size(); ++i) {
    Property *p = vec_srcprop[i];
    targetrun.addProperty(p->clone());
    g_log.debug() << "Cloned property " << p->name() << "\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Scale up the values of matrix workspace
 * @brief ConvertCWPDMDToSpectra::scaleMatrixWorkspace
 * @param matrixws
 * @param scalefactor
 * @param infinitesimal
 */
void
ConvertCWPDMDToSpectra::scaleMatrixWorkspace(API::MatrixWorkspace_sptr matrixws,
                                             const double &scalefactor,
                                             const double &infinitesimal) {
  size_t numspec = matrixws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    MantidVec &datay = matrixws->dataY(iws);
    MantidVec &datae = matrixws->dataE(iws);
    size_t numelements = datay.size();
    for (size_t i = 0; i < numelements; ++i) {
      // bin with zero counts is not scaled up
      if (datay[i] >= infinitesimal) {
        datay[i] *= scalefactor;
        datae[i] *= scalefactor;
      }
    }
  } // FOR(iws)

  return;
}

bool
ConvertCWPDMDToSpectra::isExcluded(const std::vector<detid_t> &vec_excludedet,
                                   const detid_t detid) {
  return true;
}

} // namespace MDAlgorithms
} // namespace Mantid
