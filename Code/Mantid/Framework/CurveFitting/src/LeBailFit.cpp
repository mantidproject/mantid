#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iomanip>

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

#define PEAKRANGECONSTANT 5.0

bool compDescending(int a, int b)
{
    return (a >= b);
}

DECLARE_ALGORITHM(LeBailFit)

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
  
/** Sets documentation strings for this algorithm
 */
void LeBailFit::initDocs()
{
    this->setWikiSummary("Do LeBail Fit to a spectrum of powder diffraction data.. ");
    this->setOptionalMessage("Do LeBail Fit to a spectrum of powder diffraction data. ");
}

/** Define the input properties for this algorithm
 */
void LeBailFit::init()
{
    // --------------  Input and output Workspaces  -----------------

    // Data
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input),
      "Input workspace containing the data to fit by LeBail algorithm.");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output),
                          "Output workspace containing calculated pattern or calculated background. ");

    // Parameters
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("InputParameterWorkspace", "", Direction::Input),
      "Input table workspace containing the parameters required by LeBail fit. ");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("OutputParameterWorkspace", "", Direction::Output, API::PropertyMode::Optional),
      "Input table workspace containing the parameters required by LeBail fit. ");

    // Single peak: Reflection (HKL) Workspace, PeaksWorkspace
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("InputHKLWorkspace", "", Direction::InOut),
      "Input table workspace containing the list of reflections (HKL). ");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("OutputPeaksWorkspace", "", Direction::Output, API::PropertyMode::Optional),
                        "Optional output table workspace containing all peaks' peak parameters. ");

    // WorkspaceIndex
    this->declareProperty("WorkspaceIndex", 0, "Workspace index of the spectrum to fit by LeBail.");

    // Interested region
    this->declareProperty(new Kernel::ArrayProperty<double>("FitRegion"),
                          "Region of data (TOF) for LeBail fit.  Default is whole range. ");

    // Functionality: Fit/Calculation/Background
    std::vector<std::string> functions;
    functions.push_back("LeBailFit");
    functions.push_back("Calculation");
    //  TODO: This option is temporarily turned off for release 2.3
    //  functions.push_back("CalculateBackground");
    auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
    this->declareProperty("Function", "LeBailFit", validator, "Functionality");

    // About background:  Background type, input (table workspace or array)
    std::vector<std::string> bkgdtype;
    bkgdtype.push_back("Polynomial");
    bkgdtype.push_back("Chebyshev");
    auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
    this->declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Background type");

    this->declareProperty("BackgroundFunctionOrder", 12, "Order of background function.");

    // Input background parameters (array)
    this->declareProperty(new Kernel::ArrayProperty<double>("BackgroundParameters"),
                          "Optional: enter a comma-separated list of background order parameters from order 0. ");

    // Input background parameters (tableworkspace)
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("BackgroundParametersWorkspace", "", Direction::InOut, API::PropertyMode::Optional),
            "Optional table workspace containing the fit result for background.");

    // UseInputPeakHeights
    this->declareProperty("UseInputPeakHeights", true,
                        "For function Calculation, use peak heights specified in ReflectionWorkspace.  Otherwise, calcualte peaks' heights. ");

    // Peak Radius
    this->declareProperty("PeakRadius", 5, "Range (multiplier relative to FWHM) for a full peak. ");

    // Pattern calcualtion
    this->declareProperty("PlotIndividualPeaks", false, "Option to output each individual peak in mode Calculation.");

    // Minimizer
    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys(); // :Instance().getKeys();
    declareProperty("Minimizer","Levenberg-MarquardtMD",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(minimizerOptions)),
                    "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Kernel::Direction::InOut);

    return;
}

/** Implement abstract Algorithm methods
 */
void LeBailFit::exec()
{
  // 1. Get input and perform some check
  // a) Import data workspace and related, do crop
  API::MatrixWorkspace_sptr inpWS = this->getProperty("InputWorkspace");

  int tempindex = this->getProperty("WorkspaceIndex");
  if (tempindex < 0)
    throw std::invalid_argument("Input workspace index cannot be negative.");
  size_t workspaceindex = size_t(tempindex);

  if (workspaceindex >= inpWS->getNumberHistograms())
  {
    // throw if workspace index is not correct
    g_log.error() << "Input WorkspaceIndex " << workspaceindex << " is out of boundary [0, "
                  << inpWS->getNumberHistograms() << ")" << std::endl;
    throw std::invalid_argument("Invalid input workspace index. ");
  }

  dataWS = this->cropWorkspace(inpWS, workspaceindex);

  // b) Minimizer
  std::string minim = getProperty("Minimizer");
  mMinimizer = minim;

  // c) Peak parameters and related.
  parameterWS = this->getProperty("InputParameterWorkspace");
  reflectionWS = this->getProperty("InputHKLWorkspace");
  mPeakRadius = this->getProperty("PeakRadius");

  // d) Determine Functionality (function mode)
  std::string function = this->getProperty("Function");
  FunctionMode functionmode = FIT; // Default: LeBailFit
  if (function.compare("Calculation") == 0)
  {
    // peak calculation
    functionmode = CALCULATION;
  }
  else if (function.compare("CalculateBackground") == 0)
  {
    // automatic background points selection
    functionmode = BACKGROUNDPROCESS;
  }

  // 2. Import parameters from table workspace
  this->importParametersTable();
  this->importReflections();

  // 3. Create LeBail Function & initialize from input
  // a. All individual peaks
  bool inputparamcorrect = generatePeaksFromInput(workspaceindex);

  // b. Background
  std::string backgroundtype = this->getProperty("BackgroundType");
  std::vector<double> bkgdorderparams = this->getProperty("BackgroundParameters");
  DataObjects::TableWorkspace_sptr bkgdparamws = this->getProperty("BackgroundParametersWorkspace");
  if (!bkgdparamws)
  {
    g_log.information() << "[Input] Use background specified with vector with input vector sized "
                   << bkgdorderparams.size() << "." << std::endl;
  }
  else
  {
    g_log.information() << "[Input] Use background specified by table workspace. " << std::endl;
    parseBackgroundTableWorkspace(bkgdparamws, bkgdorderparams);
  }
  mBackgroundFunction = generateBackgroundFunction(backgroundtype, bkgdorderparams);

  // c. Create CompositeFunction
  API::CompositeFunction compfunction;
  mLeBailFunction = boost::make_shared<API::CompositeFunction>(compfunction);

  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator mit;
  for (mit = mPeaks.begin(); mit != mPeaks.end(); ++ mit)
  {
    mLeBailFunction->addFunction(mit->second);
  }
  mLeBailFunction->addFunction(mBackgroundFunction);

  g_log.debug() << "LeBail Composite Function: " << mLeBailFunction->asString() << std::endl;

  // 5. Create output workspace
  this->createOutputDataWorkspace(workspaceindex, functionmode);
  this->setProperty("OutputWorkspace", outputWS);

  // 6. Real work
  mLeBaiLFitChi2 = -1; // Initialize
  mLeBailCalChi2 = -1;

  switch (functionmode)
  {
    case FIT:
      // LeBail Fit
      g_log.notice() << "Function: Do LeBail Fit." << std::endl;
      if (inputparamcorrect)
      {
        doLeBailFit(workspaceindex);
      }
      else
      {
        fakeOutputData(workspaceindex, functionmode);
      }
      break;

    case CALCULATION:
      // Calculation
      g_log.notice() << "Function: Pattern Calculation." << std::endl;
      if (inputparamcorrect)
      {
        calculatePattern(workspaceindex);
      }
      else
      {
        fakeOutputData(workspaceindex, functionmode);
      }
      break;

    case BACKGROUNDPROCESS:
      // Calculating background
      g_log.notice() << "Function: Calculate Background (Precisely). " << std::endl;
      calBackground(workspaceindex);
      break;

    default:
      // Impossible
      std::stringstream errmsg;
      errmsg << "FunctionMode = " << functionmode <<" is not supported.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());

      break;
  }

  // 8. Calcualte Chi^2 of Output Data (calcualted or fitted)
  if (functionmode == 0 || functionmode == 1)
  {
    doResultStatistics();
  }

  // 7. Output peak (table) and parameter workspace
  exportEachPeaksParameters();
  exportParametersWorkspace(mFuncParameters);

  return;
}


/// =================================== Level 1 methods called by exec() directly ================================================ ///

/** Calcualte LeBail diffraction pattern:
 *  Output spectra:
 *  0: data;  1: calculated pattern; 3: difference
 *  4: input pattern w/o background
 *  5~5+(N-1): optional individual peak
 */
void LeBailFit::calculatePattern(size_t workspaceindex)
{


  // 1. Generate domain and value
  const std::vector<double> x = dataWS->readX(workspaceindex);
  API::FunctionDomain1DVector domain(x);
  API::FunctionValues values(domain);

  // 2. Calculate diffraction pattern
  bool useinputpeakheights = this->getProperty("UseInputPeakHeights");
  this->calculateDiffractionPattern(workspaceindex, domain, values, mFuncParameters, !useinputpeakheights);

    // 3. For X of first 4
    for (size_t isp = 0; isp < 5; ++isp)
    {
        for (size_t i = 0; i < domain.size(); ++i)
            outputWS->dataX(isp)[i] = domain[i];
    }

    // 4. Retrieve and construct the output
    for (size_t i = 0; i < values.size(); ++i)
    {
        outputWS->dataY(0)[i] = dataWS->readY(workspaceindex)[i];
        outputWS->dataY(1)[i] = values[i];
        outputWS->dataY(2)[i] = outputWS->dataY(0)[i] - outputWS->dataY(1)[i];
    }

    // 5. Background and pattern w/o background.
    mBackgroundFunction->function(domain, values);
    for (size_t i = 0; i < values.size(); ++i)
    {
        outputWS->dataY(3)[i] = outputWS->readY(1)[i] - values[i];
        outputWS->dataY(4)[i] = values[i];
    }

    // 4. Do peak calculation for all peaks, and append to output workspace
    bool ploteachpeak = this->getProperty("PlotIndividualPeaks");
    if (ploteachpeak)
    {
        for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
        {
            int hkl2 = mPeakHKL2[ipk];
            CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = mPeaks[hkl2];
            if (!peak)
            {
                g_log.information() << "[Warning] There is no peak corresponding to (HKL)^2 = " << hkl2 << std::endl;
            }
            else
            {
                peak->function(domain, values);
                for (size_t i = 0; i < domain.size(); ++i)
                {
                    outputWS->dataX(ipk+5)[i] = domain[i];
                }
                for (size_t i = 0; i < values.size(); ++i)
                {
                    outputWS->dataY(ipk+5)[i] = values[i];
                }
            }
        }
    }

    return;
}


/** Make output workspace valid if there is some error.
  */
void LeBailFit::fakeOutputData(size_t workspaceindex, int functionmode)
{
  // 1. Initialization
  g_log.notice() << "Input peak parameters are incorrect.  Fake output data for function mode "
                 << functionmode << std::endl;

  if (functionmode == 2)
  {
    std::stringstream errmsg;
    errmsg << "Function mode " << functionmode << " is not supported for fake output data.";
    g_log.error() << errmsg.str() << std::endl;
    throw std::invalid_argument(errmsg.str());
  }

  // 2. X&Y values
  const MantidVec& IX = dataWS->readX(workspaceindex);
  for (size_t iw = 0; iw < outputWS->getNumberHistograms(); ++iw)
  {
    MantidVec& X = outputWS->dataX(iw);
    for (size_t i = 0; i < X.size(); ++i)
    {
      X[i] = IX[i];
    }

    MantidVec& Y = outputWS->dataY(iw);
    if (iw == 0)
    {
      const MantidVec& IY = dataWS->readY(workspaceindex);
      for (size_t i = 0; i < IY.size(); ++i)
        Y[i] = IY[i];
    }
    else
    {
      for (size_t i = 0; i < Y.size(); ++i)
        Y[i] = 0.0;
    }

  }

  return;
}


/*
 * LeBail Fitting for one self-consistent iteration
 */
void LeBailFit::doLeBailFit(size_t workspaceindex)
{
    // 1. Get a copy of input function parameters (map)
    std::map<std::string, Parameter> parammap;
    parammap = mFuncParameters;

    // 2. Do 1 iteration of LeBail fit
    this->unitLeBailFit(workspaceindex, parammap);

    // 3. Output
    mFuncParameters = parammap;

    return;
}

/*
 * Calculate background of the specified diffraction pattern
 * by
 * 1. fix the peak parameters but height;
 * 2. fit only heights of the peaks in a peak-group and background coefficients (assumed order 2 or 3 polynomial)
 * 3. remove peaks by the fitting result
 */
void LeBailFit::calBackground(size_t workspaceindex)
{
  // 0. Set peak parameters to each peak
  // 1. Set parameters to each peak
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pit;
  for (pit = mPeaks.begin(); pit != mPeaks.end(); ++pit)
  {
    int hkl2 = pit->first;
    double peakheight = mPeakHeights[hkl2];
    setPeakParameters(mPeaks[hkl2], mFuncParameters, peakheight);
  }

  // 1. Split all the peaks into groups
  std::vector<std::set<size_t> > peakgroups;
  peakgroups = this->splitPeaksToGroups(); // this can be refactored from some exisiting functions

  int bkgdfuncorder = this->getProperty("BackgroundFunctionOrder");

  // 2. Do fit for each peak group
  for (size_t ipg = 0; ipg < peakgroups.size(); ++ipg)
  {
    // a. Construct the composite function
    API::CompositeFunction tempfunction;
    API::CompositeFunction_sptr groupedpeaks = boost::make_shared<API::CompositeFunction>(tempfunction);

    g_log.debug() << "DBx445 Peak Group " << ipg << std::endl;

    double tof_min = dataWS->readX(workspaceindex).back()+ 1.0;
    double tof_max = dataWS->readX(workspaceindex)[0] - 1.0;

    /// Add peaks and set up peaks' parameters
    std::set<size_t> peakindices = peakgroups[ipg];
    std::set<size_t>::iterator psiter;

    std::vector<int> hklslookup;
    int funcid = 0;
    for (psiter = peakindices.begin(); psiter != peakindices.end(); ++psiter)
    {
      // i. Add peak function in the composite function
      size_t indpeak = *psiter;
      int hkl2 = mPeakHKL2[indpeak]; // key
      hklslookup.push_back(hkl2);
      groupedpeaks->addFunction(mPeaks[hkl2]);

      mPeakGroupMap.insert(std::make_pair(hkl2, ipg));

      // ii. Set up peak function, i.e., tie peak parameters except Height
      std::vector<std::string> peakparamnames = mPeaks[hkl2]->getParameterNames();
      for (size_t im = 0; im < peakparamnames.size(); ++im)
      {
        std::string parname = peakparamnames[im];
        if (parname.compare("Height"))
        {
          double parvalue = mPeaks[hkl2]->getParameter(parname);

          // If not peak height
          std::stringstream ssname, ssvalue;
          ssname << "f" << funcid << "." << parname;
          ssvalue << parvalue;
          groupedpeaks->tie(ssname.str(), ssvalue.str());
          groupedpeaks->setParameter(ssname.str(), parvalue);
        }
      }

      // iii. find fitting boundary
      double fwhm = mPeaks[hkl2]->fwhm();
      double center = mPeaks[hkl2]->centre();
      g_log.debug() << "DB1201 Peak Index " << indpeak << " @ TOF = " << center << " FWHM =" << fwhm << std::endl;

      double leftbound = center-mPeakRadius*0.5*fwhm;
      double rightbound = center+mPeakRadius*0.5*fwhm;
      if (leftbound < tof_min)
      {
        tof_min = leftbound;
      }
      if (rightbound > tof_max)
      {
        tof_max = rightbound;
      }

      // iv. Progress function id
      ++ funcid;

    } // FOR 1 Peak in PeaksGroup

    /// Background (Polynomial)
    std::string backgroundtype = getProperty("BackgroundType");
    std::vector<double> orderparm;
    for (size_t iod = 0; iod <= size_t(bkgdfuncorder); ++iod)
    {
      orderparm.push_back(0.0);
    }
    CurveFitting::BackgroundFunction_sptr backgroundfunc = this->generateBackgroundFunction(backgroundtype, orderparm);

    groupedpeaks->addFunction(backgroundfunc);

    g_log.debug() << "DB1217 Composite Function of Peak Group: " << groupedpeaks->asString()
                        << std::endl << "Boundary: " << tof_min << ", " << tof_max << std::endl;

    // b. Fit peaks in the peak group
    /* Disabled to find memory leak */
    double unitprog = double(ipg)*0.9/double(peakgroups.size());
    API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", double(ipg)*unitprog, double(ipg+1)*unitprog, true);
    fitalg->initialize();

    fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(groupedpeaks));
    fitalg->setProperty("InputWorkspace", dataWS);
    fitalg->setProperty("WorkspaceIndex", int(workspaceindex));
    fitalg->setProperty("StartX", tof_min);
    fitalg->setProperty("EndX", tof_max);
    // fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg->setProperty("Minimizer", mMinimizer);
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 4000);

    // c. Execute
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      g_log.error() << "Fitting to LeBail function failed. " << std::endl;
      continue;
    }

    double chi2 = fitalg->getProperty("OutputChi2overDoF");
    std::string fitstatus = fitalg->getProperty("OutputStatus");

    g_log.information() << "LeBailFit (Background) Fit result:  Chi^2 = " << chi2
                        << " Fit Status = " << fitstatus << std::endl;

    mPeakGroupFitChi2Map.insert(std::make_pair(ipg, chi2));
    mPeakGroupFitStatusMap.insert(std::make_pair(ipg, fitstatus));

    // d. Get status and fitted parameter
    if (fitstatus.compare("success") == 0 && chi2 < 1.0E6)
    {
      // A successful fit
      /*
            API::IFunction_sptr fitout = fitalg->getProperty("Function");
            std::vector<std::string> parnames = fitout->getParameterNames();
            std::map<size_t, double> peakheights;
            for (size_t ipn = 0; ipn < parnames.size(); ++ipn)
            {
                /// Parameter names from function are in format fx.name.
                std::string funcparname = parnames[ipn];
                std::string parname;
                size_t functionindex;
                parseCompFunctionParameterName(funcparname, parname, functionindex);

                g_log.information() << "Parameter Name = " << parname << "(" << funcparname
                                    << ") = " << fitout->getParameter(funcparname) << std::endl;

                if (parname.compare("Height") == 0)
                {
                    peakheights.insert(std::make_pair(functionindex, fitout->getParameter(funcparname)));
                }
            }
            */

      // e. Check peaks' heights that they must be positive or 0.
      //    Give out warning if it doesn't seem right;
      for (size_t ipk = 0; ipk < hklslookup.size(); ++ipk)
      {
        // There is no need to set height as after Fit the result is stored already.
        // double height = peakheights[ipk];

        int hkl2 = hklslookup[ipk];
        double height = mPeaks[hkl2]->getParameter("Height");
        if (height < 0)
        {
          g_log.error() << "Fit for peak (HKL^2 = " << hkl2 << ") is wrong.  Peak height cannot be negative! " << std::endl;
          height = 0.0;
          mPeaks[hkl2]->setParameter("Height", height);
        }

        // g_log.information() << "DBx507  Peak " << hkl2 << " Height.  Stored In Peak = " << mPeaks[hkl2]->getParameter("Height")
        //       << "  vs. From Fit = " << height << std::endl;
      }

    }
  } // for all peak-groups

  // 4. Combine output and calcualte for background
  // Spectrum 0: Original data
  // Spectrum 1: Background calculated (important output)
  // Spectrum 2: Peaks without background

  // a) Reset background function
  std::vector<std::string> paramnames;
  paramnames = mBackgroundFunction->getParameterNames();
  std::vector<std::string>::iterator pariter;
  for (pariter = paramnames.begin(); pariter != paramnames.end(); ++pariter)
  {
    mBackgroundFunction->setParameter(*pariter, 0.0);
  }

  API::FunctionDomain1DVector domain(dataWS->readX(workspaceindex));
  API::FunctionValues values(domain);
  mLeBailFunction->function(domain, values);

  for (size_t i = 0; i < dataWS->readX(workspaceindex).size(); ++i)
  {
    outputWS->dataX(0)[i] = domain[i];
    outputWS->dataX(1)[i] = domain[i];
    outputWS->dataX(2)[i] = domain[i];

    double y = outputWS->dataY(workspaceindex)[i];
    outputWS->dataY(0)[i] = y;
    outputWS->dataY(1)[i] = y-values[i];
    outputWS->dataY(2)[i] = values[i];

    double e = outputWS->dataE(workspaceindex)[i];
    outputWS->dataE(0)[i] = e;
    double eb = 1.0;
    if ( fabs(outputWS->dataY(1)[i]) > 1.0)
    {
      eb = sqrt(fabs(outputWS->dataY(1)[i]));
    }
    outputWS->dataE(1)[i] = eb;
    outputWS->dataE(2)[i] = sqrt(values[i]);
  }

  return;
}

/// =================================== Pattern calculation ================================================ ///
/*
 * Calculate diffraction pattern from a more fexible parameter map
 */
void LeBailFit::calculateDiffractionPattern(
    size_t workspaceindex, API::FunctionDomain1DVector domain, API::FunctionValues& values,
    std::map<std::string, Parameter > parammap, bool recalpeakintesity)
{
  // 1. Set parameters to each peak
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pit;
  for (pit = mPeaks.begin(); pit != mPeaks.end(); ++pit)
  {
    int hkl2 = pit->first;
    double peakheight = mPeakHeights[hkl2];
    setPeakParameters(mPeaks[hkl2], parammap, peakheight);
  }

  // 2. Calculate peak intensities
  if (recalpeakintesity)
  {
    // Re-calcualte peak intensity
    std::vector<std::pair<int, double> > peakheights;
    this->calPeaksIntensities(peakheights, workspaceindex);

    std::stringstream msg;
    msg << "[DB1209 Pattern Calcuation]  Number of Peaks = " << peakheights.size() << std::endl;

    for (size_t ipk = 0; ipk < peakheights.size(); ++ipk)
    {
      int hkl2 = peakheights[ipk].first;
      CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = mPeaks[hkl2];
      if (!peak)
      {
        g_log.error() << "No peak corresponds to (HKL)^2 = " << hkl2 << std::endl;
      }
      else
      {
        peak->setParameter("Height", peakheights[ipk].second);
      }

      /* Debug output */
      msg << "(HKL)^2 = " << setw(3) << hkl2 << ", H = " << std::setw(7) << std::setprecision(5) << peakheights[ipk].second;
      if ((ipk+1) % 4 == 0)
      {
        msg << std::endl;
      }
      else
      {
        msg << ";  ";
      }
    } // ENDFOR: Peak

    g_log.information() << msg.str() << std::endl;
    /* END OF DB Output */
  }

  // 3. Calcualte model pattern
  mLeBailFunction->function(domain, values);

  return;
}

/*
 * Split peaks to peak groups.  Peaks in same peak group are connected.
 * The codes here are refactored from method calPeakIntensities()
 */
std::vector<std::set<size_t> > LeBailFit::splitPeaksToGroups()
{
  // 1. Sort peaks list by HKL^2:
  //    FIXME: This sorting scheme may be broken, if the crystal is not cubical!
  //           Consider to use d-spacing as the key to sort
  g_log.debug() << "DBx428 PeakHKL2 Size = " << mPeakHKL2.size() << std::endl;
  std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), CurveFitting::compDescending);

  // 2. Calculate the FWHM of each peak THEORETICALLY: Only peakcenterpairs is in order of peak position.
  //    Others are in input order of peaks
  std::vector<double> peakfwhms;
  std::vector<std::pair<double, double> > peakboundaries;
  std::vector<std::pair<double, size_t> > peakcenterpairs;

  //    Obtain each peak's center and range from calculation
  //    Must be in the descending order of HKL2 for adjacent peaks
  double boundaryconst = double(mPeakRadius);
  for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
  {
    int hkl2 = mPeakHKL2[ipk]; // key
    double fwhm = mPeaks[hkl2]->fwhm();
    double center = mPeaks[hkl2]->centre();

    double tof_left = center - 0.5*boundaryconst*fwhm;
    double tof_right = center + 0.5*boundaryconst*fwhm;

    peakfwhms.push_back(fwhm);
    peakcenterpairs.push_back(std::make_pair(center, ipk));
    peakboundaries.push_back(std::make_pair(tof_left, tof_right));

    g_log.debug() << "DB1659 Peak " << ipk << ":  FWHM = " << fwhm << " @ TOF = " << center << std::endl;
  }

  // 3. Regroup peaks. record peaks in groups; peaks in same group are very close
  std::vector<std::set<size_t> > peakgroups; /// index is for peak groups.  Inside, are indices for mPeakHKL2
  std::set<size_t> peakindices; /// Temporary buffer for peak indices of same group

  // a) Check: peak center should be increasing
  for (size_t i = 1; i < peakcenterpairs.size(); ++i)
  {
    if (peakcenterpairs[i].first <= peakcenterpairs[i-1].first)
    {
      g_log.error() << "Vector peakcenters does not store peak centers in an ascending order! It is not allowed" << std::endl;
      throw std::runtime_error("PeakCenters does not store value in ascending order.");
    }
  }

  // b) Go around
  for (size_t i = 0; i < peakcenterpairs.size(); ++i)
  {
    if (peakindices.size() > 0)
    {
      /// There are peaks in the group already
      size_t leftpeakindex = i-1;
      double leftpeak_rightbound = peakboundaries[leftpeakindex].second;

      double thispeak_leftbound = peakboundaries[i].first;

      if (thispeak_leftbound > leftpeak_rightbound)
      {
        /// current peak has no overlap with previous peak, store the present one and start a new group
        std::set<size_t> settoinsert = peakindices;
        peakgroups.push_back(settoinsert);
        peakindices.clear();
      }
    }

    // Insert the current peak index to set
    size_t ipk = peakcenterpairs[i].second;
    peakindices.insert(ipk);
  } // ENDFOR

  // Insert the last group
  peakgroups.push_back(peakindices);

  g_log.information() << "Split peaks to peak groups.  Size of peaks groups = "
                      << peakgroups.size() << std::endl;

  return peakgroups;
}


/*
 * Calculate peak heights from the model to the observed data
 * Algorithm will deal with
 * (1) Peaks are close enough to overlap with each other
 * The procedure will be
 * (a) Assign peaks into groups; each group contains either (1) one peak or (2) peaks overlapped
 * (b) Calculate peak intensities for every peak per group
 */
void LeBailFit::calPeaksIntensities(std::vector<std::pair<int, double> >& peakheights, size_t workspaceindex)
{
  // 0. Prepare
  peakheights.clear();
  const MantidVec& X = dataWS->readX(workspaceindex);

  // 1) Sort peaks list by HKL^2:
  //    FIXME: This sorting scheme may be broken, if the crystal is not cubical!
  if (mPeakHKL2.size() > 0)
  {
    std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), CurveFitting::compDescending);
  }
  else
  {
    std::stringstream errmsg;
    errmsg << "[LeBailFit] calPeaksIntensities(): No peak is found in the instance object mPeaksHKL. ";
    g_log.error() << errmsg.str() << std::endl;
    throw std::runtime_error(errmsg.str());
  }

  // 1. Calculate the FWHM of each peak: Only peakcenterpairs is in order of peak position.
  //    Others are in input order of peaks
  std::vector<double> peakfwhms;
  std::vector<double> peakcenters;
  std::vector<std::pair<double, double> > peakboundaries;
  std::vector<std::pair<double, size_t> > peakcenterpairs;

  // 2. Obtain each peak's center and range from calculation
  //    Must be in the descending order of HKL2 for adjacent peaks
  for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
  {
    int hkl2 = mPeakHKL2[ipk]; // key
    double fwhm = mPeaks[hkl2]->fwhm();
    double center = mPeaks[hkl2]->centre();

    /* Give up using observation result due to some peak with narrow peak width
        double tof_left, tof_right, tof_center;
        this->estimatePeakRange(workspaceindex, center, fwhm, tof_center, tof_left, tof_right);
        */

    double tof_left = center - fwhm;
    if (tof_left < X[0])
    {
      tof_left = X[0];
    }
    double tof_right = center + fwhm;
    if (tof_right > X.back())
    {
      tof_right = X.back();
    }

    peakfwhms.push_back(fwhm);
    peakcenters.push_back(center);
    peakcenterpairs.push_back(std::make_pair(center, ipk));
    peakboundaries.push_back(std::make_pair(tof_left, tof_right));

    g_log.debug() << "DB1659 Peak " << ipk << ":  FWHM = " << fwhm << " @ TOF = " << center << std::endl;
  }

  // 3. Regroup peaks. record peaks in groups; peaks in same group are very close
  std::vector<std::set<size_t> > peakgroups; // index is for peak groups.  Inside, are indices for mPeakHKL2
  std::set<size_t> peakindices;
  double boundaryconst = double(mPeakRadius);

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

  // 4. Calculate each peak's intensity and set
  std::vector<std::pair<size_t, double> > peakintensities;
  for (size_t ig = 0; ig < peakgroups.size(); ++ig)
  {
    std::vector<std::pair<size_t, double> > tempintensities; // index is mPeakHKL2's
    this->calPerGroupPeaksIntensities(workspaceindex, peakgroups[ig], peakcenters, peakboundaries, tempintensities);

    // b) Sort in order of mPeakHKL2
    std::sort(tempintensities.begin(), tempintensities.end());
    for (size_t ipk = 0; ipk < tempintensities.size(); ++ipk)
    {
      int hkl2 = mPeakHKL2[tempintensities[ipk].first];
      double height = tempintensities[ipk].second;
      peakheights.push_back(std::make_pair(hkl2, height));
    }

    peakintensities.insert(peakintensities.end(), tempintensities.begin(), tempintensities.end());
  }

  return;
}

/*
 * Calculate peak intensities for each group of peaks
 * Input: (1) Workspace Index (2) Peaks' indicies in mPeakHKL2  (3) Peaks' centers
 *        (4) Peaks' boundaries
 * Output: Peak intensities (index is peaks' indicies in mPeakHKL2)
 */
void LeBailFit::calPerGroupPeaksIntensities(size_t wsindex, std::set<size_t> groupedpeaksindexes, std::vector<double> peakcenters,
                                            std::vector<std::pair<double, double> > peakboundaries,
                                            std::vector<std::pair<size_t, double> >& peakintensities)
{
  // 1. Determine range of the peak group
  if (groupedpeaksindexes.size() == 0)
  {
    std::stringstream errmsg;
    errmsg << "DB252 Group Size = " << groupedpeaksindexes.size() << " is not allowed. " << std::endl;
    throw std::runtime_error(errmsg.str());
  }
  else
  {
    g_log.debug() << "DB252 Group Size = " << groupedpeaksindexes.size() << " including peak indexed " << std::endl;
  }

  //   Clean output
  peakintensities.clear();

  std::vector<size_t> peaks; // index for mPeakHKL2
  std::set<size_t>::iterator pit;
  for (pit = groupedpeaksindexes.begin(); pit != groupedpeaksindexes.end(); ++pit)
  {
    peaks.push_back(*pit);
    g_log.debug() << "Peak index = " << *pit << std::endl;
  }

  if (peaks.size() > 1)
    std::sort(peaks.begin(), peaks.end());

  size_t iLeftPeak = peaks[0];
  double leftpeakcenter = peakcenters[iLeftPeak];
  double mostleftpeakwidth = -peakboundaries[iLeftPeak].first+peakcenters[iLeftPeak];
  double leftbound = leftpeakcenter-PEAKRANGECONSTANT*mostleftpeakwidth;

  size_t iRightPeak = peaks.back();
  double rightpeakcenter = peakcenters[iRightPeak];
  double mostrightpeakwidth = peakboundaries[iRightPeak].second-peakcenters[iRightPeak];
  double rightbound = rightpeakcenter+PEAKRANGECONSTANT*mostrightpeakwidth;

  g_log.debug() << "DB1204 Left bound = " << leftbound << " (" << iLeftPeak << "), Right bound = " << rightbound << "(" << iRightPeak << ")."
                << std::endl;

  // 1.5 Return if the complete peaks' range is out side of all data (boundary)
  if (leftbound < dataWS->readX(wsindex)[0] || rightbound > dataWS->readX(wsindex).back())
  {
    for (size_t i = 0; i < peaks.size(); ++i)
    {
      peakintensities.push_back(std::make_pair(peaks[i], 0.0));
    }
    return;
  }

  // 2. Obtain access of dataWS to calculate from
  const MantidVec& datax = dataWS->readX(wsindex);
  const MantidVec& datay = dataWS->readY(wsindex);
  std::vector<double>::const_iterator cit;

  cit = std::lower_bound(datax.begin(), datax.end(), leftbound);
  size_t ileft = size_t(cit-datax.begin());
  if (ileft > 0)
    ileft -= 1;

  cit = std::lower_bound(datax.begin(), datax.end(), rightbound);
  size_t iright = size_t(cit-datax.begin());
  if (iright < datax.size()-1)
    iright += 1;

  if (iright <= ileft)
  {
    g_log.error() << "Try to integrate peak from " << leftbound << " To " << rightbound << std::endl <<
                     "  Peak boundaries : " << peakboundaries[peaks[0]].first << ", " << peakboundaries[peaks[0]].second <<
                     "  Peak center: " << peakcenters[peaks[0]] << "  ... " << peakcenters[peaks.back()] << std::endl;
    throw std::logic_error("iRight cannot be less or equal to iLeft.");
  }
  else
  {
    g_log.debug() << "DB452 Integrate peak from " << leftbound << "/"
                  << ileft << " To " << rightbound << "/" << iright << std::endl;
  }

  // 3. Integrate
  size_t ndata = iright - ileft + 1;
  std::vector<double>::const_iterator xbegin = datax.begin()+ileft;
  std::vector<double>::const_iterator xend;
  if (iright+1 <= datax.size()-1)
    xend = datax.begin()+iright+1;
  else
    xend = datax.end();
  std::vector<double> reddatax(xbegin, xend); // reduced-size data x

  g_log.debug() << "DBx356:  ndata = " << ndata << " Reduced data range: " << reddatax[0] << ", " << reddatax.back() << std::endl;

  API::FunctionDomain1DVector xvalues(reddatax);
  std::vector<double> sumYs;
  sumYs.reserve(xvalues.size());

  for (size_t iy = 0; iy < xvalues.size(); ++iy)
  {
    sumYs.push_back(0.0);
  }

  std::vector<API::FunctionValues> peakvalues;
  std::vector<API::FunctionValues> bkgdvalues;

  // Integrate each peak
  std::stringstream errmsg;
  for (size_t i = 0; i < peaks.size(); ++i)
  {
    // For each peak in peaks group
    size_t peakindex = peaks[i];
    int hkl2 = mPeakHKL2[peakindex];
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr ipeak = mPeaks[hkl2];
    if (!ipeak)
      throw std::runtime_error("Not a peak function at all. ");
    API::FunctionValues yvalues(xvalues);
    API::FunctionValues bvalues(xvalues);

    ipeak->function(xvalues, yvalues);
    peakvalues.push_back(yvalues);

    mBackgroundFunction->function(xvalues, bvalues);
    bkgdvalues.push_back(bvalues);

    g_log.debug() << "DBx359 Integrate Peak " << peakindex << ": (HKL)^2 = " << hkl2
                  << " TOF_h = " << ipeak->centre() << " within [" << xvalues[0]
                  << ", " << xvalues[xvalues.size()-1] << "]" << std::endl;

    bool dataphysical = true;
    size_t numbadpts = 0;
    for (size_t j = 0; j < ndata; ++j)
    {
      if (yvalues[j] < DBL_MAX && yvalues[j] > -DBL_MAX)
      {
        sumYs[j] += yvalues[j];
      }
      else
      {
        // FIXME:  Need some short/informative debug output for this situation.
        g_log.debug() << "Warning1030 Peak value @ " << xvalues[j] << " is infinity.  Peak's FWHM = "
                      << ipeak->fwhm() << ", Height = " << ipeak->height() << std::endl;
        sumYs[j] += yvalues[j];
        dataphysical = false;
        numbadpts ++;
      }
    }

    if (!dataphysical)
    {
      errmsg << "TOF_h = " << std::setprecision(6) << ipeak->centre() << "; "; // ", (HKL)^2 = " << hkl2 << "
    }
  }

  // FIXME Requiring better error message output
  if (errmsg.str().size() > 0)
  {
    g_log.warning() << "Unphysical Peak Values: " << errmsg.str() << std::endl;
  }

  // 3. Calculate intensity for each peak
  for (size_t i = 0; i < peaks.size(); ++i)
  {
    double intensity = 0.0;

    for (size_t j = 0; j < sumYs.size(); ++j)
    {
      if (sumYs[j] > 1.0E-5)
      {
        // Remove background from observed data
        double temp;
        if (sumYs[j] > 1.0E-5)
        {
          temp = (datay[ileft+j]-bkgdvalues[i][j])*peakvalues[i][j]/sumYs[j];
        }
        else
        {
          temp = 0.0;
        }

        intensity += temp*(datax[ileft+j+1]-datax[ileft+j]);

        if (intensity != intensity)
        {
          // case of NaN
          g_log.information() << "[Warning] Unphysical intensity.  Temp = " << temp << "  SumY = " << sumYs[j]
                          << "  Peak value = " << peakvalues[i][j] << " Data = " << datay[ileft+j]
                          << "  Background = " << bkgdvalues[i][j] << std::endl;
        }
      }
    } // FOR: j

    // b) Check intensity
    if (intensity < 0.0)
    {
      intensity = 0.0;
    }

    peakintensities.push_back(std::make_pair(peaks[i], intensity));
    g_log.debug() << "DBx406 Result Per Group: Peak " << peaks[i] << "  Height = " << intensity << std::endl;
  }

  return;
}

/*
 * From table/map to set parameters to an individual peak
 */
void LeBailFit::setPeakParameters(
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak,
        std::map<std::string, Parameter> parammap, double peakheight)
{
    // 1. Set parameters ...
    std::map<std::string, Parameter>::iterator pit;

    std::vector<std::string> lebailparnames = peak->getParameterNames();
    std::sort(lebailparnames.begin(), lebailparnames.end());

    // 2. Apply parameters values to peak function
    for (pit = parammap.begin(); pit != parammap.end(); ++pit)
    {
        std::string parname = pit->first;
        double value = pit->second.value;

        // char fitortie = pit->second.second;

        g_log.debug() << "LeBailFit Set " << parname << "= " << value << std::endl;

        std::vector<std::string>::iterator ifind =
                std::find(lebailparnames.begin(), lebailparnames.end(), parname);
        if (ifind == lebailparnames.end())
        {
            g_log.debug() << "Parameter " << parname
                          << " in input parameter table workspace is not for peak function. " << std::endl;
            continue;
        }

        peak->setParameter(parname, value);

    } // ENDFOR: parameter iterator

    // 3. Peak height
    peak->setParameter("Height", peakheight);

    return;
}

/// =================================== Le Bail Fit (Fit Only) ================================================ ///

/*
 * Perform one itearation of LeBail fitting
 * Including
 * a) Calculate pattern for peak intensities
 * b) Set peak intensities
 */
bool LeBailFit::unitLeBailFit(size_t workspaceindex, std::map<std::string, Parameter>& parammap)
{
    // 1. Generate domain and value
    const std::vector<double> x = dataWS->readX(workspaceindex);
    API::FunctionDomain1DVector domain(x);
    API::FunctionValues values(domain);

    // 2. Calculate peak intensity and etc.
    bool calpeakintensity = true;
    this->calculateDiffractionPattern(workspaceindex, domain, values, parammap, calpeakintensity);

    // a) Apply initial calculated result to output workspace
    mWSIndexToWrite = 5;
    writeToOutputWorkspace(domain, values);

    // b) Calculate input background
    mBackgroundFunction->function(domain, values);
    mWSIndexToWrite = 6;
    writeToOutputWorkspace(domain, values);

    // 3. Construct the tie.  2-level loop. (1) peak parameter (2) peak
    this->setLeBailFitParameters();

    // 4. Construct the Fit
    this->fitLeBailFunction(workspaceindex, parammap);

    // 5. Do calculation again and set the output
    calpeakintensity = true;
    API::FunctionValues newvalues(domain);
    this->calculateDiffractionPattern(workspaceindex, domain, newvalues, parammap, calpeakintensity);

    // Add final calculated value to output workspace
    mWSIndexToWrite = 1;
    writeToOutputWorkspace(domain, newvalues);

    // Add original data and
    writeInputDataNDiff(workspaceindex, domain);

    return true;
}

/** Set up the fit/tie/set-parameter for LeBail Fit (mode)
 */
void LeBailFit::setLeBailFitParameters()
{
  // 1. Set up all the peaks' parameters... tie to a constant value.. or fit by tieing same parameters of among peaks
  std::map<std::string, Parameter>::iterator pariter;
  for (pariter = mFuncParameters.begin(); pariter != mFuncParameters.end(); ++pariter)
  {
    Parameter funcparam = pariter->second;

    g_log.debug() << "Step 1:  Set peak parameter " << funcparam.name << std::endl;

    std::string parname = pariter->first;
    double parvalue = funcparam.value;
    bool tofit = funcparam.fit;

    // a) Check whether it is a parameter used in Peak
    std::vector<std::string>::iterator sit;
    sit = std::find(mPeakParameterNames.begin(), mPeakParameterNames.end(), parname);
    if (sit == mPeakParameterNames.end())
    {
      // Not a peak profile parameter
      g_log.debug() << "Unable to tie parameter " << parname << " b/c it is not a parameter for peak. " << std::endl;
      continue;
    }

    if (!tofit)
    {
      // a) Tie the value to a constant number
      std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator piter;
      size_t peakindex = 0;
      for (piter = mPeaks.begin(); piter != mPeaks.end(); ++piter)
      {
        std::stringstream ss1, ss2;
        ss1 << "f" << peakindex << "." << parname;
        ss2 << parvalue;
        std::string tiepart1 = ss1.str();
        std::string tievalue = ss2.str();
        mLeBailFunction->tie(tiepart1, tievalue);
        g_log.debug() << "LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

        ++ peakindex;
      } // For each peak
    }
    else
    {
      // b) Tie the values among all peaks, but will fit
      for (size_t ipk = 1; ipk < mPeaks.size(); ++ipk)
      {
        std::stringstream ss1, ss2;
        ss1 << "f" << (ipk-1) << "." << parname;
        ss2 << "f" << ipk << "." << parname;
        std::string tiepart1 = ss1.str();
        std::string tiepart2 = ss2.str();
        mLeBailFunction->tie(tiepart1, tiepart2);
        g_log.debug() << "LeBailFit.  Fit(Tie) / " << tiepart1 << " / " << tiepart2 << " /" << std::endl;
      }

      // c) Set the constraint
      std::stringstream parss;
      parss << "f0." << parname;
      string parnamef0 = parss.str();
      CurveFitting::BoundaryConstraint* bc = new BoundaryConstraint(mLeBailFunction.get(), parnamef0, funcparam.minvalue, funcparam.maxvalue);
      mLeBailFunction->addConstraint(bc);

      /*
      for (size_t ipk = 0; ipk < mPeaks.size(); ++ipk)
      {
        std::stringstream parss;
        parss << "f" << ipk << "." << parname;
        string parnamef = parss.str();
        CurveFitting::BoundaryConstraint* bc = new BoundaryConstraint(mLeBailFunction.get(), parnamef, funcparam.minvalue, funcparam.maxvalue);
        mLeBailFunction->addConstraint(bc);
      }
      */
    }
  } // FOR-Function Parameters

  // 1B Set 'Height' to be fixed
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator peakiter;
  size_t peakindex = 0;
  for (peakiter = mPeaks.begin(); peakiter != mPeaks.end(); ++peakiter)
  {
    // a. Get peak height
    std::string parname("Height");
    double parvalue = peakiter->second->getParameter(parname);

    std::stringstream ss1, ss2;
    ss1 << "f" << peakindex << "." << parname;
    ss2 << parvalue;
    std::string tiepart1 = ss1.str();
    std::string tievalue = ss2.str();

    g_log.debug() << "Step 1B: LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

    mLeBailFunction->tie(tiepart1, tievalue);

    ++peakindex;

  } // For each peak

  // 2. Tie all background paramters to constants/current values
  size_t funcindex = mPeaks.size();
  std::vector<std::string> bkgdparnames = mBackgroundFunction->getParameterNames();
  for (size_t ib = 0; ib < bkgdparnames.size(); ++ib)
  {
    std::string parname = bkgdparnames[ib];
    double parvalue = mBackgroundFunction->getParameter(parname);
    std::stringstream ss1, ss2;
    ss1 << "f" << funcindex << "." << parname;
    ss2 << parvalue;
    std::string tiepart1 = ss1.str();
    std::string tievalue = ss2.str();

    g_log.debug() << "Step 2: LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

    mLeBailFunction->tie(tiepart1, tievalue);
  }

  return;
}

/** Fit LeBailFunction by calling Fit()
  * Be called after all functions in LeBailFunction (composite) are set up (tie, constrain)
  * Output: a parameter name-value map
 */
bool LeBailFit::fitLeBailFunction(size_t workspaceindex, std::map<std::string, Parameter> &parammap)
{
  // 1. Prepare fitting boundary parameters.
  double tof_min = dataWS->dataX(workspaceindex)[0];
  double tof_max = dataWS->dataX(workspaceindex).back();
  std::vector<double> fitrange = this->getProperty("FitRegion");
  if (fitrange.size() == 2 && fitrange[0] < fitrange[1])
  {
    // Properly defined
    tof_min = fitrange[0];
    tof_max = fitrange[1];
  }

  // 2. Call Fit to fit LeBail function.
  // a) Initialize
  std::string fitoutputwsrootname("xLeBailOutput");

  API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
  fitalg->initialize();

  g_log.debug() << "[Before Fit] Function To Fit: " << mLeBailFunction->asString() << std::endl;

  // b) Set property
  mLeBailFunction->useNumericDerivatives( true );
  fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(mLeBailFunction));
  fitalg->setProperty("InputWorkspace", dataWS);
  fitalg->setProperty("WorkspaceIndex", int(workspaceindex));
  fitalg->setProperty("StartX", tof_min);
  fitalg->setProperty("EndX", tof_max);
  fitalg->setProperty("Minimizer", mMinimizer); // default is "Levenberg-MarquardtMD"
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);
  fitalg->setProperty("CreateOutput", true);
  fitalg->setProperty("Output", fitoutputwsrootname);
  fitalg->setProperty("CalcErrors", true);

  // c) Execute
  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || ! successfulfit)
  {
    // Early return due to bad fit
    g_log.error() << "Fitting to LeBail function failed. " << std::endl;
    return false;
  }
  else
  {
    g_log.debug() << "[Fit Le Bail Function] Fitting successful. " << std::endl;
  }

  mLeBaiLFitChi2 = fitalg->getProperty("OutputChi2overDoF");
  std::string fitstatus = fitalg->getProperty("OutputStatus");

  API::ITableWorkspace_sptr covarws = fitalg->getProperty("OutputNormalisedCovarianceMatrix");
  if (covarws)
  {
    declareProperty(
          new API::WorkspaceProperty<API::ITableWorkspace>("OutputNormalisedCovarianceMatrix","",Kernel::Direction::Output),
          "The name of the TableWorkspace in which to store the final covariance matrix" );
    setPropertyValue("OutputNormalisedCovarianceMatrix", "NormalisedCovarianceMatrix");
    setProperty("OutputNormalisedCovarianceMatrix", covarws);
  }

  API::ITableWorkspace_sptr fitvaluews
      = (fitalg->getProperty("OutputParameters"));
  if (fitvaluews)
  {
    g_log.debug() << "DBx318 Got the table workspace.  Col No = " << fitvaluews->columnCount() << std::endl;
    for (size_t ir = 0; ir < fitvaluews->rowCount(); ++ir)
    {
      API::TableRow row = fitvaluews->getRow(ir);
      std::string parname;
      double parvalue, parerror;
      row >> parname >> parvalue >> parerror;
    }
  }

  // d) Get parameters
  API::IFunction_sptr fitout = fitalg->getProperty("Function");

  std::vector<std::string> parnames = fitout->getParameterNames();

  std::stringstream rmsg;
  rmsg << "Fitting Result: " << std::endl;
  for (size_t ip = 0; ip < parnames.size(); ++ip)
  {
    std::string parname = parnames[ip];
    double curvalue = fitout->getParameter(ip);
    double error = fitout->getError(ip);

    // split parameter string
    // FIXME These codes are duplicated as to method parseCompFunctionParameterName().  Refactor!
    std::vector<std::string> results;
    boost::split(results, parname, boost::is_any_of("."));

    if (results.size() != 2)
    {
      g_log.error() << "Parameter name : " << parname << " does not have 1 and only 1 (.).  Cannot support!" << std::endl;
      throw std::runtime_error("Unable to support parameter name to split.");
    }

    // Error only set in one parameter if multiple function has same parameter tied.
    if (error > 1.0E-5)
    {
      // Update the function parameters' error
      mFuncParameterErrors.insert(std::make_pair(parname, error));

      // Output
      std::string parnamex = results[1];
      if (parammap[parnamex].fit)
      {
        // Fit
        parammap[parnamex].value = curvalue;
        parammap[parnamex].fit = true;

        rmsg << std::setw(10) << parnamex << " = " << setw(7) << setprecision(5) << curvalue
             << ",    Error = " << setw(7) << setprecision(5) << error << std::endl;
      }
      else
      {
        g_log.warning() << " [Fitting Result] Parameter " << parnamex << " is not set to refine.  "
                        << "But its chi^2 =" << error << std::endl;
      }
    }
  }

  g_log.information() << rmsg.str();

  // e) Get parameter output workspace from it for error
  if (fitvaluews)
  {
    for (size_t ir = 0; ir < fitvaluews->rowCount(); ++ir)
    {
      // 1. Get row and parse
      API::TableRow row = fitvaluews->getRow(ir);
      std::string parname;
      double parvalue, parerror;
      row >> parname >> parvalue >> parerror;

      g_log.debug() << "Row " << ir << ": " << parname << " = " << parvalue << " +/- " << parerror << std::endl;

      // 2. Parse parameter and set it up if error is not zero
      if (parerror > 1.0E-2)
      {
        std::vector<std::string> results;
        boost::split(results, parname, boost::is_any_of("."));

        if (results.size() != 2)
        {
          g_log.error() << "Parameter name : " << parname << " does not have 1 and only 1 (.).  Cannot support!" << std::endl;
          throw std::runtime_error("Unable to support parameter name to split.");
        }

        mFuncParameterErrors.insert(std::make_pair(results[1], parerror));
      }
    }
  }

  // 3. Calculate Chi^2 wih all parmeters fixed
  vector<string> lbparnames = mLeBailFunction->getParameterNames();
  for (size_t i = 0; i < lbparnames.size(); ++i)
  {
    mLeBailFunction->fix(i);
  }

  API::IAlgorithm_sptr calalg = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
  calalg->initialize();
  calalg->setProperty("Function", boost::shared_ptr<API::IFunction>(mLeBailFunction));
  calalg->setProperty("InputWorkspace", dataWS);
  calalg->setProperty("WorkspaceIndex", int(workspaceindex));
  calalg->setProperty("StartX", tof_min);
  calalg->setProperty("EndX", tof_max);
  calalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  calalg->setProperty("CostFunction", "Least squares");
  calalg->setProperty("MaxIterations", 2);
  calalg->setProperty("CreateOutput", false);

  successfulfit = calalg->execute();
  if (!calalg->isExecuted() || ! successfulfit)
  {
    // Early return due to bad fit
    g_log.error() << "Fitting to LeBail function failed. " << std::endl;
    throw runtime_error("DBx457 This is not possible!");
  }

  mLeBailCalChi2 = calalg->getProperty("OutputChi2overDoF");
  g_log.notice() << "LeBailFit (LeBailFunction) Fit result:  Chi^2 (Fit) = " << mLeBaiLFitChi2
                 << ", Chi^2 (Cal) = " << mLeBailCalChi2
                 << ", Fit Status = " << fitstatus << std::endl;

  return true;
}

/* === Method deleted & Backed up in MantidBackup/20120816....cpp
 *
 * Reason to remove: Not used by any other methods
 *
 * Estimate peak center and peak range according to input information (from observation)
 * - center: user input peak center
 * - fwhm: user input fwhm
 * - tof_center: estimated peak center (output)
 * - tof_left: estimated left boundary at half maximum
 * - tof_right: estimated right boundary at half maximum
 * Return: False if no peak found (maximum value is at center+/-fwhm
 *
 * WARNING: This algorithm fails if the peak only has the width of very few pixels.
 *          because it heavily replies on observation data.
 *
 * bool LeBailFit::observePeakRange(size_t workspaceindex, double center, double fwhm,
    double& tof_center, double& tof_left, double& tof_right)
 */

/// =================================== Methods about input/output & create workspace ================================================ ///
/** Create and set up an output TableWorkspace for each individual peaks
 * Parameters include H, K, L, Height, TOF_h, PeakGroup, Chi^2, FitStatus
 * Where chi^2 and fit status are used only in 'CalculateBackground'
 */
void LeBailFit::exportEachPeaksParameters()
{
  // 1. Create peaks workspace
  DataObjects::TableWorkspace_sptr peakWS = DataObjects::TableWorkspace_sptr(new DataObjects::TableWorkspace);

  // 2. Set up peak workspace
  peakWS->addColumn("int", "H");
  peakWS->addColumn("int", "K");
  peakWS->addColumn("int", "L");
  peakWS->addColumn("double", "Height");
  peakWS->addColumn("double", "TOF_h");
  peakWS->addColumn("double", "Alpha");
  peakWS->addColumn("double", "Beta");
  peakWS->addColumn("double", "Sigma2");
  peakWS->addColumn("double", "Gamma");
  peakWS->addColumn("double", "FWHM");
  peakWS->addColumn("int", "PeakGroup");
  peakWS->addColumn("double", "Chi^2");
  peakWS->addColumn("str", "FitStatus");

  // 3. Construct a list
  std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), compDescending);

  for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
  {
    // a. Access peak function
    int hkl2 = mPeakHKL2[ipk];
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr tpeak = mPeaks[hkl2];

    // b. Get peak's nature parameters
    int h, k, l;
    tpeak->getMillerIndex(h, k, l);
    double tof_h = tpeak->centre();
    double height = tpeak->height();
    double alpha = tpeak->getPeakParameters("Alpha");
    double beta = tpeak->getPeakParameters("Beta");
    double sigma2 = tpeak->getPeakParameters("Sigma2");
    double gamma = tpeak->getPeakParameters("Gamma");
    double fwhm = tpeak->fwhm();

    // c. Get peak's fitting and etc.
    size_t peakgroupindex = mPeaks.size()+10; // Far more than max peak group index
    std::map<int, size_t>::iterator git;
    git = mPeakGroupMap.find(hkl2);
    if (git != mPeakGroupMap.end())
    {
      peakgroupindex = git->second;
    }

    double chi2 = -1.0;
    std::string fitstatus("No Fit");

    std::map<size_t, double>::iterator cit;
    std::map<size_t, std::string>::iterator sit;
    cit = mPeakGroupFitChi2Map.find(peakgroupindex);
    if (cit != mPeakGroupFitChi2Map.end())
    {
      chi2 = cit->second;
    }
    sit = mPeakGroupFitStatusMap.find(peakgroupindex);
    if (sit != mPeakGroupFitStatusMap.end())
    {
      fitstatus = sit->second;
    }

    /// Peak group index converted to integer
    int ipeakgroupindex = -1;
    if (peakgroupindex < mPeaks.size())
    {
      ipeakgroupindex = int(peakgroupindex);
    }

    API::TableRow newrow = peakWS->appendRow();
    if (tof_h < 0)
    {
      g_log.error() << "For peak (HKL)^2 = " << hkl2 << "  TOF_h is NEGATIVE!" << std::endl;
    }
    newrow << h << k << l << height << tof_h << alpha << beta << sigma2 << gamma << fwhm
           << ipeakgroupindex << chi2 << fitstatus;
  }

  // 4. Set
  this->setProperty("OutputPeaksWorkspace", peakWS);

  return;
}


/** Generate a list of peaks from input
  * Initial screening will be made to exclude peaks out of data range
  * The peak parameters will be set up to each peak
  * If the peak parameters are invalid:
  * (1) alpha < 0
  * (2) beta < 0
  * (3) sigma2 < 0
  * An error message will be spit out and there will be an early return
  *
  * RETURN:  True if no peak parameters is wrong!
 */
bool LeBailFit::generatePeaksFromInput(size_t workspaceindex)
{   
  // There is no need to consider peak's order now due to map

  //1. Generate peaks
  size_t numpeaksoutofrange = 0;
  size_t numpeaksparamerror = 0;
  bool peakparametererror = false;

  double tofmin = dataWS->readX(workspaceindex)[0];
  double tofmax =  dataWS->readX(workspaceindex).back();

  for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
  {
    // 1. Generate peak
    int h = mPeakHKLs[ipk][0];
    int k = mPeakHKLs[ipk][1];
    int l = mPeakHKLs[ipk][2];
    int hkl2 = h*h+k*k+l*l;

    CurveFitting::ThermalNeutronBk2BkExpConvPV tmppeak;
    tmppeak.setMillerIndex(h, k, l);
    tmppeak.initialize();
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr speak = boost::make_shared
        <CurveFitting::ThermalNeutronBk2BkExpConvPV>(tmppeak);

    // 2. Set peak function
    this->setPeakParameters(speak, this->mFuncParameters, false);

    // 3. Check peak parameters
    double eta, alpha, beta, H, sigma2, gamma, N, tof_h;
    speak->calculateParameters(tof_h, eta, alpha, beta, H, sigma2, gamma, N, false);
    speak->setPeakRadius(mPeakRadius);

    // 4. Exclude peak out of range
    if (tof_h < tofmin || tof_h > tofmax)
    {
      g_log.debug() << "Input peak (" << h << ", " << k << ", " << l << ") is out of range. "
                    << "TOF_h = " << tof_h << std::endl;
      numpeaksoutofrange ++;
      continue;
    }

    // 5. Check peak parameters' validity
    // double alpha = speak->getParameter("Alpha");
    if (alpha != alpha || alpha <= 0)
    {
      g_log.information() << "[Warning] Peak (" << h << ", " << k << ", " << l << ") Alpha = " <<
                         alpha << " is not physical!" << std::endl;
      peakparametererror = true;
    }
    // double beta = speak->getParameter("Beta");
    if (beta != beta || beta <= 0)
    {
      g_log.information() << "[Warning] Peak (" << h << ", " << k << ", " << l << ") Beta = " <<
                         beta << " is not physical!" << std::endl;
      peakparametererror = true;
    }
    // double sigma2 = speak->getParameter("Sigma2");
    if (sigma2 != sigma2 || sigma2 <= 0)
    {
      g_log.information() << "[Warning] Peak (" << h << ", " << k << ", " << l << ") Sigma^2 = " <<
                         sigma2 << " is not physical!" << std::endl;
      peakparametererror = true;
    }

    if (peakparametererror)
    {
      ++ numpeaksparamerror;
    }

    // 6. Add peak to peak map
    mPeaks.insert(std::make_pair(hkl2, speak));
    std::vector<int>::iterator fit = std::find(mPeakHKL2.begin(), mPeakHKL2.end(), hkl2);
    if (fit != mPeakHKL2.end())
    {
      std::stringstream errmsg;
      errmsg << "H^2+K^2+L^2 = " << hkl2 << " already exists. This situation is not considered";
      g_log.error()  << errmsg.str() << std::endl;
      throw std::invalid_argument(errmsg.str());
    }
    else
    {
      mPeakHKL2.insert(fit, hkl2);
    }

  }

  g_log.information() << "Number of ... Input Peaks = " << mPeakHKLs.size() << "; Peaks Generated: " << mPeakHKL2.size()
                 << "; Peaks With Error Parameters = " << numpeaksparamerror
                 << "; Peaks Outside Range = " <<  numpeaksoutofrange
                 << "; Range: " << setprecision(5) << tofmin << ", " << setprecision(5) << tofmax <<  std::endl;

  // 4. Set the parameters' names
  mPeakParameterNames.clear();
  if (mPeaks.size() > 0)
  {
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = mPeaks.begin()->second;
    mPeakParameterNames = peak->getParameterNames();
  }
  std::sort(mPeakParameterNames.begin(), mPeakParameterNames.end());

  return (!peakparametererror);
}

/** Generate background function accroding to input: mBackgroundFunction
 */
CurveFitting::BackgroundFunction_sptr LeBailFit::generateBackgroundFunction(std::string backgroundtype,
                                                                            std::vector<double> bkgdparamws)
{
    auto background = API::FunctionFactory::Instance().createFunction(backgroundtype);
    CurveFitting::BackgroundFunction_sptr bkgdfunc = boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(background);

    size_t order = bkgdparamws.size();
    g_log.information() << "DB1250 Generate background function of order = " << order << std::endl;

    bkgdfunc->setAttributeValue("n", int(order));
    bkgdfunc->initialize();

    for (size_t i = 0; i < order; ++i)
    {
        std::stringstream ss;
        ss << "A" << i;
        std::string parname = ss.str();

        bkgdfunc->setParameter(parname, bkgdparamws[i]);
    }

    g_log.debug() << "DBx423: Create background function: " << bkgdfunc->asString() << std::endl;

    return bkgdfunc;
}

/** Parse table workspace (from Fit()) containing background parameters to a vector
 */
void LeBailFit::parseBackgroundTableWorkspace(DataObjects::TableWorkspace_sptr bkgdparamws, std::vector<double>& bkgdorderparams)
{
  g_log.debug() << "DB1105A Parsing background TableWorkspace." << std::endl;

  // 1. Clear (output) map
  bkgdorderparams.clear();
  std::map<std::string, double> parmap;

  // 2. Check
  std::vector<std::string> colnames = bkgdparamws->getColumnNames();
  if (colnames.size() < 2)
  {
    g_log.error() << "Input parameter table workspace must have more than 1 columns" << std::endl;
    throw std::invalid_argument("Invalid input background table workspace. ");
  }
  else
  {
    if (!(boost::starts_with(colnames[0], "Name") && boost::starts_with(colnames[1], "Value")))
    {
      // Column 0 and 1 must be Name and Value (at least started with)
      g_log.error() << "Input parameter table workspace have wrong column definition." << std::endl;
      for (size_t i = 0; i < 2; ++i)
        g_log.error() << "Column " << i << " Should Be Name.  But Input is " << colnames[0] << std::endl;
      throw std::invalid_argument("Invalid input background table workspace. ");
    }
  }

  g_log.debug() << "DB1105B Background TableWorkspace is valid. " << std::endl;

  // 3. Input
  for (size_t ir = 0; ir < bkgdparamws->rowCount(); ++ir)
  {
    API::TableRow row = bkgdparamws->getRow(ir);
    std::string parname;
    double parvalue;
    row >> parname >> parvalue;

    if (parname.size() > 0 && parname[0] == 'A')
    {
      // Insert parameter name starting with A
      parmap.insert(std::make_pair(parname, parvalue));
    }
  }

  // 4. Sort: increasing order
  bkgdorderparams.reserve(parmap.size());
  for (size_t i = 0; i < parmap.size(); ++i)
  {
    bkgdorderparams.push_back(0.0);
  }

  std::map<std::string, double>::iterator mit;
  for (mit = parmap.begin(); mit != parmap.end(); ++mit)
  {
    std::string parname = mit->first;
    double parvalue = mit->second;
    std::vector<std::string> terms;
    boost::split(terms, parname, boost::is_any_of("A"));
    int tmporder = atoi(terms[1].c_str());
    bkgdorderparams[tmporder] = parvalue;
  }

  // 5. Debug output
  std::stringstream msg;
  msg << "Background Order = " << bkgdorderparams.size() << ": ";
  for (size_t iod = 0; iod < bkgdorderparams.size(); ++iod)
  {
    msg << "A" << iod << " = " << bkgdorderparams[iod] << "; ";
  }
  g_log.information() << "DB1105 Importing background TableWorkspace is finished. " << msg.str() << std::endl;

  return;
}

/** Parse the input TableWorkspace to some maps for easy access
 */
void LeBailFit::importParametersTable()
{
  // 1. Check column orders
  if (parameterWS->columnCount() < 3)
  {
    g_log.error() << "Input parameter table workspace does not have enough number of columns. "
                  << " Number of columns (Input =" << parameterWS->columnCount() << ") >= 3 as required. " << std::endl;
    throw std::invalid_argument("Input parameter workspace is wrong. ");
  }

  // 2. Import data to maps
  size_t numrows = parameterWS->rowCount();
  std::vector<std::string> colnames = parameterWS->getColumnNames();
  size_t numcols = colnames.size();

  std::map<std::string, double> tempdblmap;
  std::map<std::string, std::string> tempstrmap;
  std::map<std::string, double>::iterator dbliter;
  std::map<string, string>::iterator striter;

  std::string colname;
  double dblvalue;
  std::string strvalue;

  for (size_t ir = 0; ir < numrows; ++ir)
  {
    // a) Clear the map
    tempdblmap.clear();
    tempstrmap.clear();

    // b) Get the row
    API::TableRow trow = parameterWS->getRow(ir);

    // c) Parse each term
    for (size_t icol = 0; icol < numcols; ++icol)
    {
      colname = colnames[icol];
      if (colname.compare("FitOrTie") != 0 && colname.compare("Name") != 0)
      {
        // double data
        trow >> dblvalue;
        tempdblmap.insert(std::make_pair(colname, dblvalue));
      }
      else
      {
        // string data
        trow >> strvalue;
        tempstrmap.insert(std::make_pair(colname, strvalue));
      }
    }

    // d) Construct a Parameter instance
    Parameter newparameter;
    // i.   name
    striter = tempstrmap.find("Name");
    if (striter != tempstrmap.end())
    {
      newparameter.name = striter->second;
    }
    else
    {
      std::stringstream errmsg;
      errmsg << "Parameter (table) workspace " << parameterWS->name()
             << " does not contain column 'Name'.  It is not a valid input.  Quit ";
      g_log.error() << errmsg.str() << std::endl;
      throw std::invalid_argument(errmsg.str());
    }

    // ii.  fit
    striter = tempstrmap.find("FitOrTie");
    if (striter != tempstrmap.end())
    {
      std::string fitortie = striter->second;
      bool tofit = true;
      if (fitortie.length() > 0)
      {
        char fc = fitortie.c_str()[0];
        if (fc == 't' || fc == 'T')
        {
          tofit = false;
        }
      }
      newparameter.fit = tofit;
    }
    else
    {
      std::stringstream errmsg;
      errmsg << "Parameter (table) workspace " << parameterWS->name()
             << " does not contain column 'FitOrTie'.  It is not a valid input.  Quit ";
      g_log.error() << errmsg.str() << std::endl;
      throw std::invalid_argument(errmsg.str());
    }

    // iii. value
    dbliter = tempdblmap.find("Value");
    if (dbliter != tempdblmap.end())
    {
      newparameter.value = dbliter->second;
    }
    else
    {
      std::stringstream errmsg;
      errmsg << "Parameter (table) workspace " << parameterWS->name()
             << " does not contain column 'Value'.  It is not a valid input.  Quit ";
      g_log.error() << errmsg.str() << std::endl;
      throw std::invalid_argument(errmsg.str());
    }

    // iv.  min
    dbliter = tempdblmap.find("Min");
    if (dbliter != tempdblmap.end())
    {
      newparameter.minvalue = dbliter->second;
    }
    else
    {
      newparameter.minvalue = -1.0E10;
    }

    // v.   max
    dbliter = tempdblmap.find("Max");
    if (dbliter != tempdblmap.end())
    {
      newparameter.maxvalue = dbliter->second;
    }
    else
    {
      newparameter.maxvalue = 1.0E10;
    }

    // vi.  stepsize
    dbliter = tempdblmap.find("StepSize");
    if (dbliter != tempdblmap.end())
    {
      newparameter.stepsize = dbliter->second;
    }
    else
    {
      newparameter.stepsize = 1.0;
    }

    // vii. error
    newparameter.error = 1.0E10;

    mFuncParameters.insert(std::make_pair(newparameter.name, newparameter));
    mOrigFuncParameters.insert(std::make_pair(newparameter.name, newparameter.value));

    if (newparameter.fit)
    {
      g_log.information() << "[Input]: " << newparameter.name << ": value = " << newparameter.value
                          << " Range: [" << newparameter.minvalue << ", " << newparameter.maxvalue
                          << "], MC Step = " << newparameter.stepsize << ", Fit? = "
                          << newparameter.fit << std::endl;
    }
  }

  g_log.information() << "DB1118: Successfully Imported Peak Parameters TableWorkspace "
                      << parameterWS->name() << std::endl;

  return;
}

/** Parse the reflections workspace to a list of reflections;
  * Output --> mPeakHKLs
  * It will NOT screen the peaks whether they are in the data range.
 */
void LeBailFit::importReflections()
{
  g_log.debug() << "DB1119:  Importing HKL TableWorkspace" << std::endl;

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

  bool hasPeakHeight = false;
  if (colnames.size() >= 4 && colnames[3].compare("PeakHeight") == 0)
    {
        // Has a column for peak height
        hasPeakHeight = true;
    }

  /* FIXME This section is disabled presently
    bool userexcludepeaks = false;
    if (colnames.size() >= 5 && colnames[4].compare("Include/Exclude") == 0)
    {
        userexcludepeaks = true;
    }
    */

  // 2. Import data to maps
  int h, k, l;

  size_t numrows = reflectionWS->rowCount();
  for (size_t ir = 0; ir < numrows; ++ir)
  {
    // 1. Get from table row
    API::TableRow trow = reflectionWS->getRow(ir);
    trow >> h >> k >> l;

    // 2. Check whether this (hkl)^2 exits.  Throw exception if it exist
    //    Leave this part late to generate peaks

    // 3. Insert related data structure
    std::vector<int> hkl;
    hkl.push_back(h);
    hkl.push_back(k);
    hkl.push_back(l);
    mPeakHKLs.push_back(hkl);

    // optional peak height
    double peakheight = 1.0;
    if (hasPeakHeight)
        {
            trow >> peakheight;
        }

    // FIXME: Need to add the option to store user's selection to include/exclude peak
    int hkl2 = h*h + k*k + l*l;

    mPeakHeights.insert(std::make_pair(hkl2, peakheight));
  } // ENDFOR row

  g_log.debug() << "DB1119:  Finished importing HKL TableWorkspace.   Size of Rows = "
                << numrows << std::endl;

  return;
}

/** Write data (domain, values) to one specified spectrum of output workspace
 */
void LeBailFit::writeToOutputWorkspace(API::FunctionDomain1DVector domain,  API::FunctionValues values)
{
  if (outputWS->getNumberHistograms() <= mWSIndexToWrite)
  {
    g_log.error() << "LeBailFit.writeToOutputWorkspace.  Try to write to spectrum " << mWSIndexToWrite << " out of range = "
                  << outputWS->getNumberHistograms() << std::endl;
    throw std::invalid_argument("Try to write to a spectrum out of range.");
  }

  for (size_t i = 0; i < domain.size(); ++i)
  {
    outputWS->dataX(mWSIndexToWrite)[i] = domain[i];
  }
  for (size_t i = 0; i < values.size(); ++i)
  {
    outputWS->dataY(mWSIndexToWrite)[i] = values[i];
    if (fabs(values[i]) > 1.0)
      outputWS->dataE(mWSIndexToWrite)[i] = std::sqrt(fabs(values[i]));
    else
      outputWS->dataE(mWSIndexToWrite)[i] = 1.0;
  }

  return;
}

/*
 * Crop workspace if user required
 */
API::MatrixWorkspace_sptr LeBailFit::cropWorkspace(API::MatrixWorkspace_sptr inpws, size_t wsindex)
{
    // 1. Read inputs
    std::vector<double> fitrange = this->getProperty("FitRegion");

    double tof_min, tof_max;
    if (fitrange.size() == 0)
    {
        tof_min = inpws->readX(wsindex)[0];
        tof_max = inpws->readX(wsindex).back();
    }
    else if (fitrange.size() == 2)
    {
        tof_min = fitrange[0];
        tof_max = fitrange[1];
    }
    else
    {
        g_log.warning() << "Input FitRegion has more than 2 entries.  Using default in stread." << std::endl;

        tof_min = inpws->readX(wsindex)[0];
        tof_max = inpws->readX(wsindex).back();
    }

    // 2.Call  CropWorkspace()
    API::IAlgorithm_sptr cropalg = this->createSubAlgorithm("CropWorkspace", -1, -1, true);
    cropalg->initialize();

    cropalg->setProperty("InputWorkspace", inpws);
    cropalg->setPropertyValue("OutputWorkspace", "MyData");
    cropalg->setProperty("XMin", tof_min);
    cropalg->setProperty("XMax", tof_max);

    bool cropstatus = cropalg->execute();
    if (!cropstatus)
    {
      std::stringstream errmsg;
      errmsg << "DBx309 Cropping workspace unsuccessful.  Fatal Error. Quit!";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }

    API::MatrixWorkspace_sptr cropws = cropalg->getProperty("OutputWorkspace");
    if (!cropws)
    {
        g_log.error() << "Unable to retrieve a Workspace2D object from subalgorithm Crop." << std::endl;
    }
    else
    {
      g_log.debug() << "DBx307: Cropped Workspace... Range From " << cropws->readX(wsindex)[0] << " To "
                    << cropws->readX(wsindex).back() << std::endl;
    }

    return cropws;
}

/*
 * Write orignal data and difference b/w data and model to output's workspace
 * index 0 and 2
 */
void LeBailFit::writeInputDataNDiff(size_t workspaceindex, API::FunctionDomain1DVector domain)
{
    // 1. X-axis
    for (size_t i = 0; i < domain.size(); ++i)
    {
        outputWS->dataX(0)[i] = domain[i];
        outputWS->dataX(2)[i] = domain[i];
    }

    // 2. Add data and difference to output workspace (spectrum 1)
    for (size_t i = 0; i < dataWS->readY(workspaceindex).size(); ++i)
    {
        double modelvalue = outputWS->readY(1)[i];
        double inputvalue = dataWS->readY(workspaceindex)[i];
        double diff = modelvalue - inputvalue;
        outputWS->dataY(0)[i] = inputvalue;
        outputWS->dataY(2)[i] = diff;
    }

    return;
}

/*
 * Create a new table workspace for parameter values and set to output
 * to replace the input peaks' parameter workspace
 * Old: std::pair<double, char>
 */
void LeBailFit::exportParametersWorkspace(std::map<std::string, Parameter> parammap)
{
    DataObjects::TableWorkspace *tablews;

    tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr parameterws(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");
    tablews->addColumn("double", "chi^2");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");
    tablews->addColumn("double", "Diff");

    // 3. Add value
    std::map<std::string, Parameter>::iterator paramiter;
    std::map<std::string, double >::iterator opiter;
    for (paramiter = parammap.begin(); paramiter != parammap.end(); ++paramiter)
    {
        std::string parname = paramiter->first;
        if (parname.compare("Height"))
        {
          // If not Height
          // a. current value
          double parvalue = paramiter->second.value;

          // b. fit or tie?
          char fitortie = 't';
          if (paramiter->second.fit)
          {
            fitortie = 'f';
          }
          std::stringstream ss;
          ss << fitortie;
          std::string fit_tie = ss.str();

          // c. original value
          opiter = mOrigFuncParameters.find(parname);
          double origparvalue = -1.0E100;
          if (opiter != mOrigFuncParameters.end())
          {
            origparvalue = opiter->second;
          }

          // d. chi^2
          double chi2 = 0.0;
          opiter = mFuncParameterErrors.find(parname);
          if (opiter != mFuncParameterErrors.end())
          {
            chi2 = opiter->second;
          }

          // e. create the row
          double diff = origparvalue - parvalue;
          double min = paramiter->second.minvalue;
          double max = paramiter->second.maxvalue;
          double step = paramiter->second.stepsize;

          API::TableRow newparam = tablews->appendRow();
          newparam << parname << parvalue << fit_tie << chi2 << min << max << step << diff;
        } // ENDIF
    }

    // 3b. Chi^2
    API::TableRow fitchi2row = tablews->appendRow();
    fitchi2row << "FitChi2" << mLeBaiLFitChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    API::TableRow chi2row = tablews->appendRow();
    chi2row << "Chi2" << mLeBailCalChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;

    // 4. Add to output peroperty
    this->setProperty("OutputParameterWorkspace", parameterws);

    return;
}

/// ===================================   Auxiliary Functions   ==================================== ///

/*
 * Parse fx.abc to x and abc where x is the index of function and abc is the parameter name
 */
void LeBailFit::parseCompFunctionParameterName(std::string fullparname, std::string& parname, size_t& funcindex)
{
    std::vector<std::string> terms;
    boost::split(terms, fullparname, boost::is_any_of("."));

    if (terms.size() != 2)
    {
        g_log.error() << "Parameter name : " << fullparname << " does not have 1 and only 1 (.).  Cannot support!" << std::endl;
        throw std::runtime_error("Unable to support parameter name to split.");
    }

    // 1. Term 0, Index
    if (terms[0][0] != 'f')
    {
        g_log.error() << "Function name is not started from 'f'.  But " << terms[0] << ".  It is not supported!" << std::endl;
        throw std::runtime_error("Unsupported CompositeFunction parameter name.");
    }
    std::vector<std::string> t2s;
    boost::split(t2s, terms[0], boost::is_any_of("f"));
    std::stringstream ss(t2s[1]);
    ss >> funcindex;

    // 2. Term 1, Name
    parname = terms[1];

    // g_log.debug() << "DBx518 Split Parameter " << fullparname << " To Function Index " << funcindex << "  Name = " << parname << std::endl;

    return;
}


/** Do statistics to result (fitted or calcualted)
  */
void LeBailFit::doResultStatistics()
{
  const MantidVec& oY = outputWS->readY(0);
  const MantidVec& eY = outputWS->readY(1);
  const MantidVec& oE = outputWS->readE(0);

  double chi2 = 0.0;
  size_t numpts = oY.size();
  for (size_t i = 0; i < numpts; ++i)
  {
    double temp = (oY[i]-eY[i])*(oY[i]-eY[i])/(oE[i]*oE[i]);
    chi2 += temp;
  }

  chi2 = chi2/static_cast<double>(numpts);

  Parameter localchi2;
  localchi2.name = "LocalChi2";
  localchi2.value = chi2;

  mFuncParameters.insert(std::make_pair("LocalChi2", localchi2));

  g_log.information() << "[VZ] LeBailFit Result:  chi^2 = " << chi2 << std::endl;

  return;
}

/** Create output data workspace
  */
void LeBailFit::createOutputDataWorkspace(size_t workspaceindex, FunctionMode functionmode)
{
  // 1. Determine number of output spectra
  size_t nspec;
  bool plotindpeak = this->getProperty("PlotIndividualPeaks");

  switch (functionmode)
  {
    case FIT:
      // Lebail Fit mode
      // (0) final calculated result (1) original data (2) difference
      // (3) fitted pattern w/o background
      // (4) background (being fitted after peak)
      // (5) calculation based on input only (no fit)
      // (6) background (input)
      nspec = 7;

      break;

    case CALCULATION:
      // Calcualtion mode
      // (0) Data
      // (1) Calculation (LeBail)
      // (2) Difference
      // (3) Calculation w/o background
      // (4) Background
      // (5+) One spectrum for each peak
      nspec = 5;
      if (plotindpeak)
        nspec += mPeaks.size();

      break;

    case BACKGROUNDPROCESS:
      // Background calculation mode
      nspec = 3;

      break;

    default:
      // Error default
      std::stringstream errmsg;
      errmsg << "Function mode " << functionmode << " is not supported in createOutputWorkspace.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());

      break;
  }

  // 2. Create workspace2D and set the data to spectrum 0 (common among all)
  size_t nbin = dataWS->dataX(workspaceindex).size();
  outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
        API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
  for (size_t i = 0; i < nbin; ++i)
  {
    outputWS->dataX(0)[i] = dataWS->readX(workspaceindex)[i];
    outputWS->dataY(0)[i] = dataWS->readY(workspaceindex)[i];
    outputWS->dataE(0)[i] = dataWS->readE(workspaceindex)[i];
  }

  // 3. Set axis
  outputWS->getAxis(0)->setUnit("TOF");

  API::TextAxis* tAxis = 0;

  switch (functionmode)
  {
    case FIT:
      // Fit mode
      tAxis = new API::TextAxis(7);
      tAxis->setLabel(0, "Data");
      tAxis->setLabel(1, "Calc");
      tAxis->setLabel(2, "Diff");
      tAxis->setLabel(3, "CalcNoBkgd");
      tAxis->setLabel(4, "OutBkgd");
      tAxis->setLabel(5, "InpCalc");
      tAxis->setLabel(6, "InBkgd");

      break;

    case CALCULATION:
      // Calculation (Le Bail) mode
      tAxis = new API::TextAxis(nspec);
      tAxis->setLabel(0, "Data");
      tAxis->setLabel(1, "Calc");
      tAxis->setLabel(2, "Diff");
      tAxis->setLabel(3, "CalcNoBkgd");
      tAxis->setLabel(4, "Bkgd");
      for (size_t i = 0; i < (nspec-5); ++i)
      {
        std::stringstream ss;
        ss << "Peak_" << i;
        tAxis->setLabel(5+i, ss.str());
      }

      break;

    case BACKGROUNDPROCESS:
      // Background mode
      tAxis = new API::TextAxis(3);
      tAxis->setLabel(0, "Data");
      tAxis->setLabel(1, "Background");
      tAxis->setLabel(2, "DataNoBackground");

      break;

    default:
      // Error default
      std::stringstream errmsg;
      errmsg << "Function mode " << functionmode << " is not supported in createOutputWorkspace.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());

      break;
  }

  outputWS->replaceAxis(1, tAxis);

  return;
}

} // namespace CurveFitting
} // namespace Mantid
