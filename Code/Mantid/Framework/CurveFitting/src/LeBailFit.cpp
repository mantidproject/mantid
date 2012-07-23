#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

#define PEAKRANGECONSTANT 5
#define WIDTH_FACTOR 3

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_ALGORITHM(LeBailFit)

  //----------------------------------------------------------------------------------------------
  /*
   * Define structure ObservedPeak
   */
  struct ObservedPeak
  {
    double peakHeight;
    double peakPosition;
    double leftFWHM;
    double rightFWHM;
  };

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LeBailFit::LeBailFit()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LeBailFit::~LeBailFit()
  {
  }
  
  /// Sets documentation strings for this algorithm
  void LeBailFit::initDocs()
  {
    this->setWikiSummary("Do LeBail Fit to a spectrum of powder diffraction data.. ");
    this->setOptionalMessage("Do LeBail Fit to a spectrum of powder diffraction data. ");
  }

  /*
   * Define the input properties for this algorithm
   */
  void LeBailFit::init()
  {
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input),
        "Input workspace containing the data to fit by LeBail algorithm.");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ParametersWorkspace", "", Direction::InOut),
        "Input table workspace containing the parameters required by LeBail fit. ");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ReflectionsWorkspace", "", Direction::InOut),
        "Input table workspace containing the list of reflections (HKL). ");
    this->declareProperty("WorkspaceIndex", 0, "Workspace index of the spectrum to fit by LeBail.");

    std::vector<std::string> functions;
    functions.push_back("LeBailFit");
    functions.push_back("Calculation");
    functions.push_back("AutoSelectBackgroundPoints");
    auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
    this->declareProperty("Function", "LeBailFit", validator, "Functionality");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputBackgroundWorkspace", "", Direction::Output),
        "Output workspace containing calculated background. ");

    return;
  }

  /* Implement abstract Algorithm methods
   *
   */
  void LeBailFit::exec()
  {
    // 1. Get input
    dataWS = this->getProperty("InputWorkspace");
    parameterWS = this->getProperty("ParametersWorkspace");
    reflectionWS = this->getProperty("ReflectionsWorkspace");
    int tempindex = this->getProperty("WorkspaceIndex");
    if (tempindex < 0)
      throw std::invalid_argument("Input workspace index cannot be negative.");
    size_t workspaceindex = size_t(tempindex);

    // Function
    std::string function = this->getProperty("Function");
    int functionmode = 0; // calculation
    if (function.compare("Calculation") == 0)
    {
      // peak calculation
      functionmode = 1;
    }
    else if (function.compare("AutoSelectBackgroundPoints") == 0)
    {
      // automatic background points selection
      functionmode = 2;
    }

    // 2. Check and/or process inputs
    if (workspaceindex >= dataWS->getNumberHistograms())
    {
      g_log.error() << "Input WorkspaceIndex " << workspaceindex << " is out of boundary [0, " <<
          dataWS->getNumberHistograms() << ")" << std::endl;
      throw std::invalid_argument("Invalid input workspace index. ");
    }

    this->importParametersTable();
    this->importReflections();

    // 3. Create LeBail Function & initialize from input
    mLeBail = boost::shared_ptr<CurveFitting::LeBailFunction>(new CurveFitting::LeBailFunction);
    mLeBail->initialize();
    this->setLeBailParameters(mLeBail);
    this->initLeBailPeakParameters(mLeBail);

    // 4. LeBail Fit or calculation
    bool converged = false;
    switch (functionmode)
    {
    case 0:
        // LeBail Fit
        g_log.notice() << "Do LeBail Fit." << std::endl;
        while (!converged)
        {
          converged = iterateFit(size_t(workspaceindex));
        }
        break;

    case 1:
        // Calculation
        g_log.notice() << "Pattern Calculation. It is not FINISHED yet. " << std::endl;
        break;

    case 2:
        // Background
        this->getBackground(workspaceindex);
        break;

    default:
        // Impossible
        g_log.warning() << "FunctionMode = " << functionmode <<".  It is not possible" << std::endl;
        break;
    }

    // 5. Release

    return;
  }

  /*
   * One iteration to fit LeBail function
   */
  bool LeBailFit::iterateFit(size_t wsindex)
  {
    bool retvalue = false;

    // 1. Calculate I(cal) for each peak
    std::vector<double> peakheights;
    calPeakHeights(peakheights, wsindex);
    mLeBail->setPeakHeights(peakheights);

    // 2. Set fit and run
    double tof_min, tof_max;
    tof_min = dataWS->readX(wsindex)[0];
    tof_max = dataWS->readX(wsindex).back();

    API::IAlgorithm_sptr fit = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
    fit->initialize();

    // fit->setPropertyValue("Function", mLeBail->asString());
    fit->setProperty("Function", boost::shared_ptr<API::IFunction>(mLeBail));
    fit->setPropertyValue("InputWorkspace", dataWS->name());
    fit->setProperty("WorkspaceIndex", int(wsindex));
    fit->setProperty("StartX", tof_min);
    fit->setProperty("EndX", tof_max);
    fit->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit->setProperty("CostFunction", "Least squares");
    fit->setProperty("MaxIterations", 100);

    // 3. Analyze the fitting result
    g_log.debug() << "DB215 Fit(" << mLeBail->asString() << ")" << std::endl;
    bool execed = fit->execute();
    if (!execed)
    {
      retvalue = false;

      g_log.error() << "Fit LeBailFunction Fails. " << std::endl;
    }
    else
    {
      retvalue = true;

      double chi2 = fit->getProperty("OutputChi2overDoF");
      std::string fitstatus = fit->getProperty("OutputStatus");
      g_log.information() << "LeBailFit (LeBailFunction) Fit result:  Chi^2 = " << chi2
          << " Fit Status = " << fitstatus << std::endl;

      // b) Get parameters
      std::map<std::string, std::pair<double, char> >::iterator pit;
      IFunction_sptr fitout = fit->getProperty("Function");

      std::vector<std::string> parnames = fitout->getParameterNames();
      std::sort(parnames.begin(), parnames.end());

      for (pit = mFuncParameters.begin(); pit != mFuncParameters.end(); ++pit)
      {
        std::string parname = pit->first;
        std::vector<std::string>::iterator fit = std::find(parnames.begin(), parnames.end(), parname);
        if (fit != parnames.end())
        {
          double prevalue = pit->second.first;
          double curvalue = fitout->getParameter(parname);
          g_log.debug() << "DB216 Parameter " << parname << ": " << prevalue << "  vs  " << curvalue << std::endl;
        }
      }
    }

    return retvalue;
  }

  /*
   * Set the parameters for LeBail function (not to each individual peak)
   * from mFuncParameters()
   */
  void LeBailFit::setLeBailParameters(CurveFitting::LeBailFunction_sptr func)
  {
    // 1. Set parameters ...
    std::map<std::string, std::pair<double, char> >::iterator pit;

    std::vector<std::string> lebailparnames = func->getParameterNames();
    std::sort(lebailparnames.begin(), lebailparnames.end());

    for (pit = mFuncParameters.begin(); pit != mFuncParameters.end(); ++pit)
    {
      std::string parname = pit->first;
      double value = pit->second.first;
      char fitortie = pit->second.second;

      g_log.debug() << "LeBailFit Set " << parname << "= " << value << std::endl;

      std::vector<std::string>::iterator ifind = std::find(lebailparnames.begin(),
          lebailparnames.end(), parname);
      if (ifind == lebailparnames.end())
      {
        g_log.warning() << "Parameter " << parname << " in input parameter table workspace is not for peak function. " << std::endl;
        continue;
      }

      func->setParameter(parname, value);

      if (fitortie == 'f')
      {
        ;
      }
      else if (fitortie == 't')
      {
        std::stringstream ss;
        ss << value;
        func->tie(parname, ss.str());
      }
      else
      {
        throw std::invalid_argument("Only f and t are supported as for fit or tie.");
      }
    }

    return;
  }

  /*
   * Add peaks to LeBail function and calculate each individual's peak parameters
   */
  void LeBailFit::initLeBailPeakParameters(CurveFitting::LeBailFunction_sptr func)
  {
    // 1. Add peaks: using 1.0 as default value of peak height
    std::vector<double> heights;
    for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
      heights.push_back(1.0);

    func->addPeaks(mPeakHKLs, heights);

    // 2. Calculate each peak's parameters
    func->calPeaksParameters();

    return;
  }

  /*
   * Calculate the peak intensities (I) from observed and calculated peak profile
   */
  void LeBailFit::calPeakHeights(std::vector<double>& peakheights, size_t workspaceindex)
  {
    // 1. Calculate the FWHM of each peak: Only peakcenterpairs is in order of peak position. Others are in input order of peaks
    std::vector<double> peakfwhms;
    std::vector<double> peakcenters;
    std::vector<std::pair<double, double> > peakboundaries;
    std::vector<std::pair<double, size_t> > peakcenterpairs;

    // a) Get center of peaks for sorting, and estimate peaks' range
    for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
    {
      double fwhm = mLeBail->getPeak(ipk)->fwhm();
      double center = mLeBail->getPeak(ipk)->centre();

      double tof_left, tof_right, tof_center;
      this->estimatePeakRange(workspaceindex, center, fwhm, tof_center, tof_left, tof_right);

      peakcenters.push_back(tof_center);
      peakcenterpairs.push_back(std::make_pair(tof_center, ipk));
      peakboundaries.push_back(std::make_pair(tof_left, tof_right));

      g_log.debug() << "DB1144 Peak " << mPeakHKLs[ipk][0] << ", " << mPeakHKLs[ipk][1] << ", " << mPeakHKLs[ipk][2] << ": "
          << "FWHM = " << fwhm << " @ TOF = " << center << std::endl;
    }

    // b) Sort
    std::sort(peakcenterpairs.begin(), peakcenterpairs.end());

    // 2. Regroup peaks
    std::vector<std::set<size_t> > peakgroups; // record peaks in groups; peaks in same group are very close
    std::set<size_t> peakindices;
    double boundaryconst = 4.0;

    for (size_t ix = 0; ix < peakcenters.size(); ++ix)
    {
      // Note: ix is only bounded to peakcenterpairs
      size_t ipk = peakcenterpairs[ix].second;

      if (peakindices.size() > 0)
      {
        size_t leftpeakindex = peakcenterpairs[ix-1].second;
        double leftpeakcenter = peakcenterpairs[ix-1].first;
        double leftpeakrange = peakboundaries[leftpeakindex].second - leftpeakcenter;
        double leftpeak_rightbound =
            leftpeakcenter + boundaryconst * leftpeakrange;

        double thispeak_leftbound =
            peakcenterpairs[ix].first - boundaryconst * (peakcenterpairs[ix].first - peakboundaries[ipk].first);

        if (thispeak_leftbound > leftpeak_rightbound)
        {
          // current peak has no overlap with previous peak, start a new peak group
          std::set<size_t> settoinsert = peakindices;
          peakgroups.push_back(settoinsert);
          peakindices.clear();
        }
      }

      // Insert the current peak index to set
      peakindices.insert(ipk);
    } // ENDFOR

    // Insert the last group
    peakgroups.push_back(peakindices);

    g_log.debug() << "LeBailFit:  Size(Peak Groups) = " << peakgroups.size() << std::endl;

    // 3. Calculate each peak's intensity
    std::vector<std::pair<size_t, double> > peakintensities;
    for (size_t ig = 0; ig < peakgroups.size(); ++ig)
    {
      std::vector<std::pair<size_t, double> > tempintensities;
      this->calPeaksIntensity(workspaceindex, peakgroups[ig], peakcenters, peakboundaries, tempintensities);
      peakintensities.insert(peakintensities.end(), tempintensities.begin(), tempintensities.end());
    }

    // 4. Set up peak-heights
    peakheights.clear();
    for (size_t ipk = 0; ipk < peakintensities.size(); ++ipk)
    {
      peakheights.push_back(-1.0);
    }
    for (size_t ipk = 0; ipk < peakintensities.size(); ++ipk)
    {
      size_t ipeak = peakintensities[ipk].first;
      double height = peakintensities[ipk].second;
      peakheights[ipeak] = height;
    }

    return;
  }

  /*
   * Calculate peak intensities for each group of peaks
   */
  void LeBailFit::calPeaksIntensity(size_t wsindex, std::set<size_t> peakindices, std::vector<double> peakcenters,
      std::vector<std::pair<double, double> > peakboundaries, std::vector<std::pair<size_t, double> >& peakintensities)
  {
    std::set<size_t>::iterator pit;

    // 1. Determine range
    g_log.debug() << "DB252 Group Size = " << peakindices.size() << " Including peak indexed " << std::endl;
    std::vector<size_t> peaks;
    for (pit = peakindices.begin(); pit != peakindices.end(); ++pit)
    {
      peaks.push_back(*pit);
      g_log.debug() << "Peak index = " << *pit << std::endl;
    }

    if (peaks.size() > 1)
      std::sort(peaks.begin(), peaks.end());

    double leftbound = peakcenters[peaks[0]]-PEAKRANGECONSTANT*(-peakboundaries[peaks[0]].first+peakcenters[peaks[0]]);
    double rightbound = peakcenters[peaks.back()]+PEAKRANGECONSTANT*(peakboundaries[peaks.back()].second-peakcenters[peaks.back()]);

    const MantidVec& datax = dataWS->readX(wsindex);
    const MantidVec& datay = dataWS->readY(wsindex);
    size_t ileft = this->findNearest(datax, leftbound);
    size_t iright = this->findNearest(datax, rightbound);

    if (iright <= ileft)
    {
      g_log.error() << "Try to integrate peak from " << leftbound << " To " << rightbound << std::endl <<
          "  Peak boundaries : " << peakboundaries[peaks[0]].first << ", " << peakboundaries[peaks[0]].second <<
          "  Peak center: " << peakcenters[peaks[0]] << "  ... " << peakcenters[peaks.back()] << std::endl;
      throw std::logic_error("iRight cannot be less or equal to iLeft.");
    }
    else
    {
      g_log.debug() << "DB452 Integrate peak from " << leftbound << "/" <<
          ileft << " To " << rightbound << "/" << iright << std::endl;
    }

    // 2. Integrate
    size_t ndata = iright - ileft + 1;
    double *xvalues = new double[ndata];
    double *tempout = new double[ndata];
    double *sumYs = new double[ndata];
    for (size_t iy = 0; iy < ndata; ++iy)
    {
      xvalues[iy] = datax[iy+ileft];
      tempout[iy] = 0.0;
      sumYs[iy] = 0.0;
    }

    for (size_t i = 0; i < peaks.size(); ++i)
    {
      CurveFitting::Bk2BkExpConvPV_sptr ipeak = mLeBail->getPeak(peaks[i]);
      ipeak->function1D(tempout, xvalues, ndata);
      for (size_t j = 0; j < ndata; ++j)
      {
        sumYs[j] += tempout[j];
      }
    }

    // 3. Calculate intensity for each peak
    for (size_t i = 0; i < peaks.size(); ++i)
    {
      double intensity = 0.0;
      CurveFitting::Bk2BkExpConvPV_sptr ipeak = mLeBail->getPeak(peaks[i]);
      ipeak->function1D(tempout, xvalues, ndata);
      for (size_t j = 0; j < ndata; ++j)
      {
        if (sumYs[j] > 1.0E-5)
        {
          double temp = datay[ileft+j]*tempout[j]/sumYs[j];
          intensity += temp*(datax[j+1]-datax[j]);
        }
      }
      peakintensities.push_back(std::make_pair(peaks[i], intensity));
      g_log.debug() << "Peak " << peaks[i] << "  Height = " << intensity << std::endl;
    }

    // -1. Clean
    delete[] xvalues;
    delete[] tempout;
    delete[] sumYs;

    return;
  }

  /*
   * Estimate peak center and peak range according to input information;
   * - center: user input peak center
   * - fwhm: user input fwhm
   * - tof_center: estimated peak center (output)
   * - tof_left: estimated left boundary at half maximum
   * - tof_right: estimated right boundary at half maximum
   * Return: False if no peak found (maximum value is at center+/-fwhm
   */
  bool LeBailFit::estimatePeakRange(size_t workspaceindex, double center, double fwhm,
      double& tof_center, double& tof_left, double& tof_right)
  {
    const MantidVec& datax = dataWS->readX(workspaceindex);
    const MantidVec& datay = dataWS->readY(workspaceindex);

    // 1. Find index of center
    // size_t icenter = findNearest(datax, center);
    size_t ileft = findNearest(datax, center-fwhm);
    size_t iright = findNearest(datax, center+fwhm);

    // 2. Find maximum
    double maxh = 0;
    size_t icenter = ileft;
    for (size_t i = ileft; i <= iright; ++i)
    {
      if (datay[i] > maxh)
      {
        icenter = i;
        maxh = datay[i];
      }
    }
    if (icenter == ileft || icenter == iright)
    {
      g_log.error() << "Designated peak @ TOF = " << center << " cannot be located within user input center+/-fwhm = "
          << fwhm << std::endl;
      tof_center = -0.0;
      tof_left = center-fwhm;
      tof_right = center+fwhm;

      return false;
    }

    // 3. Find half maximum
    double halfmax = 0.5*maxh;

    // a) Find left boundary
    size_t itof = icenter-1;
    bool continuesearch = true;
    bool cannotfindL = false;
    while (continuesearch)
    {
      if (datay[itof] <= halfmax && datay[itof+1] > halfmax)
      {
        // Find it!
        ileft = itof;
        continuesearch = false;
      }
      else if (datay[itof] > datay[itof+1])
      {
        // Min value exceeds half maximum
        cannotfindL = true;
        ileft = itof+1;
        continuesearch = false;
      }
      else if (itof == 0)
      {
        // Impossible situation
        cannotfindL = true;
        ileft = itof;
        continuesearch = false;
      }
      itof --;
    }

    // b) Find right boundary
    // a) Find left boundary
    itof = icenter+1;
    continuesearch = true;
    bool cannotfindR = false;
    while (continuesearch)
    {
      if (datay[itof] <= halfmax && datay[itof-1] > halfmax)
      {
        // Find it!
        iright = itof;
        continuesearch = false;
      }
      else if (datay[itof] > datay[itof-1])
      {
        // Min value exceeds half maximum
        cannotfindR = true;
        iright = itof-1;
        continuesearch = false;
      }
      else if (itof == datax.size()-1)
      {
        // Impossible situation
        cannotfindR = true;
        iright = itof;
        continuesearch = false;
      }
      itof ++;
    }

    tof_center = datax[icenter];
    tof_left = datax[ileft]+(datax[ileft+1]-datax[ileft])*(halfmax-datay[ileft])/(datay[ileft+1]-datay[ileft]);
    tof_right = datax[iright]-(datax[iright]-datax[iright-1])*(halfmax-datay[iright])/(datay[iright-1]-datay[iright]);

    g_log.information() << "DB502 Estimate Peak Range:  Center = " << tof_center << ";  Left = " << tof_left << ", Right = " << tof_right << std::endl;

    return (!cannotfindL && !cannotfindR);
  }

  /*
   * Parse the input TableWorkspace to some maps for easy access
   */
  void LeBailFit::importParametersTable()
  {
    // 1. Check column orders
    std::vector<std::string> colnames = parameterWS->getColumnNames();
    if (colnames.size() < 3)
    {
      g_log.error() << "Input parameter table workspace does not have enough number of columns. "
          << " Number of columns = " << colnames.size() << " < 3 as required. " << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }
    if (colnames[0].compare("Name") != 0 ||
        colnames[1].compare("Value") != 0 ||
        colnames[2].compare("FitOrTie") != 0)
    {
      g_log.error() << "Input parameter table workspace does not have the columns in order.  "
          << " It must be Name, Value, FitOrTie." << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }

    // 2. Import data to maps
    std::string parname, fitortie;
    double value;

    size_t numrows = parameterWS->rowCount();

    for (size_t ir = 0; ir < numrows; ++ir)
    {
      API::TableRow trow = parameterWS->getRow(ir);
      trow >> parname >> value >> fitortie;
      // fit or tie?
      char tofit = 'f';
      if (fitortie.length() > 0)
      {
        char fc = fitortie.c_str()[0];
        if (fc == 't' || fc == 'T')
        {
          tofit = 't';
        }
      }
      mFuncParameters.insert(std::make_pair(parname, std::make_pair(value, tofit)));
    }

    return;
  }

  /*
   * Parse the reflections workspace to a list of reflections;
   */
  void LeBailFit::importReflections()
  {
    // 1. Check column orders
    std::vector<std::string> colnames = reflectionWS->getColumnNames();
    if (colnames.size() < 3)
    {
      g_log.error() << "Input parameter table workspace does not have enough number of columns. "
          << " Number of columns = " << colnames.size() << " < 3 as required. " << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }
    if (colnames[0].compare("H") != 0 ||
        colnames[1].compare("K") != 0 ||
        colnames[2].compare("L") != 0)
    {
      g_log.error() << "Input parameter table workspace does not have the columns in order.  "
          << " It must be H, K, L." << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }

    // 2. Import data to maps
    int h, k, l;

    size_t numrows = reflectionWS->rowCount();
    for (size_t ir = 0; ir < numrows; ++ir)
    {
      API::TableRow trow = reflectionWS->getRow(ir);
      trow >> h >> k >> l;
      std::vector<int> hkl;
      hkl.push_back(h);
      hkl.push_back(k);
      hkl.push_back(l);

      mPeakHKLs.push_back(hkl);
    }

    return;
  }

  /*
   * Find the nearest index of value
   */
  size_t LeBailFit::findNearest(const MantidVec& vec, double value)
  {
    if (value <= vec[0])
    {
      return 0;
    }
    else if (value >= vec.back())
    {
      return (vec.size()-1);
    }
    else
    {
      // My own version of binary search... As lower_bound() cannot take a constant vector
      size_t istart = 0;
      size_t iend = vec.size()-1;
      bool search = true;
      while (search)
      {
        if (iend == istart)
        {
          return iend;
        }
        else if (iend == istart+1)
        {
          if (vec[iend]-value < value-vec[istart])
          {
            return iend;
          }
          else
          {
            return istart;
          }
        }
        else
        {
          size_t imiddle = (istart+iend)/2;
          if (value < vec[imiddle])
          {
            iend = imiddle;
          }
          else if (value > vec[imiddle])
          {
            istart = imiddle;
          }
          else
          {
            return imiddle;
          }
        }
      } // ENDWHILE
    }

    return 0;
  }


  /*
   * Get background including
   * (1) Choose background points automatically between each two adjacent peaks (if not too close);
   * (2) Fit background.
   * (3) Store background points to output workspace
   */
  void LeBailFit::getBackground(size_t wsindex)
  {
    // 1. Sort peak by TOF
    std::vector<std::pair<double, size_t> > mpeaks; // TOF, peak index
    for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
    {
      double tof = mLeBail->getPeakParameter(ipk, "TOF_h");
      mpeaks.push_back(std::make_pair(tof, ipk));
    }
    std::sort(mpeaks.begin(), mpeaks.end());

    // 2. Find observed peaks
    std::vector<ObservedPeak> mobservedpeaks;
    // std::vector<DataObjects::ObservedPeak* > mobservedpeaks;
    for (size_t ipk = 0; ipk < mpeaks.size(); ++ipk)
    {
      size_t peakindex = mpeaks[ipk].second;
      double peakposition, leftfwhm, rightfwhm, peakheight, leftposition, rightposition;
      peakheight = 1.0; // It is not required at this stage
      bool peakfound = this->estimatePeakRange(wsindex, mLeBail->getPeakParameter(peakindex, "TOF_h"),
          mLeBail->getPeakFWHM(peakindex), peakposition, leftposition, rightposition);
      leftfwhm = peakposition-leftposition;
      rightfwhm = rightposition-peakposition;
      if (peakfound)
      {
        ObservedPeak tempeak;
        tempeak.peakPosition = peakposition;
        tempeak.peakHeight = peakheight;
        tempeak.leftFWHM = leftfwhm*2.0;
        tempeak.rightFWHM = rightfwhm*2.0;
        mobservedpeaks.push_back(tempeak);
      }
    }
    g_log.information() << "LeBailFit: Number of peaks found = " << mobservedpeaks.size() << std::endl;

    if (mobservedpeaks.size() == 0)
    {
      g_log.error() << "No background point can be determined.  It is an abnormal situation. " << std::endl;
      throw std::runtime_error("No background point can be determined.  It is an abnormal situation. ");
    }

    // 3. Locate background points
    std::vector<size_t> ibackgroundpts;
    std::vector<double>::const_iterator cit;
    const MantidVec& vecX = dataWS->readX(wsindex);

    // a) From zero to first peak
    double tofright = mobservedpeaks[0].peakPosition - WIDTH_FACTOR*mobservedpeaks[0].leftFWHM;
    cit = std::lower_bound(vecX.begin(), vecX.end(), tofright);
    size_t iright = size_t(cit-vecX.begin());
    for (size_t ipt = 0; ipt < iright; ++ipt)
    {
        ibackgroundpts.push_back(ipt);
    }

    // b) between 2 peaks
    for (size_t ipk = 1; ipk < mobservedpeaks.size(); ++ipk)
    {
      // size_t peak_left = mpeaks[ipk - 1].second;
      // size_t peak_right = mpeaks[ipk].second;
      size_t peak_left = ipk-1;
      size_t peak_right = ipk;
      double tof_leftbound = mobservedpeaks[peak_left].peakPosition + WIDTH_FACTOR * mobservedpeaks[peak_left].rightFWHM;
      double tof_rightbound = mobservedpeaks[peak_right].peakPosition - WIDTH_FACTOR * mobservedpeaks[peak_right].leftFWHM;
      if (tof_leftbound < tof_rightbound)
      {
          /*
          size_t imin = findMinValue(dataWS->readX(wsindex), dataWS->readY(wsindex), tof_leftbound,
            tof_rightbound);
          ibackgroundpts.push_back(imin);
          */
          cit = std::lower_bound(vecX.begin(), vecX.end(), tof_leftbound);
          size_t ileft = size_t(cit-vecX.begin());
          cit = std::lower_bound(vecX.begin(), vecX.end(), tof_rightbound);
          size_t iright = size_t(cit-vecX.begin());
          g_log.information() << "Between peak @ " <<  mobservedpeaks[peak_left].peakPosition << " and peak @ "
                              << mobservedpeaks[peak_right].peakPosition << ".   "
                              << (iright-ileft+1) << " background points selected. " << std::endl;
          for (size_t ipt = ileft; ipt <= iright; ++ipt)
          {
              ibackgroundpts.push_back(ipt);
          }
      }
      else
      {
          g_log.information() << "Peak @ " << mobservedpeaks[peak_left].peakPosition << " and @ "
              << mobservedpeaks[peak_right].peakPosition << " are overlapped. "
              << "Left Peak FWHM = " << mobservedpeaks[peak_left].rightFWHM
              << "; Right Peak FWHM = " << mobservedpeaks[peak_right].leftFWHM
              << "; Factor = " << WIDTH_FACTOR  << std::endl;
      }
    } // ENDFOR

    // c) From last peak to end
    double tofleft = mobservedpeaks.back().peakPosition + WIDTH_FACTOR*mobservedpeaks.back().rightFWHM;
    cit = std::find(vecX.begin(), vecX.end(), tofleft);
    size_t ileft = size_t(cit-vecX.begin());
    for (size_t ipt = ileft; ipt < vecX.size(); ++ipt)
    {
        ibackgroundpts.push_back(ipt);
    }

    g_log.information() << "Number of background points = " << ibackgroundpts.size() << std::endl;

    // 4. Set background workspace for fit;
    size_t nspec = 1;
    size_t nbin = ibackgroundpts.size();
    DataObjects::Workspace2D_sptr bkgdws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
    for (size_t i = 0; i < nbin; ++i)
    {
      bkgdws->dataX(0)[i] = dataWS->readX(wsindex)[ibackgroundpts[i]];
      bkgdws->dataY(0)[i] = dataWS->readY(wsindex)[ibackgroundpts[i]];
    }
    bkgdws->getAxis(0)->setUnit("TOF");

    this->setProperty("OutputBackgroundWorkspace", bkgdws);

    // 5. Fit background
    // FIXME background is not fitted because a proper background function has not been decided yet.
    // fitBackground(bkgdws, 2);

    return;
  }


  /*
   * Find the mininum value (index in give vector pair) within a specified region
   */
  size_t LeBailFit::findMinValue(const MantidVec& vecX, const MantidVec& vecY, double leftbound, double rightbound)
  {

    MantidVec::const_iterator mit = std::lower_bound(vecX.begin(), vecX.end(), leftbound);
    size_t ileft = size_t(mit-vecX.begin());
    double minY = 1.0E80;
    size_t imin = ileft;
    size_t icurr = ileft + 1;
    while (vecX[icurr] <= rightbound)
    {
      if (vecY[icurr] < minY)
      {
        imin = icurr;
        minY = vecY[icurr];
      }
      ++icurr;
    }

    g_log.debug() << "Find min value between " << leftbound << " , " << rightbound
        << " Find min I(TOF) @ TOF = " << vecX[imin] << " /" << imin << std::endl;

    return imin;
  }

  /*
   * Fit background function
   */
  void LeBailFit::fitBackground(DataObjects::Workspace2D_sptr bkgdWS, size_t polyorder)
  {
    throw std::runtime_error("To Be Implemented Soon!");
    g_log.debug() << "Background Name = " << bkgdWS->name() << "  Order = " << polyorder << std::endl;
  }

  /*
   * Get parameter value (fitted)
   */
  double LeBailFit::getFittedParameterValue(std::string parname)
  {
    double value = mLeBail->getParameter(parname);

    return value;
  }




} // namespace Mantid
} // namespace Algorithms














