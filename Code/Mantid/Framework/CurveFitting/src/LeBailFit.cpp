#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Chebyshev.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iomanip>

#include <fstream>

#define FITTEDPUREPEAKINDEX   3
#define PUREPEAKINDEX         7   // Output workspace pure peak (data with background removed)
#define FITTEDBACKGROUNDINDEX 4   // Output workspace background at ws index 4
#define INPUTBACKGROUNDINDEX  6   // Input background
#define SMOOTHEDBACKGROUND    8   // Smoothed background
const double NEG_DBL_MAX(-1.*DBL_MAX);

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

#define PEAKRANGECONSTANT 5.0

  bool compDescending2(int a, int b)
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

    // Input Data Workspace
    this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
                          "Input workspace containing the data to fit by LeBail algorithm.");

    // Output Result Data/Model Workspace
    this->declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "", Direction::Output),
                          "Output workspace containing calculated pattern or calculated background. ");

    // Instrument profile Parameters
    this->declareProperty(new WorkspaceProperty<TableWorkspace>("InputParameterWorkspace", "", Direction::Input),
                          "Input table workspace containing the parameters required by LeBail fit. ");

    // Output instrument profile parameters
    auto tablewsprop1 =  new WorkspaceProperty<TableWorkspace>("OutputParameterWorkspace", "", Direction::Output,
                                                               API::PropertyMode::Optional);
    this->declareProperty(tablewsprop1, "Input table workspace containing the parameters required by LeBail fit. ");

    // Single peak: Reflection (HKL) Workspace, PeaksWorkspace
    this->declareProperty(new WorkspaceProperty<TableWorkspace>("InputHKLWorkspace", "", Direction::InOut),
                          "Input table workspace containing the list of reflections (HKL). ");

    // Bragg peaks profile parameter output table workspace
    auto tablewsprop2 = new WorkspaceProperty<TableWorkspace>("OutputPeaksWorkspace", "", Direction::Output,
                                                              API::PropertyMode::Optional);
    this->declareProperty(tablewsprop2, "Optional output table workspace containing all peaks' peak parameters. ");

    // WorkspaceIndex
    this->declareProperty("WorkspaceIndex", 0, "Workspace index of the spectrum to fit by LeBail.");

    // Interested region
    this->declareProperty(new Kernel::ArrayProperty<double>("FitRegion"),
                          "Region of data (TOF) for LeBail fit.  Default is whole range. ");

    // Functionality: Fit/Calculation/Background
    std::vector<std::string> functions;
    functions.push_back("LeBailFit");
    functions.push_back("Calculation");
    functions.push_back("MonteCarlo");
    //  TODO: Turn on this option in release 2.4
    //  functions.push_back("CalculateBackground");
    auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
    this->declareProperty("Function", "LeBailFit", validator, "Functionality");

    //------------------  Background Related Properties  -------------------------
    // About background:  Background type, input (table workspace or array)
    std::vector<std::string> bkgdtype;
    bkgdtype.push_back("Polynomial");
    bkgdtype.push_back("Chebyshev");
    auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
    this->declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Background type");

    // Background function order
    // this->declareProperty("BackgroundFunctionOrder", 12, "Order of background function.");

    // Input background parameters (array)
    this->declareProperty(new Kernel::ArrayProperty<double>("BackgroundParameters"),
                          "Optional: enter a comma-separated list of background order parameters from order 0. ");

    // Input background parameters (tableworkspace)
    auto tablewsprop3 = new WorkspaceProperty<TableWorkspace>("BackgroundParametersWorkspace", "", Direction::InOut,
                                                              API::PropertyMode::Optional);
    this->declareProperty(tablewsprop3, "Optional table workspace containing the fit result for background.");

    // UseInputPeakHeights
    this->declareProperty("UseInputPeakHeights", true,
                          "For 'Calculation' mode only, use peak heights specified in ReflectionWorkspace. "
                          "Otherwise, calcualte peaks' heights. ");

    // Peak Radius
    this->declareProperty("PeakRadius", 5, "Range (multiplier relative to FWHM) for a full peak. ");

    // Pattern calcualtion
    this->declareProperty("PlotIndividualPeaks", false, "Option to output each individual peak in mode Calculation.");

    // Minimizer
    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys(); // :Instance().getKeys();
    declareProperty("Minimizer","Levenberg-MarquardtMD",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(minimizerOptions)),
                    "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Kernel::Direction::InOut);

    declareProperty("Damping", 1.0, "Damping factor if minizer is 'Damping'");

    declareProperty("NumberMinimizeSteps", 100, "Number of Monte Carlo random walk steps.");

    declareProperty("FitGeometryParameter", false, "Option to choose to fit geometry-related parameters.");

    declareProperty("RandomSeed", 1, "Randum number seed.");

    declareProperty("AnnealingTemperature", 1.0, "Temperature used Monte Carlo.  "
                    "Negative temperature is for simulated annealing. ");

    declareProperty("UseAnnealing", true, "Allow annealing temperature adjusted automatically.");

    declareProperty("DrunkenWalk", false, "Flag to use drunken walk algorithm. "
                    "Otherwise, random walk algorithm is used. ");

    declareProperty("MinimumPeakHeight", 0.01, "Minimum height of a peak to be counted "
                    "during smoothing background by exponential smooth algorithm. ");

    // Flag to allow input hkl file containing degenerated peaks
    declareProperty("AllowDegeneratedPeaks", false,
                    "Flag to allow degenerated peaks in input .hkl file. "
                    "Otherwise, an exception will be thrown if this situation occurs.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
  */
  void LeBailFit::exec()
  {
    // 1. Process input properties
    processInputProperties();

    // 2. Import parameters from table workspace
    this->parseInstrumentParametersTable();
    this->parseBraggPeaksParametersTable();

    // 3. Create LeBail Function & initialize from input
    // a. All individual peaks
    bool inputparamcorrect = generatePeaksFromInput();

    // b. Background information
    std::string backgroundtype = this->getProperty("BackgroundType");
    std::vector<double> bkgdorderparams = this->getProperty("BackgroundParameters");
    DataObjects::TableWorkspace_sptr bkgdparamws = this->getProperty("BackgroundParametersWorkspace");

    // c. Create CompositeFunction
    createLeBailFunction(backgroundtype, bkgdorderparams, bkgdparamws);

    g_log.debug() << "LeBail Composite Function: " << m_lebailFunction->asString() << "\n";

    // d. Function mode
    if (inputparamcorrect)
    {
      // All peaks within range are physical and good to refine
      m_inputParameterPhysical = true;
    }
    else
    {
      // Some peaks within range have unphysical parameters.  Just calcualtion for reference
      m_inputParameterPhysical = false;
      g_log.warning() << "Input instrument parameters generate some peaks with unphysical profile "
                         "parameters.\n";
      if (m_fitMode == FIT)
      {
        g_log.warning() << "Function mode FIT is disabled.  Convert to Calculation mode.\n";
        m_fitMode = CALCULATION;
      }
    }

    // 5. Create output workspace
    this->createOutputDataWorkspace();

    // 6. Real work
    m_lebailFitChi2 = -1; // Initialize
    m_lebailCalChi2 = -1;

    // a) Calculate background
    FunctionDomain1DVector domainBkgd(m_dataWS->readX(m_wsIndex));
    FunctionValues valuesBkgd(domainBkgd);
    m_backgroundFunction->function(domainBkgd, valuesBkgd);
    m_backgroundVec.resize(domainBkgd.size());
    for (size_t i = 0; i < domainBkgd.size(); ++i)
      m_backgroundVec[i] = valuesBkgd[i];

    // b) Calculate/fit peaks
    switch (m_fitMode)
    {
      case FIT:
        // LeBail Fit
        g_log.notice() << "Function: Do LeBail Fit.\n";
        if (inputparamcorrect)
        {
          execLeBailFit();
        }
        else
        {
          writeFakedDataToOutputWS(m_wsIndex, m_fitMode);
        }
        break;

      case CALCULATION:
        // Calculation
        g_log.notice() << "Function: Pattern Calculation.\n";
        execPatternCalculation();
        break;

      case BACKGROUNDPROCESS:
        // Calculating background
        // FIXME : Determine later whether this functionality is kept or removed!
        g_log.notice() << "Function: Calculate Background (Precisely).\n";
        calBackground(m_wsIndex);
        break;

      case MONTECARLO:
        // Monte carlo Le Bail refinement
        g_log.notice("Function: Do LeBail Fit By Monte Carlo Random Walk.");
        execRandomWalkMinimizer(m_numMinimizeSteps, m_funcParameters);
        doResultStatistics();
        g_log.notice() << "Final Rwp = " << m_bestRwp << "\n";

        break;

      default:
        // Impossible
        std::stringstream errmsg;
        errmsg << "FunctionMode = " << m_fitMode <<" is not supported in exec().";
        g_log.error() << errmsg.str() << "\n";
        throw std::runtime_error(errmsg.str());

        break;
    }

    // 7. Output peak (table) and parameter workspace
    exportBraggPeakParameterToTable();
    exportInstrumentParameterToTable(m_funcParameters);
    this->setProperty("OutputWorkspace", m_outputWS);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input properties to class variables and do some initial check
    */
  void LeBailFit::processInputProperties()
  {
    // 1. Get input and perform some check
    // a) Import data workspace and related, do crop
    API::MatrixWorkspace_sptr inpWS = this->getProperty("InputWorkspace");

    int tempindex = this->getProperty("WorkspaceIndex");
    m_wsIndex = size_t(tempindex);

    if (m_wsIndex >= inpWS->getNumberHistograms())
    {
      // throw if workspace index is not correct
      stringstream errss;
      errss << "Input WorkspaceIndex " << tempindex << " is out of boundary [0, "
            << inpWS->getNumberHistograms() << "). ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    m_dataWS = this->cropWorkspace(inpWS, m_wsIndex);

    // b) Minimizer
    std::string minim = getProperty("Minimizer");
    mMinimizer = minim;

    // c) Peak parameters and related.
    parameterWS = this->getProperty("InputParameterWorkspace");
    reflectionWS = this->getProperty("InputHKLWorkspace");
    mPeakRadius = this->getProperty("PeakRadius");

    // d) Determine Functionality (function mode)
    std::string function = this->getProperty("Function");
    m_fitMode = FIT; // Default: LeBailFit
    if (function.compare("Calculation") == 0)
    {
      // peak calculation
      m_fitMode = CALCULATION;
    }
    else if (function.compare("CalculateBackground") == 0)
    {
      // automatic background points selection
      m_fitMode = BACKGROUNDPROCESS;
    }
    else if (function.compare("MonteCarlo") == 0)
    {
      // Monte Carlo random walk refinement
      m_fitMode = MONTECARLO;
    }
    else if (function.compare("LeBailFit") == 0)
    {
      // Le Bail Fit mode
      m_fitMode = FIT;
    }
    else
    {
      stringstream errss;
      errss << "Function mode " << function << " is not supported by LeBailFit().";
      g_log.error(errss.str());
      throw invalid_argument(errss.str());
    }

    m_dampingFactor = getProperty("Damping");

    tempindex = getProperty("NumberMinimizeSteps");
    if (tempindex >= 0)
      m_numMinimizeSteps = static_cast<size_t>(tempindex);
    else
    {
      m_numMinimizeSteps = 0;
      stringstream errss;
      errss << "Input number of random walk steps (" << m_numMinimizeSteps <<
               ") cannot be less and equal to zero.";
      g_log.error(errss.str());
      throw invalid_argument(errss.str());
    }

    m_minimumHeight = getProperty("MinimumPeakHeight");

    // Tolerate duplicated input peak or not?
    m_tolerateInputDupHKL2Peaks = getProperty("AllowDegeneratedPeaks");

    return;
  }


  //===================================  Pattern Calculation & Minimizing  =======================

  //----------------------------------------------------------------------------------------------
  /** Calcualte LeBail diffraction pattern:
   *  Output spectra:
   *  0: data;  1: calculated pattern; 3: difference
   *  4: input pattern w/o background
   *  5~5+(N-1): optional individual peak
   */
  void LeBailFit::execPatternCalculation()
  {
    // 1. Generate domain and value
    const vector<double> vecX = m_dataWS->readX(m_wsIndex);
    const vector<double> vecY = m_dataWS->readY(m_wsIndex);

    API::FunctionDomain1DVector domain(vecX);
    API::FunctionValues values(domain);

    // 2. Calculate diffraction pattern
    bool useinputpeakheights = this->getProperty("UseInputPeakHeights");
    calculateDiffractionPattern(m_dataWS, m_wsIndex, domain, values, m_funcParameters, !useinputpeakheights);

    // 3. Add data (0: experimental, 1: calcualted, 2: difference)
    m_outputWS->dataY(0) = vecY;
    for (size_t i = 0; i < values.size(); ++i)
    {
      m_outputWS->dataY(1)[i] = values[i];
      m_outputWS->dataY(2)[i] = vecY[i]-values[i];
    }

    //   (3: peak without background, 4: input background)
    m_backgroundFunction->function(domain, values);
    for (size_t i = 0; i < values.size(); ++i)
    {
      m_outputWS->dataY(3)[i] = m_outputWS->readY(1)[i] - values[i];
      m_outputWS->dataY(4)[i] = values[i];
    }

    // 4. Do peak calculation for all peaks, and append to output workspace
    bool ploteachpeak = this->getProperty("PlotIndividualPeaks");
    if (ploteachpeak)
    {
      size_t numpeaks = m_dspPeaks.size();
      for (size_t ipk = 0; ipk < numpeaks; ++ipk)
      {
        CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr peak = m_dspPeaks[ipk].second;

        peak->function(domain, values);
        for (size_t i = 0; i < domain.size(); ++i)
        {
          m_outputWS->dataX(ipk+5)[i] = domain[i];
        }
        for (size_t i = 0; i < values.size(); ++i)
        {
          m_outputWS->dataY(ipk+5)[i] = values[i];
        }
      } // FOR PEAKS
    } // ENDIF: plot.each.peak

    // 5. Calculate Rwp
    double rwp, rp;
    rp = -100.0;

    const MantidVec& peakvalues = m_outputWS->readY(FITTEDPUREPEAKINDEX);
    const MantidVec& background =  m_outputWS->readY(FITTEDBACKGROUNDINDEX);
    vector<double> caldata(values.size(), 0.0);
    std::transform(peakvalues.begin(), peakvalues.end(), background.begin(), caldata.begin(), std::plus<double>());
    rwp = getRFactor(m_dataWS->readY(m_wsIndex), caldata, m_dataWS->readE(m_wsIndex));
    // calculatePowderPatternStatistic(m_outputWS->dataY(FITTEDPUREPEAKINDEX), m_outputWS->dataY(FITTEDBACKGROUNDINDEX), rwp, rp);
    g_log.notice() << "Rwp = " << rwp << ", Rp = " << rp << "\n";

    Parameter par_rwp;
    par_rwp.name = "Rwp";
    par_rwp.value = rwp;

    m_funcParameters["Rwp"] = par_rwp;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** LeBail Fitting for one self-consistent iteration
   */
  void LeBailFit::execLeBailFit()
  {
    // FIXME: m_maxStep should be an instance variable and evaluate in exec as an input property
    int m_maxSteps = 1;

    // 1. Get a copy of input function parameters (map)
    std::map<std::string, Parameter> parammap;
    parammap = m_funcParameters;

    // 2. Do 1 iteration of LeBail fit
    for (int istep = 0; istep < m_maxSteps; ++istep)
    {
      this->do1StepLeBailFit(parammap);
    }

    // 3. Output
    m_funcParameters = parammap;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate Le Bail function values with calculating peak intensities
    * Arguments:
    * @param dataws :  workspace2D holding data
    * @param workspaceindex:  workspace index of the data in dataws
    * @param domain :  input data domain
    * @param values :  output function values
    * @param parammap: parameter maps (values)
    * @param recalpeakintensity: option to calculate peak intensity or use them stored in parammap
   */
  bool LeBailFit::calculateDiffractionPattern(MatrixWorkspace_sptr dataws, size_t workspaceindex,
                                               FunctionDomain1DVector domain, FunctionValues& values,
                                               map<string, Parameter > parammap, bool recalpeakintensity)
  {
    // 1. Set parameters to each peak
    bool allpeaksvalid = true;
    std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr>::iterator pit;
    //  for (pit = m_peaks.begin(); pit != m_peaks.end(); ++pit)
    size_t numpeaks = m_dspPeaks.size();
    for (size_t ipk = 0; ipk < numpeaks; ++ipk)
    {
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;

      setPeakParameters(thispeak, parammap, 1.0, false);
      double d_h, tof_h;
      string errmsg;
      bool localvalid = examinInstrumentParameterValid(thispeak, d_h, tof_h, errmsg);
      if (!localvalid)
      {
        allpeaksvalid = false;
        break;
      }
    }

    // 2. Calculate peak intensities
    if (recalpeakintensity)
    {
      // a) Calcualte peak intensity
      bool zerobackground = false;
      vector<double> peaksvalues(dataws->readY(workspaceindex).size());
      calculatePeaksIntensities(dataws, workspaceindex, zerobackground, peaksvalues);

      // b) Debug output
      std::stringstream msg;
      msg << "[DB1209 Pattern Calcuation]  Number of Peaks = " << m_dspPeaks.size() << "\n";
      for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
      {
        CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr peak = m_dspPeaks[ipk].second;
        int h, k, l;
        peak->getMillerIndex(h, k, l);
        msg << "(" << h << ", " << k << ", " << l << "), H = " << std::setw(7)
            << std::setprecision(5) << peak->height();
        if ((ipk+1) % 4 == 0)
          msg << "\n";
        else
          msg << ";  ";
      } // ENDFOR: Peak
      g_log.information() << msg.str() << "\n";
    }

    // 3. Calcualte model pattern
    m_lebailFunction->function(domain, values);

    return allpeaksvalid;
  }

  //----------------------------------------------------------------------------------------------
  /** Perform one itearation of LeBail fitting
  * Including
  * a) Calculate pattern for peak intensities
  * b) Set peak intensities
  *
  * @param parammap:  a map containing parameters by using parameter's name as key
  */
  bool LeBailFit::do1StepLeBailFit(map<string, Parameter>& parammap)
  {
    // 1. Generate domain and value
    const std::vector<double> vecX = m_dataWS->readX(m_wsIndex);
    API::FunctionDomain1DVector domain(vecX);
    API::FunctionValues values(domain);

    // 2. Calculate peak intensity and etc.
    vector<double> allpeaksvalues(vecX.size(), 0.0);
    calculatePeaksIntensities(m_dataWS, m_wsIndex, false, allpeaksvalues);
    // calculateDiffractionPattern(m_dataWS, workspaceindex, domain, values, parammap, calpeakintensity);

    writeToOutputWorkspace(5, domain, values);

    // b) Calculate input background
    m_backgroundFunction->function(domain, values);
    writeToOutputWorkspace(6, domain, values);

    // 3. Construct the tie.  2-level loop. (1) peak parameter (2) peak
    // TODO Release 2.4: setLeBailFitParameters(istep)
    this->setLeBailFitParameters();

    // 4. Construct the Fit
    this->fitLeBailFunction(parammap);

    // TODO (1) Calculate Rwp, Chi^2, .... for the fitted pattern.

    // 5. Do calculation again and set the output
    // FIXME Move this part our of UnitLeBailFit
    bool calpeakintensity = true;
    API::FunctionValues newvalues(domain);
    this->calculateDiffractionPattern(m_dataWS, m_wsIndex, domain, newvalues, parammap, calpeakintensity);

    // Add final calculated value to output workspace
    writeToOutputWorkspace(1, domain, newvalues);

    // Add original data and
    writeInputDataNDiff(m_wsIndex, domain);

    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the fit/tie/set-parameter for LeBail Fit (mode)
   * All parameters              : set the value
   * Parameters for free fit     : do nothing;
   * Parameters fixed            : set them to be fixed;
   * Parameters for fit with tie : tie all the related up
  */
  void LeBailFit::setLeBailFitParameters()
  {
    vector<string> peakparamnames = m_dspPeaks[0].second->getParameterNames();

    // 1. Set up all the peaks' parameters... tie to a constant value..
    //    or fit by tieing same parameters of among peaks
    std::map<std::string, Parameter>::iterator pariter;
    for (pariter = m_funcParameters.begin(); pariter != m_funcParameters.end(); ++pariter)
    {
      Parameter funcparam = pariter->second;

      g_log.debug() << "Step 1:  Set peak parameter value " << funcparam.name << "\n";

      std::string parname = pariter->first;
      // double parvalue = funcparam.value;

      // a) Check whether it is a parameter used in Peak
      std::vector<std::string>::iterator sit;
      sit = std::find(peakparamnames.begin(), peakparamnames.end(), parname);
      if (sit == peakparamnames.end())
      {
        // Not a peak profile parameter
        g_log.debug() << "Unable to tie parameter " << parname << " b/c it is not a parameter for peak.\n";
        continue;
      }

      if (!funcparam.fit)
      {
        // a) Fix the value to a constant number
        size_t numpeaks = m_dspPeaks.size();
        for (size_t ipk = 0; ipk < numpeaks; ++ipk)
        {
          // TODO: Make a map between peak parameter name and index. And use fix() to replace tie
          std::stringstream ss1, ss2;
          ss1 << "f" << ipk << "." << parname;
          ss2 << funcparam.value;
          std::string tiepart1 = ss1.str();
          std::string tievalue = ss2.str();
          m_lebailFunction->tie(tiepart1, tievalue);
          g_log.debug() << "Set up tie | " << tiepart1 << " <---> " << tievalue << " | \n";

          /*--  Code prepared to replace the existing block
          ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
          size_t iparam = findIndex(thispeak, funcparam.name);
          thispeak->fix(iparam);
          --*/
        } // For each peak
      }
      else
      {
        // b) Tie the values among all peaks, but will fit
        for (size_t ipk = 1; ipk < m_dspPeaks.size(); ++ipk)
        {
          std::stringstream ss1, ss2;
          ss1 << "f" << (ipk-1) << "." << parname;
          ss2 << "f" << ipk << "." << parname;
          std::string tiepart1 = ss1.str();
          std::string tiepart2 = ss2.str();
          m_lebailFunction->tie(tiepart1, tiepart2);
          g_log.debug() << "LeBailFit.  Fit(Tie) / " << tiepart1 << " / " << tiepart2 << " /\n";
        }

        // c) Set the constraint
        std::stringstream parss;
        parss << "f0." << parname;
        string parnamef0 = parss.str();
        CurveFitting::BoundaryConstraint* bc =
            new BoundaryConstraint(m_lebailFunction.get(), parnamef0, funcparam.minvalue, funcparam.maxvalue);
        m_lebailFunction->addConstraint(bc);
      }
    } // FOR-Function Parameters

    // 2. Set 'Height' to be fixed
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      // a. Get peak height
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
      thispeak->fix(0);
    } // For each peak

    // 3. Fix all background paramaters to constants/current values
    size_t funcindex = m_dspPeaks.size();
    std::vector<std::string> bkgdparnames = m_backgroundFunction->getParameterNames();
    for (size_t ib = 0; ib < bkgdparnames.size(); ++ib)
    {
      std::string parname = bkgdparnames[ib];
      double parvalue = m_backgroundFunction->getParameter(parname);
      std::stringstream ss1, ss2;
      ss1 << "f" << funcindex << "." << parname;
      ss2 << parvalue;
      std::string tiepart1 = ss1.str();
      std::string tievalue = ss2.str();

      g_log.debug() << "Step 2: LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /\n";

      m_lebailFunction->tie(tiepart1, tievalue);

      // TODO: Prefer to use fix other than tie().  Need to figure out the parameter index from name
      /*
        mLeBailFunction->fix(paramindex);
      */
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit LeBailFunction by calling Fit()
   *  NO NEED to set up the parameter value, fix the parameter or tie.
   *  Be called after all functions in LeBailFunction (composite) are set up (tie, constrain)
   *  Output: a parameter name-value map
   *
   * @param parammap  :  map containing Parameters to fit.  Key is Parameter's name
   */
  bool LeBailFit::fitLeBailFunction(std::map<std::string, Parameter> &parammap)
  {
    // 1. Prepare fitting boundary parameters.
    double tof_min = m_dataWS->dataX(m_wsIndex)[0];
    double tof_max = m_dataWS->dataX(m_wsIndex).back();
    std::vector<double> fitrange = this->getProperty("FitRegion");
    if (fitrange.size() == 2 && fitrange[0] < fitrange[1])
    {
      // Properly defined
      tof_min = fitrange[0];
      tof_max = fitrange[1];
    }

    // 2. Call Fit to fit LeBail function.
    m_lebailFunction->useNumericDerivatives( true );
    string fitstatus;
    int numiterations = static_cast<int>(m_numMinimizeSteps);
    minimizeFunction(m_dataWS, m_wsIndex, boost::shared_ptr<API::IFunction>(m_lebailFunction),
                     tof_min, tof_max, mMinimizer, m_dampingFactor, numiterations, fitstatus, m_lebailFitChi2, true);

    // 3. Get parameters
    IFunction_sptr fitout = boost::dynamic_pointer_cast<IFunction>(m_lebailFunction);
    std::vector<std::string> parnames = fitout->getParameterNames();

    std::stringstream rmsg;
    for (size_t ip = 0; ip < parnames.size(); ++ip)
    {
      // a) Get all that are needed
      std::string parname = parnames[ip];
      double curvalue = fitout->getParameter(ip);
      double error = fitout->getError(ip);

      // b) Split parameter string
      // FIXME These codes are duplicated as to method parseCompFunctionParameterName().  Refactor!
      std::vector<std::string> results;
      boost::split(results, parname, boost::is_any_of("."));

      if (results.size() != 2)
      {
        stringstream errss;
        errss << "Parameter name : " << parname << " does not have 1 and only 1 (.).  Cannot support!";
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }

      // c) Error out zero-value-error parameters.
      //    Only 1 parameter out of all tied ones will have non-zero error-report.
      if (error > 1.0E-8)
      {
        // Update the function parameters' error
        Parameter& thisparam = m_funcParameters[parname];
        thisparam.error = error;

        // Output
        std::string parnamex = results[1];
        if (parammap[parnamex].fit)
        {
          // Fit
          parammap[parnamex].value = curvalue;
          parammap[parnamex].fit = true;

          rmsg << std::setw(10) << parnamex << " = " << setw(7) << setprecision(5) << curvalue
               << ",    Error = " << setw(7) << setprecision(5) << error << "\n";
        }
        else
        {
          g_log.warning() << "[Fitting Result] Parameter " << parnamex << " is not set to refine.  "
                          << "But its chi^2 =" << error << "\n";
        }
      }
    }

    // 4. Calculate Chi^2 wih all parmeters fixed
    // a) Fit all parameters
    vector<string> lbparnames = m_lebailFunction->getParameterNames();
    for (size_t i = 0; i < lbparnames.size(); ++i)
    {
      m_lebailFunction->fix(i);
    }

    // b) Fit/calculation
    numiterations = 0; //
    string numfitstatus;
    minimizeFunction(m_dataWS, m_wsIndex, boost::shared_ptr<API::IFunction>(m_lebailFunction),
                     tof_min, tof_max, "Levenberg-MarquardtMD", 0.0, numiterations, numfitstatus, m_lebailFitChi2, false);

    g_log.notice() << "LeBailFit (LeBailFunction) Fit result:  Chi^2 (Fit) = " << m_lebailFitChi2
                   << ", Chi^2 (Cal) = " << m_lebailCalChi2
                   << ", Fit Status = " << fitstatus << " with max number of steps = " << m_numMinimizeSteps
                   << "\n" << rmsg.str();

    // TODO: Check the covariant matrix to see whether any NaN or Infty.  If so, return false with reason
    // TODO: (continue).  Code should fit again with Simplex and extends MaxIteration if not enough... ...

    return true;
  }


  //----------------------------------------------------------------------------------------------
  /** Minimize a give function
   *
   * Input arguments:
   * @param dataws   :   data workspace
   * @param wsindex  :   workspace index of the data in data workspace to fit
   * @param function :   function to fit
   * @param tofmin   :   minimum X value of data to fit
   * @param tofmax   :   maximum X value of data to fit
   * @param minimizer:   name of the minimizer to use for fitting
   * @param dampfactor:  damping factor if damping is selected as minimizer
   * @param numiteration: number of iterations for fit
   * @param outputcovarmatrix:  option to let Fit to output covariant matrix
   * Output
   * @param status : fit status
   * @param chi2   : chi square of the fit
   */
  bool LeBailFit::minimizeFunction(MatrixWorkspace_sptr dataws, size_t wsindex, IFunction_sptr function,
                                    double tofmin, double tofmax, string minimizer, double dampfactor, int numiteration,
                                    string& status, double& chi2, bool outputcovarmatrix)
  {
    std::string fitoutputwsrootname("TempMinimizerOutput");

    // 1. Initialize
    API::IAlgorithm_sptr fitalg = this->createChildAlgorithm("Fit", -1.0, -1.0, true);
    fitalg->initialize();

    g_log.debug() << "[DBx534 | Before Fit] Function To Fit: " << function->asString()
                  << "\n" << "Number of iteration = " << numiteration << "\n";

    // 2. Set property
    fitalg->setProperty("Function", function);
    fitalg->setProperty("InputWorkspace", dataws);
    fitalg->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    fitalg->setProperty("StartX", tofmin);
    fitalg->setProperty("EndX", tofmax);
    fitalg->setProperty("Minimizer", minimizer); // default is "Levenberg-MarquardtMD"
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", numiteration);
    fitalg->setProperty("CreateOutput", true);
    fitalg->setProperty("Output", fitoutputwsrootname);
    fitalg->setProperty("CalcErrors", true);
    if (minimizer.compare("Damping") == 0)
    {
      fitalg->setProperty("Damping", dampfactor);
    }

    // c) Execute
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      g_log.notice() << "[Error] Fitting to LeBail function failed.\n";
      return false;
    }
    else
    {
      g_log.debug() << "[DBx523] Fitting successful.\n";
    }

    // d) Process output of fit
    //    chi^2 and status
    chi2 = fitalg->getProperty("OutputChi2overDoF");
    string fitstatus = fitalg->getProperty("OutputStatus");
    status = fitstatus;

    // 4. Optional output covariant matrix
    if (outputcovarmatrix)
    {
      ITableWorkspace_sptr covarws = fitalg->getProperty("OutputNormalisedCovarianceMatrix");
      if (covarws)
      {
        declareProperty(
              new WorkspaceProperty<ITableWorkspace>("OutputNormalisedCovarianceMatrix", "", Direction::Output),
              "The name of the TableWorkspace in which to store the final covariance matrix" );
        setPropertyValue("OutputNormalisedCovarianceMatrix", "NormalisedCovarianceMatrix");
        setProperty("OutputNormalisedCovarianceMatrix", covarws);
      }
      else
      {
        g_log.warning() << "Expected covariance matrix cannot be found with algorithm Fit.\n";
      }
    }

    return true;
  }

  //---------------------------------------------------------------------------------------------
  /** Calculate background of the specified diffraction pattern
  * by
  * 1. fix the peak parameters but height;
  * 2. fit only heights of the peaks in a peak-group and background coefficients (assumed order 2 or 3 polynomial)
  * 3. remove peaks by the fitting result
  */
  void LeBailFit::calBackground(size_t workspaceindex)
  {
    throw std::runtime_error("This method is suspended.");
    UNUSED_ARG(workspaceindex);

    return;
  }

  //===================================  Set up the Le Bail Fit   ================================
  //----------------------------------------------------------------------------------------------
  /** Create LeBailFunction
    * @param backgroundtype:  string, type of background function
    * @param bkgdorderparams:  vector of background polynomials from order 0
    * @param bkgdparamws:     TableWorkspace containing background polynomials
   */
  void LeBailFit::createLeBailFunction(string backgroundtype, vector<double>& bkgdorderparams,
                                        TableWorkspace_sptr bkgdparamws)
  {
    // 1. Background parameters are from ...
    if (!bkgdparamws)
    {
      g_log.information() << "[Input] Use background specified with vector with input vector sized "
                          << bkgdorderparams.size() << ".\n";
    }
    else
    {
      g_log.information() << "[Input] Use background specified by table workspace.\n";
      parseBackgroundTableWorkspace(bkgdparamws, bkgdorderparams);
    }

    // 2. Create background function
    auto background = API::FunctionFactory::Instance().createFunction(backgroundtype);
    m_backgroundFunction = boost::dynamic_pointer_cast<BackgroundFunction>(background);

    size_t order = bkgdorderparams.size();

    m_backgroundFunction->setAttributeValue("n", int(order));
    m_backgroundFunction->initialize();

    for (size_t i = 0; i < order; ++i)
    {
      std::stringstream ss;
      ss << "A" << i;
      std::string parname = ss.str();
      m_backgroundFunction->setParameter(parname, bkgdorderparams[i]);
    }

    g_log.information() << "Generate background function.  Type = " << backgroundtype
                        << " Order = " << order << "\n";
    g_log.debug() << "DBx423: Create background function: " << m_backgroundFunction->asString() << "\n";

    // 3. Generate the composite function
    API::CompositeFunction compfunction;
    m_lebailFunction = boost::make_shared<API::CompositeFunction>(compfunction);
    m_lebailFunction->useNumericDerivatives(true);

    // 4. Add peaks to LeBail Function
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
      m_lebailFunction->addFunction(thispeak);
    }
    m_lebailFunction->addFunction(m_backgroundFunction);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Crop workspace if user required
    * @param inpws :  input workspace to crop
    * @param wsindex: workspace index of the data to fit against
   */
  API::MatrixWorkspace_sptr LeBailFit::cropWorkspace(API::MatrixWorkspace_sptr inpws, size_t wsindex)
  {
    // 1. Read inputs
    std::vector<double> fitrange = this->getProperty("FitRegion");

    double tof_min, tof_max;
    if (fitrange.empty())
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
      g_log.warning() << "Input FitRegion has more than 2 entries.  Using default in stread.\n";

      tof_min = inpws->readX(wsindex)[0];
      tof_max = inpws->readX(wsindex).back();
    }

    // 2.Call  CropWorkspace()
    API::IAlgorithm_sptr cropalg = this->createChildAlgorithm("CropWorkspace", -1, -1, true);
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
      g_log.error() << errmsg.str() << "\n";
      throw std::runtime_error(errmsg.str());
    }

    API::MatrixWorkspace_sptr cropws = cropalg->getProperty("OutputWorkspace");
    if (!cropws)
    {
      g_log.error() << "Unable to retrieve a Workspace2D object from ChildAlgorithm Crop.\n";
    }
    else
    {
      g_log.debug() << "DBx307: Cropped Workspace... Range From " << cropws->readX(wsindex)[0] << " To "
                    << cropws->readX(wsindex).back() << "\n";
    }

    return cropws;
  }


  //===================================  Operation with Bragg Peaks   ============================
  //----------------------------------------------------------------------------------------------
  /** Generate a list of peaks from input
   * Initial screening will be made to exclude peaks out of data range
   * The peak parameters will be set up to each peak
   * If the peak parameters are invalid:
   * (1) alpha < 0
   * (2) beta < 0
   * (3) sigma2 < 0
   * An error message will be spit out and there will be an early return
   *
   * RETURN:  True if there is no peak that has UNPHYSICAL profile parameters.
  */
  bool LeBailFit::generatePeaksFromInput()
  {
    // 1. Prepare
    size_t numpeaksoutofrange = 0;
    size_t numpeaksparamerror = 0;
    stringstream errss;

    double tofmin = m_dataWS->readX(m_wsIndex)[0];
    double tofmax =  m_dataWS->readX(m_wsIndex).back();

    // 2. Generate peaks
    //    There is no need to consider peak's order now due to map
    size_t numinput = m_inputPeakInfoVec.size();
    for (size_t ipk = 0; ipk < numinput; ++ipk)
    {
      // a) Get peak information
      vector<int>& hkl = m_inputPeakInfoVec[ipk].first;
      double peakheight = m_inputPeakInfoVec[ipk].second;

      int h = hkl[0];
      int k = hkl[1];
      int l = hkl[2];

      // b) Generate peak
      CurveFitting::ThermalNeutronBk2BkExpConvPVoigt tmppeak;
      tmppeak.setMillerIndex(h, k, l);
      tmppeak.initialize();
      CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr speak = boost::make_shared
          <CurveFitting::ThermalNeutronBk2BkExpConvPVoigt>(tmppeak);

      // c) Set peak function
      setPeakParameters(speak, this->m_funcParameters, peakheight, true);
      speak->setPeakRadius(mPeakRadius);

      // d) Calculate peak parameters
      double tof_h, d_h;
      string errmsg;
      bool validparam = examinInstrumentParameterValid(speak, d_h, tof_h, errmsg);

      // 6. Add peak to peak map???
      if (tof_h < tofmin || tof_h > tofmax)
      {
        //  Exclude peak out of range
        g_log.debug() << "Input peak (" << h << ", " << k << ", " << l << ") is out of range. "
                      << "TOF_h = " << tof_h << "\n";
        ++ numpeaksoutofrange;
      }
      else if (validparam)
      {
        // Valid peak parameters, then add peak to the list
        m_dspPeaks.push_back(make_pair(d_h, speak));
      }
      else
      {
        // Peak has UNPHYSICAL profile parameter value from model
        ++ numpeaksparamerror;
        errss << "[Warning] Peak (" << h << ", " << k << ", " << l << "), d_h = " << d_h
              << ", TOF_h = " << tof_h << ": "
              << errmsg << "\n";
      }
    } // ENDFOR All Input (HKL)

    sort(m_dspPeaks.begin(), m_dspPeaks.end());

    // 3. Check to see whether there is any duplicate peaks
    bool noduppeaks = true;
    vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> >::iterator peakiter, leftpeakiter;
    for (peakiter = m_dspPeaks.begin()+1; peakiter != m_dspPeaks.end(); ++peakiter)
    {
      leftpeakiter = peakiter-1;
      double this_dh = peakiter->first;
      double prev_dh = leftpeakiter->first;
      if ( this_dh - prev_dh < 1.0E-10 )
      {
        // Possibly 2 exactly same (HKL)^2
        // a) Signal
        noduppeaks = false;

        // b) Print out information
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = peakiter->second;
        ThermalNeutronBk2BkExpConvPVoigt_sptr leftpeak = leftpeakiter->second;
        int h0, k0, l0, h1, k1, l1;
        thispeak->getMillerIndex(h0, k0, l0);
        leftpeak->getMillerIndex(h1, k1, l1);
        int hkl2a = h0*h0 + k0*k0 + l0*l0;
        int hkl2b = h1*h1 + k1*k1 + l1*l1;
        int ipk = static_cast<int>(peakiter-m_dspPeaks.begin());
        std::stringstream errmsg;
        errmsg << "Duplicate (d_h) peaks found! They are peak (" << h0 << ", " << k0 << ", " << l0
               << ") ^2 = " << hkl2a << " indexed as " << ipk << ", and peak ("
               << h1 << ", " << k1 << ", " << l1 << ")^2 = " << hkl2b << " indexed as " << ipk-1;

        if (m_tolerateInputDupHKL2Peaks)
          g_log.warning(errmsg.str());
        else
          g_log.error(errmsg.str());
      }
    }

    // 4. Remove duplicated peaks or throw exception
    if (!noduppeaks)
    {
      if (m_tolerateInputDupHKL2Peaks)
      {
        // Allow duplicate input.  Remove peaks then.
        for (peakiter = m_dspPeaks.begin()+1; peakiter != m_dspPeaks.end(); ++peakiter)
        {
          leftpeakiter = peakiter-1;
          double this_dh = peakiter->first;
          double prev_dh = leftpeakiter->first;
          if ( this_dh - prev_dh < 1.0E-10 )
          {
            // Possibly 2 exactly same (HKL)^2
            // a) Get information
            ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = peakiter->second;
            ThermalNeutronBk2BkExpConvPVoigt_sptr leftpeak = leftpeakiter->second;
            int h0, k0, l0, h1, k1, l1;
            thispeak->getMillerIndex(h0, k0, l0);
            leftpeak->getMillerIndex(h1, k1, l1);
            int hkl2a = h0*h0 + k0*k0 + l0*l0;
            int hkl2b = h1*h1 + k1*k1 + l1*l1;

            // b) Some exception
            if (hkl2a != hkl2b)
            {
              stringstream errmsg;
              errmsg << "Peak (" << h0 << ", " << k0 << ", " << l0 << ") ^2 = " << hkl2a
                     << ", d = " << this_dh << " "
                     << "and peak (" << h1 << ", " << k1 << ", " << l1 << ")^2 = " << hkl2b
                     << ", d = " << prev_dh << " "
                     << "have too close d-spacing value, but they are different peaks. "
                     << "This situation is not supported yet.";
              g_log.error(errmsg.str());
              throw runtime_error(errmsg.str());
            }

            // c) Remove this peak
            peakiter = m_dspPeaks.erase(peakiter);
          }
        } // ENDFOR
      }
      else
      {
        // Disallow duplicated input peak (HKL)^2.
        throw runtime_error("Input contains peaks with duplicated (HKL)^2. User treats it an exception.");
      }
    }


    /*
    size_t numpeaks = m_dspPeaks.size();
    for (size_t ipk = 0; ipk < numpeaks-1; ++ipk)
    {
      double this_dh = m_dspPeaks[ipk].first;
      double next_dh = m_dspPeaks[ipk+1].first;
      if (next_dh - this_dh < 1.0E-10)
      {
        // 2 neighboring peaks have almost same d_h
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
        ThermalNeutronBk2BkExpConvPVoigt_sptr nextpeak = m_dspPeaks[ipk+1].second;
        int h0, k0, l0, h1, k1, l1;
        thispeak->getMillerIndex(h0, k0, l0);
        nextpeak->getMillerIndex(h1, k1, l1);
        std::stringstream errmsg;
        errmsg << "Duplicate (d_h) peaks found! They are peak (" << h0 << ", " << k0 << ", " << l0
               << ") indexed as " << ipk << ", and peak (" << h1 << ", " << k1 << ", " << l1
               << ") indexed as " << ipk + 1;

        g_log.error(errmsg.str());
        throw std::invalid_argument(errmsg.str());
      }
    }
    */

    // 4. Information output
    g_log.information() << "Number of ... Input Peaks = " << m_inputPeakInfoVec.size() << "; Peaks Generated: "
                        << m_dspPeaks.size() << "; Peaks With Error Parameters = " << numpeaksparamerror
                        << "; Peaks Outside Range = " <<  numpeaksoutofrange
                        << "; Range: " << setprecision(5) << tofmin << ", " << setprecision(5) << tofmax <<  "\n";

    if (numpeaksparamerror > 0)
    {
      g_log.information(errss.str());
    }

    // Return
    return (numpeaksparamerror == 0);
  }

  //----------------------------------------------------------------------------------------------
  /** Examine whether the insturment parameter set to a peak can cause a valid set of
  * peak profile of that peak
  * @param peak :  ThermalNuetronBk2BkExpConvPVoigt peak function
  * @param d_h  :  output, the d-spacing value of the peak centre
  * @param tof_h:  output, the TOF value of peak centre
  * @param errmsg: output, the error message if the peak parameters are not valid
  */
  bool LeBailFit::examinInstrumentParameterValid(ThermalNeutronBk2BkExpConvPVoigt_sptr peak, double& d_h, double& tof_h,
                                                  string& errmsg)
  {
    // 1. Calculate peak parameters
    double alpha, beta, sigma2;
    alpha = peak->getPeakParameter("Alpha");
    beta = peak->getPeakParameter("Beta");
    sigma2 = peak->getPeakParameter("Sigma2");
    d_h = peak->getPeakParameter("d_h");
    tof_h = peak->centre();

    // 2. Check peak parameters' validity
    stringstream localerrss; //
    bool valid = true;
    if (alpha != alpha || alpha <= 0)
    {
      localerrss << "Alpha = " << alpha << "; ";
      valid = false;
    }
    if (beta != beta || beta <= 0)
    {
      localerrss << "Beta = " << beta << "; ";
      valid = false;
    }
    if (sigma2 != sigma2 || sigma2 <= 0)
    {
      localerrss << "Sigma^2 = " << sigma2 << ";";
      valid = false;
    }

    errmsg = localerrss.str();

    return valid;
  }

  //----------------------------------------------------------------------------------------------
  /** From table/map to set parameters to an individual peak.
   * It mostly is called by function in calculation.
   * @param peak :  ThermalNeutronBk2BkExpConvPVoigt function to have parameters' value set
   * @param parammap:  map of Parameters to set to peak
   * @param peakheight: height of the peak
   * @param setpeakheight:  boolean as the option to set peak height or not.
   */
  void LeBailFit::setPeakParameters(ThermalNeutronBk2BkExpConvPVoigt_sptr peak, map<std::string, Parameter> parammap,
                                     double peakheight, bool setpeakheight)
  {
    // 1. Prepare, sort parameters by name
    std::map<std::string, Parameter>::iterator pit;
    vector<string> peakparamnames = peak->getParameterNames();

    // 2. Apply parameters values to peak function
    for (pit = parammap.begin(); pit != parammap.end(); ++pit)
    {
      // a) Check whether the parameter is a peak parameter
      std::string parname = pit->first;
      std::vector<std::string>::iterator ifind =
          std::find(peakparamnames.begin(), peakparamnames.end(), parname);

      // b) Set parameter value
      if (ifind == peakparamnames.end())
      {
        // If not a peak profile parameter, skip
        g_log.debug() << "Parameter " << parname
                      << " in input parameter table workspace is not for peak function.\n";
      }
      else
      {
        // Set value
        double value = pit->second.value;
        peak->setParameter(parname, value);
        g_log.debug() << "LeBailFit Set " << parname << "= " << value << "\n";
      }
    } // ENDFOR: parameter iterator

    // 3. Peak height
    if (setpeakheight)
      peak->setParameter(0, peakheight);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** From table/map to set parameters to all peaks.
    * @param peaks:   vector of shared pointers to peaks that have parameters' values to set
    * @param parammap: map of Parameters to set to peak
    * @param peakheight: a universal peak height to set to all peaks
    * @param setpeakheight: flag to set peak height to each peak or not.
   */
  void LeBailFit::setPeaksParameters(vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peaks,
                                      map<std::string, Parameter> parammap,
                                      double peakheight, bool setpeakheight)
  {
    // 1. Prepare, sort parameters by name
    std::map<std::string, Parameter>::iterator pit;
    size_t numpeaks = peaks.size();
    if (numpeaks == 0)
      throw runtime_error("Set parameters to empty peak list. ");

    // 2. Apply parameters values to peak function
    vector<string> peakparnames = peaks[0].second->getParameterNames();
    size_t numparnames = peakparnames.size();

    for (size_t i = 0; i < numparnames; ++i)
    {
      if (i == 0 && setpeakheight)
      {
        // a) If index is height and set to height
        for (size_t ipk = 0; ipk < numpeaks; ++ipk)
          peaks[ipk].second->setParameter(0, peakheight);

      }
      else if (i > 0)
      {
        // b) A non height parameter
        string parname = peakparnames[i];
        pit = parammap.find(parname);

        // Skip if not found
        if (pit == parammap.end())
        {
          // Not found
          g_log.debug() << "Peak parameter " << parname
                        << "cannot be found in parameter map.\n";
          continue;
        }

        // Set value to each peak
        double parvalue = pit->second.value;
        for (size_t ipk = 0; ipk < numpeaks; ++ipk)
          peaks[ipk].second->setParameter(i, parvalue);

      }
      else
      {
        // c) If index is height, but height is not be set
        ;
      }
    }

    return;
  }


  //============================ Implement Le Bail Formular: Calculate Peak Intensities ==========

  //----------------------------------------------------------------------------------------------
  /** Calculate peak heights from the model to the observed data
  * Algorithm will deal with
  * (1) Peaks are close enough to overlap with each other
  * The procedure will be
  * (a) Assign peaks into groups; each group contains either (1) one peak or (2) peaks overlapped
  * (b) Calculate peak intensities for every peak per group
  *
  * @param dataws :  data workspace holding diffraction data for peak calculation
  * @param workspaceindex:  workpace index of the data for peak calculation in dataws
  * @param zerobackground:  flag if the data is zero background
  * @param allpeaksvalues:  output vector storing peaks' values calculated
  *
  * Return: True if all peaks' height are physical.  False otherwise
  */
  bool LeBailFit::calculatePeaksIntensities(MatrixWorkspace_sptr dataws, size_t workspaceindex, bool zerobackground,
                                             vector<double>& allpeaksvalues)
  {
    // 1. Group the peak
    vector<vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > > peakgroupvec;
    groupPeaks(peakgroupvec);

    // 2. Calculate each peak's intensity and set
    bool peakheightsphysical = true;
    for (size_t ig = 0; ig < peakgroupvec.size(); ++ig)
    {
      g_log.debug() << "[DBx351] Peak group " << ig << " : number of peaks = "
                          << peakgroupvec[ig].size() << "\n";
      bool localphysical = calculateGroupPeakIntensities(peakgroupvec[ig], dataws,
                                                         workspaceindex, zerobackground, allpeaksvalues);
      if (!localphysical)
      {
        peakheightsphysical = false;
      }
    }

    return peakheightsphysical;
  }

  //----------------------------------------------------------------------------------------------
  /** Group peaks together
    * @param peakgroupvec:  output vector containing peaks grouped together.
    * Disabled argument: MatrixWorkspace_sptr dataws, size_t workspaceindex,
   */
  void LeBailFit::groupPeaks(vector<vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > >& peakgroupvec)
  {
    // 1. Sort peaks
    if (m_dspPeaks.size() > 0)
    {
      sort(m_dspPeaks.begin(), m_dspPeaks.end());
    }
    else
    {
      std::stringstream errmsg;
      errmsg << "Group peaks:  No peak is found in the peak vector. ";
      g_log.error() << errmsg.str() << "\n";
      throw std::runtime_error(errmsg.str());
    }
    size_t numpeaks = m_dspPeaks.size();

    // 2. Group peaks
    peakgroupvec.clear();

    // a) Starting value
    vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peakgroup;
    size_t ipk = 0;

    while (ipk < numpeaks)
    {
      peakgroup.push_back(m_dspPeaks[ipk]);
      if (ipk < numpeaks-1)
      {
        // Test whether next peak will be the different group
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
        ThermalNeutronBk2BkExpConvPVoigt_sptr rightpeak = m_dspPeaks[ipk+1].second;

        double thisrightbound = thispeak->centre() + PEAKRANGECONSTANT * thispeak->fwhm();
        double rightleftbound = rightpeak->centre() - PEAKRANGECONSTANT * rightpeak->fwhm();

        if (thisrightbound < rightleftbound)
        {
          // This peak and right peak are away
          vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peakgroupcopy = peakgroup;
          peakgroupvec.push_back(peakgroupcopy);
          peakgroup.clear();
        }
        else
        {
          // Do nothing
          ;
        }
      }
      else
      {
        // Last peak.  Push the current
        vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peakgroupcopy = peakgroup;
        peakgroupvec.push_back(peakgroupcopy);
      }
      ++ ipk;
    } // ENDWHILE

    g_log.debug() << "[Calculate Peak Intensity]:  Number of Peak Groups = " << peakgroupvec.size()
                  << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak's intensities in a group and set the calculated peak height
   * to the corresponding peak function.
   * @param allpeaksvalues:  vector containing the peaks values.  Increment will be made on each
   *                      peak group
   * @param peakgroup:  vector of peak-centre-dpsace value and peak function pair for peaks that are overlapped
   * @param dataws:  data workspace for the peaks
   * @param wsindex: workspace index of the peaks data in dataws
   * @param zerobackground: true if background is zero
   */
  bool LeBailFit::calculateGroupPeakIntensities(vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peakgroup,
                                                 MatrixWorkspace_sptr dataws, size_t wsindex, bool zerobackground,
                                                 vector<double>& allpeaksvalues)
  {    
    // 1. Sort by d-spacing
    if (peakgroup.empty())
    {
      throw runtime_error("Programming error such that input peak group cannot be empty!");
    }
    else
    {
      g_log.debug() << "[DBx155] Peaks group size = " << peakgroup.size() << "\n";
    }
    if (peakgroup.size() > 1)
      sort(peakgroup.begin(), peakgroup.end());

    const MantidVec& vecX = dataws->readX(wsindex);
    const MantidVec& vecY = dataws->readY(wsindex);

    // Check input vector validity
    if (allpeaksvalues.size() != vecY.size())
    {
      stringstream errss;
      errss << "Input vector 'allpeaksvalues' has wrong size = " << allpeaksvalues.size()
            << " != data workspace Y's size = " << vecY.size();
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // 2. Check boundary
    ThermalNeutronBk2BkExpConvPVoigt_sptr leftpeak = peakgroup[0].second;
    double leftbound = leftpeak->centre() - PEAKRANGECONSTANT * leftpeak->fwhm();
    if (leftbound < vecX[0])
    {
      g_log.information() << "Peak group's left boundary " << leftbound << " is out side of "
                          << "input data workspace's left bound (" << vecX[0]
                          << ")! Accuracy of its peak intensity might be affected.\n";
      leftbound = vecX[0] + 0.1;
    }
    ThermalNeutronBk2BkExpConvPVoigt_sptr rightpeak = peakgroup.back().second;
    double rightbound = rightpeak->centre() + PEAKRANGECONSTANT * rightpeak->fwhm();
    if (rightbound > vecX.back())
    {
      g_log.information() << "Peak group's right boundary " << rightbound << " is out side of "
                          << "input data workspace's right bound (" << vecX.back()
                          << ")! Accuracy of its peak intensity might be affected.\n";
      rightbound = vecX.back() - 0.1;
    }

    // 3. Calculate calculation range to input workspace: [ileft, iright)
    vector<double>::const_iterator cviter;

    cviter = lower_bound(vecX.begin(), vecX.end(), leftbound);
    size_t ileft = static_cast<size_t>(cviter-vecX.begin());
    if (ileft > 0)
      --ileft;

    cviter = lower_bound(vecX.begin(), vecX.end(), rightbound);
    size_t iright = static_cast<size_t>(cviter-vecX.begin());
    if (iright <= vecX.size()-1)
      ++ iright;

    // 4. Integrate
    // a) Data structure to hold result
    size_t ndata = iright-ileft;
    if (ndata == 0 || ndata > iright)
    {
      stringstream errss;
      errss << "[Calcualte Peak Intensity] Group range is unphysical.  iLeft = " << ileft << ", iRight = "
            << iright << "; Number of peaks = " << peakgroup.size()
            << "; Left boundary = " << leftbound << ", Right boundary = " << rightbound
            << "; Left peak FWHM = " << leftpeak->fwhm() << ", Right peak FWHM = " << rightpeak->fwhm();
      for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
      {
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = peakgroup[ipk].second;
        errss << "Peak " << ipk << ":  d_h = " << peakgroup[ipk].first << ", TOF_h = " << thispeak->centre()
              << ", FWHM = " << thispeak->fwhm() << "\n";
        vector<string> peakparamnames = thispeak->getParameterNames();
        for (size_t ipar = 0; ipar < peakparamnames.size(); ++ipar)
        {
          errss << "\t" << peakparamnames[ipar] << " = " << thispeak->getParameter(peakparamnames[ipar]) << "\n";
        }
      }

      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    //   Partial data range
    vector<double> datax(vecX.begin()+ileft, vecX.begin()+iright);
    vector<double> datay(vecY.begin()+ileft, vecY.begin()+iright);
    if (datax.size() != ndata)
    {
      g_log.error() << "Partial peak size = " << datax.size() << " != ndata = " << ndata << "\n";
      throw runtime_error("ndata error!");
    }

    // FunctionDomain1DVector xvalues(datax);

    g_log.debug() << "[DBx356] Number of data points = " << ndata << " index from " << ileft
                  << " to " << iright << ";  Size(datax, datay) = " << datax.size() << "\n";

    vector<double> sumYs(ndata, 0.0);
    size_t numPeaks(peakgroup.size());
    vector<vector<double> > peakvalues(numPeaks);

    // b) Integrage peak by peak
    for (size_t ipk = 0; ipk < numPeaks; ++ipk)
    {
      // calculate peak function value
      ThermalNeutronBk2BkExpConvPVoigt_sptr peak = peakgroup[ipk].second;
      // FunctionValues localpeakvalue(xvalues);
      vector<double> localpeakvalue(ndata, 0.0);

      // peak->function(xvalues, localpeakvalue);
      peak->functionLocal(localpeakvalue, datax);

      // check data
      size_t numbadpts(0);
      vector<double>::const_iterator localpeakvalue_end = localpeakvalue.end();
      for (auto it = localpeakvalue.begin(); it != localpeakvalue_end; ++it)
      {
        if ( (*it != 0.) && (*it < NEG_DBL_MAX || *it > DBL_MAX))
        {
          numbadpts++;
        }
      }

      // report the problem and/or integrate data
      if (numbadpts == 0)
      {
        // Data is fine.  Integrate them all
        for (size_t i = 0; i < ndata; ++i)
        {
          // If value is physical
          sumYs[i] += localpeakvalue[i];
        }
      }
      else
      {
        // Report the problem

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        stringstream warnss;
        warnss << "Peak (" << h << ", " << k << ", " << l <<") has " << numbadpts << " data points whose "
               << "values exceed limit (i.e., not physical).\n";
        g_log.warning(warnss.str());
      }
      peakvalues[ipk].assign(localpeakvalue.begin(), localpeakvalue.end());
    } // For All peaks

    // 5. Calculate intensity of all peaks
    vector<double> pureobspeaksintensity(ndata);

    // a) Remove background
    if (zerobackground)
    {
      pureobspeaksintensity.assign(datay.begin(), datay.end());
    }
    else
    {
      // Non-zero background.  Remove the background
      FunctionDomain1DVector xvalues(datax);
      FunctionValues bkgdvalue(xvalues);
      m_backgroundFunction->function(xvalues, bkgdvalue);

      for (size_t i = 0; i < ndata; ++i)
        pureobspeaksintensity[i] = datay[i] - bkgdvalue[i];

      /*
      stringstream dbss;
      dbss << "DBx254 I wonder that all background's parameters are zero!\n";
      vector<string> bkgdparnames = m_backgroundFunction->getParameterNames();
      for (size_t i = 0; i < bkgdparnames.size(); ++i)
        dbss << bkgdparnames[i] << " = " << m_backgroundFunction->getParameter(i) << "\n";
      g_log.error(dbss.str());
      */
    }

    bool peakheightsphysical = true;
    for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
    {
      ThermalNeutronBk2BkExpConvPVoigt_sptr peak = peakgroup[ipk].second;
      double intensity = 0.0;

      for (size_t i = 0; i < ndata; ++i)
      {
        double temp;
        if (sumYs[i] > 1.0E-5)
        {
          // Reasonable non-zero value
          double peaktogroupratio = peakvalues[ipk][i]/sumYs[i];
          temp = pureobspeaksintensity[i] * peaktogroupratio;
        }
        else
        {
          // SumY too smaller
          temp = 0.0;
        }
        double deltax;
        if (i == 0)
          deltax = datax[1] - datax[0];
        else
          deltax = datax[i] - datax[i-1];
        intensity += temp * deltax;
      } // for data points

      if (intensity != intensity)
      {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = NaN!\n";

      }
      else if (intensity <= -DBL_MAX || intensity >= DBL_MAX)
      {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = Infty!\n";
      }
      else if (intensity < 0.0)
      {
        // No negative intensity
        intensity = 0.0;
      }

      g_log.debug() << "[DBx407] Peak @ " << peak->centre() << ": Set Intensity = " << intensity << "\n";
      peak->setHeight(intensity);

      // Add peak's value to peaksvalues
      for (size_t i = ileft; i < iright; ++i)
      {
        allpeaksvalues[i] += (intensity * peakvalues[ipk][i-ileft]);
      }

    } // ENDFOR each peak

    return peakheightsphysical;
  }

  //================================= Import/Parse and Output  ===================================

  //----------------------------------------------------------------------------------------------
  /** Parse the input TableWorkspace to some maps for easy access
  */
  void LeBailFit::parseInstrumentParametersTable()
  {
    // 1. Check column orders
    if (parameterWS->columnCount() < 3)
    {
      g_log.error() << "Input parameter table workspace does not have enough number of columns. "
                    << " Number of columns (Input =" << parameterWS->columnCount() << ") >= 3 as required.\n";
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
        g_log.error() << errmsg.str() << "\n";
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
        g_log.error() << errmsg.str() << "\n";
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
        g_log.error() << errmsg.str() << "\n";
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

      m_funcParameters.insert(std::make_pair(newparameter.name, newparameter));
      m_origFuncParameters.insert(std::make_pair(newparameter.name, newparameter.value));

      if (newparameter.fit)
      {
        g_log.information() << "[Input]: " << newparameter.name << ": value = " << newparameter.value
                            << " Range: [" << newparameter.minvalue << ", " << newparameter.maxvalue
                            << "], MC Step = " << newparameter.stepsize << ", Fit? = "
                            << newparameter.fit << "\n";
      }
    }

    g_log.information() << "DB1118: Successfully Imported Peak Parameters TableWorkspace "
                        << parameterWS->name() << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse the reflections workspace to a list of reflections;
   * Output --> mPeakHKLs
   * It will NOT screen the peaks whether they are in the data range.
  */
  void LeBailFit::parseBraggPeaksParametersTable()
  {
    g_log.debug() << "DB1119:  Importing HKL TableWorkspace\n";

    // 1. Check column orders
    std::vector<std::string> colnames = reflectionWS->getColumnNames();
    if (colnames.size() < 3)
    {
      g_log.error() << "Input parameter table workspace does not have enough number of columns. "
                    << " Number of columns = " << colnames.size() << " < 3 as required.\n";
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }
    if (colnames[0].compare("H") != 0 ||
        colnames[1].compare("K") != 0 ||
        colnames[2].compare("L") != 0)
    {
      stringstream errss;
      errss << "Input Bragg peak parameter TableWorkspace does not have the columns in order.  "
            << "It must be H, K, L. for the first 3 columns.";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // Has peak height?
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

      // 3. Insert related data structure
      std::vector<int> hkl;
      hkl.push_back(h);
      hkl.push_back(k);
      hkl.push_back(l);

      // optional peak height
      double peakheight = 1.0;
      if (hasPeakHeight)
      {
        trow >> peakheight;
      }

      m_inputPeakInfoVec.push_back(make_pair(hkl, peakheight));
    } // ENDFOR row

    g_log.debug() << "DB1119:  Finished importing HKL TableWorkspace.   Size of Rows = "
                  << numrows << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse table workspace (from Fit()) containing background parameters to a vector
   */
  void LeBailFit::parseBackgroundTableWorkspace(TableWorkspace_sptr bkgdparamws, vector<double>& bkgdorderparams)
  {
    g_log.debug() << "DB1105A Parsing background TableWorkspace.\n";

    // 1. Clear (output) map
    bkgdorderparams.clear();
    std::map<std::string, double> parmap;

    // 2. Check
    std::vector<std::string> colnames = bkgdparamws->getColumnNames();
    if (colnames.size() < 2)
    {
      g_log.error() << "Input parameter table workspace must have more than 1 columns\n";
      throw std::invalid_argument("Invalid input background table workspace. ");
    }
    else
    {
      if (!(boost::starts_with(colnames[0], "Name") && boost::starts_with(colnames[1], "Value")))
      {
        // Column 0 and 1 must be Name and Value (at least started with)
        g_log.error() << "Input parameter table workspace have wrong column definition.\n";
        for (size_t i = 0; i < 2; ++i)
          g_log.error() << "Column " << i << " Should Be Name.  But Input is " << colnames[0] << "\n";
        throw std::invalid_argument("Invalid input background table workspace. ");
      }
    }

    g_log.debug() << "DB1105B Background TableWorkspace is valid.\n";

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
    g_log.information() << "DB1105 Importing background TableWorkspace is finished. " << msg.str() << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Make output workspace valid if there is some error.
    * @param workspaceindex:  the workspace index of the spectra in m_outputWS to write fake data
    * @param functionmode:    LeBailFit's mode of function
    */
  void LeBailFit::writeFakedDataToOutputWS(size_t workspaceindex, int functionmode)
  {
    // 1. Initialization
    g_log.notice() << "Input peak parameters are incorrect.  Fake output data for function mode "
                   << functionmode << "\n";

    if (functionmode == 2)
    {
      std::stringstream errmsg;
      errmsg << "Function mode " << functionmode << " is not supported for fake output data.";
      g_log.error() << errmsg.str() << "\n";
      throw std::invalid_argument(errmsg.str());
    }

    // 2. X&Y values
    const MantidVec& IX = m_dataWS->readX(workspaceindex);
    for (size_t iw = 0; iw < m_outputWS->getNumberHistograms(); ++iw)
    {
      MantidVec& X = m_outputWS->dataX(iw);
      for (size_t i = 0; i < X.size(); ++i)
      {
        X[i] = IX[i];
      }

      MantidVec& Y = m_outputWS->dataY(iw);
      if (iw == 0)
      {
        const MantidVec& IY = m_dataWS->readY(workspaceindex);
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

  //----------------------------------------------------------------------------------------------
  /** Create and set up an output TableWorkspace for each individual peaks
   * Parameters include H, K, L, Height, TOF_h, PeakGroup, Chi^2, FitStatus
   * Where chi^2 and fit status are used only in 'CalculateBackground'
   */
  void LeBailFit::exportBraggPeakParameterToTable()
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
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      // a. Access peak function
      CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr tpeak = m_dspPeaks[ipk].second;

      // b. Get peak's nature parameters
      int h, k, l;
      tpeak->getMillerIndex(h, k, l);
      double tof_h = tpeak->centre();
      double height = tpeak->height();
      double alpha = tpeak->getPeakParameter("Alpha");
      double beta = tpeak->getPeakParameter("Beta");
      double sigma2 = tpeak->getPeakParameter("Sigma2");
      double gamma = tpeak->getPeakParameter("Gamma");
      double fwhm = tpeak->fwhm();

      // c) New row
      API::TableRow newrow = peakWS->appendRow();
      newrow << h << k << l << height << tof_h << alpha << beta << sigma2 << gamma << fwhm
             << -1 << -1.0 << "N/A";

      if (tof_h < 0)
      {
        stringstream errss;
        errss << "Peak (" << h << ", " << k << ", " << l << "): TOF_h (=" << tof_h
              << ") is NEGATIVE!";
        g_log.error(errss.str());
      }
    }

    // 4. Set
    this->setProperty("OutputPeaksWorkspace", peakWS);

    g_log.notice("[DBx403] Set property to OutputPeaksWorkspace.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Write data (domain, values) to one specified spectrum of output workspace
    * @param wsindex  :  workspace index of the spectrum of m_outputWS to write data in
    * @param domain   :  FunctionDomain of the data to write
    * @param values   :  FunctionValues of the data write
    */
  void LeBailFit::writeToOutputWorkspace(size_t wsindex, FunctionDomain1DVector domain,  FunctionValues values)
  {
    // Check workspace index
    if (m_outputWS->getNumberHistograms() <= wsindex)
    {
      stringstream errss;
      errss << "LeBailFit.writeToOutputWorkspace.  Try to write to spectrum " << wsindex << " out of range = "
            << m_outputWS->getNumberHistograms();
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // Write X
    for (size_t i = 0; i < domain.size(); ++i)
    {
      m_outputWS->dataX(wsindex)[i] = domain[i];
    }

    // Write Y & E (square root of Y)
    for (size_t i = 0; i < values.size(); ++i)
    {
      m_outputWS->dataY(wsindex)[i] = values[i];
      if (fabs(values[i]) > 1.0)
        m_outputWS->dataE(wsindex)[i] = std::sqrt(fabs(values[i]));
      else
        m_outputWS->dataE(wsindex)[i] = 1.0;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Write orignal data and difference b/w data and model to output's workspace
    * @param workspaceindex  :  workspace index of the spectrum of m_outputWS to write data in
    * @param domain   :  FunctionDomain of the data to write
    * index 0 and 2
   */
  void LeBailFit::writeInputDataNDiff(size_t workspaceindex, API::FunctionDomain1DVector domain)
  {
    // 1. X-axis
    for (size_t i = 0; i < domain.size(); ++i)
    {
      m_outputWS->dataX(0)[i] = domain[i];
      m_outputWS->dataX(2)[i] = domain[i];
    }

    // 2. Add data and difference to output workspace (spectrum 1)
    for (size_t i = 0; i < m_dataWS->readY(workspaceindex).size(); ++i)
    {
      double modelvalue = m_outputWS->readY(1)[i];
      double inputvalue = m_dataWS->readY(workspaceindex)[i];
      double diff = modelvalue - inputvalue;
      m_outputWS->dataY(0)[i] = inputvalue;
      m_outputWS->dataY(2)[i] = diff;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a new table workspace for parameter values and set to output
   * to replace the input peaks' parameter workspace
   * @param parammap : map of Parameters whose values are written to TableWorkspace
   */
  void LeBailFit::exportInstrumentParameterToTable(std::map<std::string, Parameter> parammap)
  {
    // 1. Create table workspace
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

    // 2. Add profile parameter value
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
        opiter = m_origFuncParameters.find(parname);
        double origparvalue = -1.0E100;
        if (opiter != m_origFuncParameters.end())
        {
          origparvalue = opiter->second;
        }
        double diff = origparvalue - parvalue;

        // d. (standard) error
        double paramerror = paramiter->second.error;

        // e. create the row
        double min = paramiter->second.minvalue;
        double max = paramiter->second.maxvalue;
        double step = paramiter->second.stepsize;

        API::TableRow newparam = tablews->appendRow();
        newparam << parname << parvalue << fit_tie << paramerror << min << max << step << diff;
      } // ENDIF
    }

    // 3. Add chi^2
    if (m_fitMode == FIT && !m_inputParameterPhysical)
    {
      // Impossible mode
      throw runtime_error("Impossible to have this situation happen.  Flag 541.");
    }
    else if (!m_inputParameterPhysical)
    {
      // Input instrument profile parameters are not physical
      m_lebailCalChi2 = DBL_MAX;
      m_lebailFitChi2 = DBL_MAX;
    }

    API::TableRow fitchi2row = tablews->appendRow();
    fitchi2row << "FitChi2" << m_lebailFitChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    API::TableRow chi2row = tablews->appendRow();
    chi2row << "Chi2" << m_lebailCalChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;

    // 4. Add to output peroperty
    this->setProperty("OutputParameterWorkspace", parameterws);

    g_log.debug("[DBx404] Set Property To Instrument Parameter Workspace.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Do statistics to result (fitted or calcualted)
  */
  void LeBailFit::doResultStatistics()
  {
    const MantidVec& oY = m_outputWS->readY(0);
    const MantidVec& eY = m_outputWS->readY(1);
    const MantidVec& oE = m_outputWS->readE(0);

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

    m_funcParameters.insert(std::make_pair("LocalChi2", localchi2));

    g_log.information() << "[VZ] LeBailFit Result:  chi^2 = " << chi2 << "\n";

    return;
  }

  //-----------------------------------------------------------------------------
  /** Create output data workspace
    */
  void LeBailFit::createOutputDataWorkspace()
  {
    // 1. Determine number of output spectra
    size_t nspec;
    bool plotindpeak = this->getProperty("PlotIndividualPeaks");

    switch (m_fitMode)
    {
      case FIT:
        // Lebail Fit mode
        // (0) original data
        // (1) fitted data
        // (2) difference
        // --------------------------------------------
        // (3) fitted pattern w/o background
        // (4) background (being fitted after peak)
        // (5) calculation based on input only (no fit)
        // (6) background (input)
        // (7) original data with background removed;
        ;

      case MONTECARLO:
        // Monte Carlo fit mode.  Same as LebailFit mode
        nspec = 9;

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
          nspec += m_dspPeaks.size();

        break;

      case BACKGROUNDPROCESS:
        // Background calculation mode
        nspec = 3;

        break;

      default:
        // Error default
        std::stringstream errmsg;
        errmsg << "Function mode " << m_fitMode << " is not supported in createOutputWorkspace.";
        g_log.error() << errmsg.str() << "\n";
        throw std::runtime_error(errmsg.str());

        break;
    }

    // 2. Create workspace2D and set the data to spectrum 0 (common among all)
    size_t nbin = m_dataWS->dataX(m_wsIndex).size();
    m_outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
    for (size_t i = 0; i < nbin; ++i)
    {
      for (size_t j = 0; j < m_outputWS->getNumberHistograms(); ++j)
        m_outputWS->dataX(j)[i] = m_dataWS->readX(m_wsIndex)[i];
      m_outputWS->dataY(0)[i] = m_dataWS->readY(m_wsIndex)[i];
      m_outputWS->dataE(0)[i] = m_dataWS->readE(m_wsIndex)[i];
    }

    // 3. Set axis
    m_outputWS->getAxis(0)->setUnit("TOF");

    API::TextAxis* tAxis = 0;

    switch (m_fitMode)
    {
      case FIT:
        // Fit mode
        ;
      case MONTECARLO:
        // Monte carlo mode, same as FIT
        tAxis = new API::TextAxis(nspec);
        tAxis->setLabel(0, "Data");
        tAxis->setLabel(1, "Calc");
        tAxis->setLabel(2, "Diff");
        tAxis->setLabel(3, "CalcNoBkgd");
        tAxis->setLabel(4, "OutBkgd");
        tAxis->setLabel(5, "InpCalc");
        tAxis->setLabel(6, "InBkgd");
        tAxis->setLabel(7, "DataNoBkgd");
        tAxis->setLabel(8, "SmoothedBkgd");

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
        tAxis = new API::TextAxis(nspec);
        tAxis->setLabel(0, "Data");
        tAxis->setLabel(1, "Background");
        tAxis->setLabel(2, "DataNoBackground");

        break;

      default:
        // Error default
        std::stringstream errmsg;
        errmsg << "Function mode " << m_fitMode << " is not supported in createOutputWorkspace.";
        g_log.error() << errmsg.str() << "\n";
        throw std::runtime_error(errmsg.str());

        break;
    }

    m_outputWS->replaceAxis(1, tAxis);

    return;
  }


  // ====================================== Random Walk Suite ====================================
  //----------------------------------------------------------------------------------------------
  /** Refine instrument parameters by random walk algorithm (MC)
   *
   * @param maxcycles: number of Monte Carlo steps/cycles
   * @param parammap:  map containing Parameters to refine in MC algorithm
   */
  void LeBailFit::execRandomWalkMinimizer(size_t maxcycles, map<string, Parameter>& parammap)
  {
    // 1. Initialization
    const MantidVec& vecInY = m_dataWS->readY(m_wsIndex);
    const MantidVec& vecE = m_dataWS->readE(m_wsIndex);
    size_t numpts = vecInY.size();

    const MantidVec& domain = m_dataWS->readX(m_wsIndex);
    MantidVec values(domain.size(), 0.0);

    //    Strategy and map
    setupRandomWalkStrategy();
    map<string, Parameter> newparammap = parammap;

    //    Random seed
    int randomseed = getProperty("RandomSeed");
    srand(randomseed);

    double startrwp, currwp, startrp, currp;

    // Annealing temperature
    m_Temperature = getProperty("AnnealingTemperature");
    if (m_Temperature < 0)
      m_Temperature = fabs(m_Temperature);

    m_useAnnealing = getProperty("UseAnnealing");

    // Walking style
    bool usedrunkenwalk = getProperty("DrunkenWalk");
    if (usedrunkenwalk)
      m_walkStyle = DRUNKENWALK;
    else
      m_walkStyle = RANDOMWALK;

    // 2. Process background.
    // a) Calculate background
    FunctionDomain1DVector domainB(domain);
    FunctionValues valuesB(domainB);
    m_backgroundFunction->function(domainB, valuesB);
    MantidVec& background = m_outputWS->dataY(INPUTBACKGROUNDINDEX);
    MantidVec& purepeakdata = m_outputWS->dataY(PUREPEAKINDEX);
    MantidVec& purepeakerror = m_outputWS->dataE(PUREPEAKINDEX);

    for (size_t i = 0; i < numpts; ++i)
    {
      background[i] = valuesB[i];
      purepeakdata[i] = vecInY[i] - background[i];
      purepeakerror[i] = vecE[i];
    }

    // b) Reset LeBailFunction's background component to 'zero'
    vector<string> bkgdnames = m_backgroundFunction->getParameterNames();
    for (size_t i = 0; i < bkgdnames.size(); ++i)
    {
      m_backgroundFunction->setParameter(i, 0.0);
    }

    // 3. Calcualte starting Rwp and etc
    bool startvaluevalid = calculateDiffractionPatternMC(m_outputWS, PUREPEAKINDEX, parammap,
                                                         background, values, startrwp, startrp);

    if (!startvaluevalid)
    {
      // Throw exception if starting values are not valid for all
      throw runtime_error("Starting value of instrument profile parameters can generate peaks with"
                          " unphyiscal parameters values.");
    }

    currwp = startrwp;
    currp = startrp;
    m_bestRwp = currwp + 1.0;
    bookKeepBestMCResult(parammap, background, currwp, 0);

    g_log.notice() << "[DBx255] Random-walk Starting Rwp = " << currwp << "\n";

    // 4. Random walk loops
    // generate some MC trace structure
    vector<double> vecIndex(maxcycles+1);
    vector<double> vecRwp(maxcycles+1);
    size_t numinvalidmoves = 0;
    size_t numacceptance = 0;
    bool prevcyclebetterrwp = true;

    // Annealing record
    int numRecentAcceptance = 0;
    int numRecentSteps = 0;

    // Loop start
    for (size_t icycle = 1; icycle <= maxcycles; ++icycle)
    {
      // a) Remove background as background is in the fitting process too
      MantidVec& pVecY = m_outputWS->dataY(PUREPEAKINDEX);
      for (size_t i = 0; i < numpts; ++i)
        pVecY[i] = vecInY[i] - background[i];

      // b) Refine parameters (for all parameters in turn) to data with background removed
      for (size_t igroup = 0; igroup < m_numMCGroups; ++igroup)
      {
        // i.   Propose the value
        bool hasnewvalues = proposeNewValues(m_MCGroups[igroup], currp, parammap, newparammap, prevcyclebetterrwp);

        if (!hasnewvalues)
        {
          // g_log.notice() << "[DB1035.  Group " << igroup << " has no new value propsed. \n";
          continue;
        }

        // ii.  Evaluate
        double newrwp, newrp;
        bool validparams = calculateDiffractionPatternMC(m_outputWS, PUREPEAKINDEX, newparammap, background,
                                                         values, newrwp, newrp);

        // iii. Determine whether to take the change or not
        bool acceptchange;
        if (!validparams)
        {
          ++ numinvalidmoves;
          acceptchange = false;
          prevcyclebetterrwp = false;
        }
        else
        {
          acceptchange = acceptOrDeny(currwp, newrwp);
          if (newrwp < currwp)
            prevcyclebetterrwp = true;
          else
            prevcyclebetterrwp = false;
        }

        g_log.debug() << "[DBx317] Step " << icycle << ": New Rwp = " << setprecision(10)
                      << newrwp << ", Rp = " << setprecision(5) << newrp
                      << "; Accepted = " << acceptchange << "; Proposed parameters valid ="
                      << validparams << "\n";

        // iv. Apply change and book keeping
        if (acceptchange)
        {
          // Apply the change to current
          applyParameterValues(newparammap, parammap);
          currwp = newrwp;
          currp = newrp;

          // All tim ebest
          if (currwp < m_bestRwp)
          {
            // Book keep the best
            bookKeepBestMCResult(parammap, background, currwp, icycle);
          }

          // Statistic
          ++ numacceptance;
          ++ numRecentAcceptance;
        }
        ++ numRecentSteps;

        // e) Annealing
        if (m_useAnnealing)
        {
          // FIXME : Here are some magic numbers
          if (numRecentSteps == 10)
          {
            // i. Change temperature
            if (numRecentAcceptance < 2)
            {
              m_Temperature *= 2.0;
            }
            else if (numRecentAcceptance >= 8)
            {
              m_Temperature /= 2.0;
            }
            // ii  Reset counters
            numRecentAcceptance = 0;
            numRecentSteps = 0;
          }
        }

        // e) Debug output
        // exportDomainValueToFile(domain, values, "mc_step0_group0.dat");
      } // END FOR Group

      // v. Improve the background
      if (currwp < m_bestRwp)
      {
        fitBackground(m_wsIndex, domainB, valuesB, background);
      }

      // vi. Record some information
      vecIndex[icycle] = static_cast<double>(icycle);
      if (currwp < 1.0E5)
        vecRwp[icycle] = currwp;
      else
        vecRwp[icycle] = -1;

      // vii. progress
      if (icycle%10 == 0)
        progress(double(icycle)/double(maxcycles));

    } // ENDFOR MC Cycles

    progress(1.0);

    // 5. Sum up
    // a) Summary output
    g_log.notice() << "[SUMMARY] Random-walk Rwp:  Starting = " << startrwp << ", Best = " << m_bestRwp
                   << " @ Step = " << m_bestMCStep << "; Last Step Rwp = " << currwp
                   << ", Rp = " << currp
                   << ", Acceptance ratio = " << double(numacceptance)/double(maxcycles*m_numMCGroups) << "\n";

    map<string,Parameter>::iterator mapiter;
    for (mapiter = parammap.begin(); mapiter != parammap.end(); ++mapiter)
    {
      Parameter& param = mapiter->second;
      if (param.fit)
      {
        g_log.notice() << setw(10) << param.name << "\t: Average Stepsize = " << setw(10) << setprecision(5)
                       << param.sumstepsize/double(maxcycles)
                       << ", Max Step Size = " << setw(10) << setprecision(5) << param.maxabsstepsize
                       << ", Number of Positive Move = " << setw(4) << param.numpositivemove
                       << ", Number of Negative Move = " << setw(4) << param.numnegativemove
                       << ", Number of No Move = " << setw(4) << param.numnomove << "\n";
      }
    }
    g_log.notice() << "Number of invalid proposed moves = " << numinvalidmoves << "\n";
    exportXYDataToFile(vecIndex, vecRwp, "rwp_trace.dat");

    // b) Calculate again
    calculateDiffractionPatternMC(m_outputWS, PUREPEAKINDEX, m_bestParameters, background,
                                  values, currwp, currp);

    MantidVec& vecModel = m_outputWS->dataY(1);
    MantidVec& vecDiff = m_outputWS->dataY(2);
    MantidVec& vecModelNoBkgd = m_outputWS->dataY(FITTEDPUREPEAKINDEX);
    MantidVec& vecBkgd = m_outputWS->dataY(FITTEDBACKGROUNDINDEX);
    for (size_t i = 0; i < numpts; ++i)
    {
      // Fitted data
      vecModel[i] = values[i] + background[i];
      // Diff
      vecDiff[i] = vecInY[i] - vecModel[i];
      // Model no background
      vecModelNoBkgd[i] = values[i];
      // Different between calculated peaks and raw data
      vecBkgd[i] = vecInY[i] - values[i];
    }

    // c) Apply the best parameters to param
    applyParameterValues(m_bestParameters, parammap);
    Parameter par_rwp;
    par_rwp.name = "Rwp";
    par_rwp.value = m_bestRwp;
    parammap["Rwp"] = par_rwp;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up Monte Carlo random walk strategy
   */
  void LeBailFit::setupRandomWalkStrategy()
  {
    bool fitgeometry = getProperty("FitGeometryParameter");

    stringstream dboutss;
    dboutss << "Monte Carlo minimizer refines: ";

    // 1. Monte Carlo groups
    // a. Instrument gemetry
    vector<string> geomparams;
    addParameterToMCMinimize(geomparams, "Dtt1");
    addParameterToMCMinimize(geomparams, "Dtt1t");
    addParameterToMCMinimize(geomparams, "Dtt2t");
    addParameterToMCMinimize(geomparams, "Zero");
    addParameterToMCMinimize(geomparams, "Zerot");
    addParameterToMCMinimize(geomparams, "Width");
    addParameterToMCMinimize(geomparams, "Tcross");
    if (fitgeometry)
      m_MCGroups.push_back(geomparams);

    dboutss << "Geometry parameters: ";
    for (size_t i = 0; i < geomparams.size(); ++i)
      dboutss << geomparams[i] << "\t\t";
    dboutss << "\n";

    // b. Alphas
    vector<string> alphs;
    addParameterToMCMinimize(alphs, "Alph0");
    addParameterToMCMinimize(alphs, "Alph1");
    addParameterToMCMinimize(alphs, "Alph0t");
    addParameterToMCMinimize(alphs, "Alph1t");
    m_MCGroups.push_back(alphs);

    dboutss << "Alpha parameters";
    for (size_t i = 0; i < alphs.size(); ++i)
      dboutss << alphs[i] << "\t\t";
    dboutss << "\n";

    // c. Beta
    vector<string> betas;
    addParameterToMCMinimize(betas, "Beta0");
    addParameterToMCMinimize(betas, "Beta1");
    addParameterToMCMinimize(betas, "Beta0t");
    addParameterToMCMinimize(betas, "Beta1t");
    m_MCGroups.push_back(betas);

    dboutss << "Beta parameters";
    for (size_t i = 0; i < betas.size(); ++i)
      dboutss << betas[i] << "\t\t";
    dboutss << "\n";

    // d. Sig
    vector<string> sigs;
    addParameterToMCMinimize(sigs, "Sig0");
    addParameterToMCMinimize(sigs, "Sig1");
    addParameterToMCMinimize(sigs, "Sig2");
    m_MCGroups.push_back(sigs);

    dboutss << "Sig parameters";
    for (size_t i = 0; i < sigs.size(); ++i)
      dboutss << sigs[i] << "\t\t";
    dboutss << "\n";

    g_log.notice(dboutss.str());

    m_numMCGroups = m_MCGroups.size();

    // 2. Dictionary for each parameter for non-negative, mcX0, mcX1
    // a) Sig0, Sig1, Sig2
    for (size_t i = 0; i < sigs.size(); ++i)
    {
      string parname = sigs[i];
      m_funcParameters[parname].mcA0 = 2.0;
      m_funcParameters[parname].mcA1 = 1.0;
      m_funcParameters[parname].nonnegative = true;
    }

    // b) Alpha
    for (size_t i = 0; i < alphs.size(); ++i)
    {
      string parname = alphs[i];
      m_funcParameters[parname].mcA1 = 1.0;
      m_funcParameters[parname].nonnegative = false;
    }
    m_funcParameters["Alph0"].mcA0 = 0.05;
    m_funcParameters["Alph1"].mcA0 = 0.02;
    m_funcParameters["Alph0t"].mcA0 = 0.1;
    m_funcParameters["Alph1t"].mcA0 = 0.05;

    // c) Beta
    for (size_t i = 0; i < betas.size(); ++i)
    {
      string parname = betas[i];
      m_funcParameters[parname].mcA1 = 1.0;
      m_funcParameters[parname].nonnegative = false;
    }
    m_funcParameters["Beta0"].mcA0 = 0.5;
    m_funcParameters["Beta1"].mcA0 = 0.05;
    m_funcParameters["Beta0t"].mcA0 = 0.5;
    m_funcParameters["Beta1t"].mcA0 = 0.05;

    // d) Geometry might be more complicated
    m_funcParameters["Width"].mcA0 = 0.0;
    m_funcParameters["Width"].mcA1 = 1.0;
    m_funcParameters["Width"].nonnegative = true;

    m_funcParameters["Tcross"].mcA0 = 0.0;
    m_funcParameters["Tcross"].mcA1 = 1.0;
    m_funcParameters["Tcross"].nonnegative = true;

    m_funcParameters["Zero"].mcA0 = 5.0;  // 5.0
    m_funcParameters["Zero"].mcA1 = 0.0;
    m_funcParameters["Zero"].nonnegative = false;

    m_funcParameters["Zerot"].mcA0 = 5.0; // 5.0
    m_funcParameters["Zerot"].mcA1 = 0.0;
    m_funcParameters["Zerot"].nonnegative = false;

    m_funcParameters["Dtt1"].mcA0 = 5.0;  // 20.0
    m_funcParameters["Dtt1"].mcA1 = 0.0;
    m_funcParameters["Dtt1"].nonnegative = true;

    m_funcParameters["Dtt1t"].mcA0 = 5.0; // 20.0
    m_funcParameters["Dtt1t"].mcA1 = 0.0;
    m_funcParameters["Dtt1t"].nonnegative = true;

    m_funcParameters["Dtt2t"].mcA0 = 0.1;
    m_funcParameters["Dtt2t"].mcA1 = 1.0;
    m_funcParameters["Dtt2t"].nonnegative = false;

    // 4. Reset
    map<string, Parameter>::iterator mapiter;
    for (mapiter = m_funcParameters.begin(); mapiter != m_funcParameters.end(); ++mapiter)
    {
      mapiter->second.movedirection = 1;
      mapiter->second.sumstepsize = 0.0;
      mapiter->second.numpositivemove = 0;
      mapiter->second.numnegativemove = 0;
      mapiter->second.numnomove = 0;
      mapiter->second.maxabsstepsize = -0.0;
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Add parameter (to a vector of string/name) for MC random walk
   * according to Fit in Parameter
   *
   * @param parnamesforMC: vector of parameter for MC minimizer
   * @param parname: name of parameter to check whether to put into refinement list
   */
  void LeBailFit::addParameterToMCMinimize(vector<string>& parnamesforMC, string parname)
  {
    map<string, Parameter>::iterator pariter;
    pariter = m_funcParameters.find(parname);
    if (pariter == m_funcParameters.end())
    {
      stringstream errss;
      errss << "Parameter " << parname << " does not exisit Le Bail function parameters. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    if (pariter->second.fit)
      parnamesforMC.push_back(parname);

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Calculate diffraction pattern in Le Bail algorithm for MC Random walk
   *
   * @param dataws  :  workspace of the data
   * @param wsindex :  workspace index of the data with background removed.
   * @param funparammap:  map of Parameters of the function to optimize
   * @param background:  background values
   * @param values:  function values
   * @param rwp:  output, Rwp of the model to data
   * @param rp:   output, Rp o the model to data
   */
  bool LeBailFit::calculateDiffractionPatternMC(MatrixWorkspace_sptr dataws, size_t wsindex,
                                                 map<string, Parameter> funparammap,
                                                 MantidVec& background, MantidVec& values,
                                                 double &rwp, double& rp)
  {
    // 1. Set the parameters
    // a) Set the parameters to all peaks
    setPeaksParameters(m_dspPeaks, funparammap, 1.0, true);

    // b) Examine peaks are valid
    bool paramsvalid = true;
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      ThermalNeutronBk2BkExpConvPVoigt_sptr peak = m_dspPeaks[ipk].second;
      double d_h, tof_h;
      string errmsg;
      bool localvalid = examinInstrumentParameterValid(peak, d_h, tof_h, errmsg);
      if (!localvalid)
      {
        // Break the loop if the propose peak parameter causes unphysical peaks
        paramsvalid = false;
        break;
      }
    }

    vector<double> peaksvalues(dataws->readY(wsindex).size(), 0.0);

    // 2. Calcualte intensity
    if (paramsvalid)
    {
      // a) Check (Before)
      stringstream dbss;
      dbss << "[T1205] Peak Heights Before Calculating Intensities:\n";
      for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
      {
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
        dbss << "Peak @ d = " << m_dspPeaks[ipk].first << ",  I = " << thispeak->height() << "\n";
      }
      g_log.debug(dbss.str());

      bool zerobackground = true;
      bool heightphysical = calculatePeaksIntensities(dataws, wsindex, zerobackground, peaksvalues);
      if (!heightphysical)
      {
        // If peak heights have some unphyiscal value
        paramsvalid = false;
      }

      // c) Check After
      stringstream dbss2;
      dbss2 << "[T1205] Peak Heights After Calculating Intensities:\n";
      for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
      {
        ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
        dbss2 << "Peak @ d = " << m_dspPeaks[ipk].first << ",  I = " << thispeak->height() << "\n";
      }
      g_log.debug(dbss2.str());

    }

    // 3. Calcualte peak pattern and thus statistic values (rwp)
    if (paramsvalid)
    {
      // Replaced: m_lebailFunction->function(domain, values);

      // a) Background
      // FIXME : Need to clean up m_backgroundFunction->function(....)
      /* as m_background is set to zero.  shouldn't make any difference...
      FunctionDomain1DVector domainB(m_dataWS->readX(m_wsIndex));
      FunctionValues valuesB(domainB);
      m_backgroundFunction->function(domainB, valuesB);
      */

      values.assign(peaksvalues.begin(), peaksvalues.end());

      // b) Peaks
      // FIXME:  There is still tiny difference between using result from calculate-peak-intensity
      //         and functionLocal().  Neglect it at this momement.  And come back for detailed
      //         examine la ter!

      // 5. Calculate Rwp
      vector<double> caldata(values.size(), 0.0);
      std::transform(values.begin(), values.end(), background.begin(), caldata.begin(), std::plus<double>());
      rwp = getRFactor(m_dataWS->readY(m_wsIndex), values, m_dataWS->readE(m_wsIndex));
      rp = -100;
      // calculatePowderPatternStatistic(values, background, rwp, rp);
    }

    if (!paramsvalid)
    {
      // If the propose instrument parameters have some unphysical parameter
      g_log.information() << "Proposed new instrument profile values cause peak(s) to have "
                          << "unphysical parameter values.\n";
      rwp = DBL_MAX;
      rp = DBL_MAX;
    }

    return paramsvalid;
  }

  //----------------------------------------------------------------------------------------------
  /** Propose new parameters
    * @param mcgroup:  monte carlo group
    * @param m_totRwp: total Rwp
    * @param curparammap:  current map of Parameters whose values are used for propose new values
    * @param newparammap:  map of Parameters hold new values
    * @param prevBetterRwp: boolean.  true if previously proposed value resulted in a better Rwp
    *
    * Return: Boolean to indicate whether there is any parameter that have proposed new values in
    *         this group
    */
  bool LeBailFit::proposeNewValues(vector<string> mcgroup, double m_totRwp, map<string, Parameter>& curparammap,
                                    map<string, Parameter>& newparammap, bool prevBetterRwp)
  {
    // TODO: Study the possibility to merge curparammap and newparammap

    bool anyparameterrefined = false;

    for (size_t i = 0; i < mcgroup.size(); ++i)
    {
      // parameter information
      string paramname = mcgroup[i];
      Parameter param = curparammap[paramname];
      if (param.fit)
        anyparameterrefined = true;
      else
        continue;

      // random number between -1 and 1
      double randomnumber = 2*static_cast<double>(rand())/static_cast<double>(RAND_MAX) - 1.0;
      double stepsize = m_dampingFactor * m_totRwp * (param.value * param.mcA1 + param.mcA0) * randomnumber;

      // drunk walk or random walk
      double newvalue;
      if (m_walkStyle == RANDOMWALK)
      {
        // Random walk.  No preference on direction
        newvalue = param.value + stepsize;
      }
      else if (m_walkStyle == DRUNKENWALK)
      {
        // Drunken walk.  Prefer to previous successful move direction
        int prevRightDirection;
        if (prevBetterRwp)
          prevRightDirection = 1;
        else
          prevRightDirection = -1;

        double randirint = static_cast<double>(rand())/static_cast<double>(RAND_MAX);

        // FIXME Here are some MAGIC numbers
        if (randirint < 0.1)
        {
          // Negative direction to previous direction
          stepsize = -1.0*fabs(stepsize)*static_cast<double>(param.movedirection*prevRightDirection);
        }
        else if (randirint < 0.4)
        {
          // No preferance and thus do nothing
        }
        else
        {
          // Positive direction to previous direction
          stepsize = fabs(stepsize)*static_cast<double>(param.movedirection*prevRightDirection);
        }

        newvalue = param.value + stepsize;
      }
      else
      {
        newvalue = DBL_MAX;
        throw runtime_error("Unrecoganized walk style. ");
      }

      // restriction
      if (param.nonnegative && newvalue < 0)
      {
        // If not allowed to be negative
        newvalue = fabs(newvalue);
      }

      // apply to new parameter map
      newparammap[paramname].value = newvalue;

      // record some trace
      Parameter& p = curparammap[paramname];
      if (stepsize > 0)
      {
        p.movedirection = 1;
        ++ p.numpositivemove;
      }
      else if (stepsize < 0)
      {
        p.movedirection = -1;
        ++ p.numnegativemove;
      }
      else
      {
        p.movedirection = -1;
        ++p.numnomove;
      }
      p.sumstepsize += fabs(stepsize);
      if (fabs(stepsize) > p.maxabsstepsize)
        p.maxabsstepsize = fabs(stepsize);

      g_log.debug() << "[DBx257] " << paramname << "\t" << "Proposed value = " << setw(15)
                    << newvalue << " (orig = " << param.value << ",  step = "
                    << stepsize << "), totRwp = " << m_totRwp << "\n";
    }

    return anyparameterrefined;
  }

  //-----------------------------------------------------------------------------------------------
  /** Determine whether the proposed value should be accepted or denied
    * @param currwp:  current Rwp
    * @param newrwp:  Rwp of function whose parameters' values are the proposed.
    */
  bool LeBailFit::acceptOrDeny(double currwp, double newrwp)
  {
    bool accept;

    if (newrwp < currwp)
    {
      // Lower Rwp.  Take the change
      accept = true;
    }
    else
    {
      // Higher Rwp. Take a chance to accept
      double dice = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
      double bar = exp(-(newrwp-currwp)/(currwp*m_Temperature));
      // double bar = exp(-(newrwp-currwp)/m_bestRwp);
      // g_log.notice() << "[DBx329] Bar = " << bar << ", Dice = " << dice << "\n";
      if (dice < bar)
      {
        // random number (dice, 0 and 1) is smaller than bar (between -infty and 0)
        accept = true;
      }
      else
      {
        // Reject
        accept = false;
      }
    }

    return accept;
  }

  //----------------------------------------------------------------------------------------------
  /** Book keep the (sopposed) best MC result
    * @param parammap:  map of Parameters to book keep with
    * @param bkgddata:  background data to book keep with
    * @param rwp :: rwp
    * @param istep:     current MC step to be recorded
   */
  void LeBailFit::bookKeepBestMCResult(map<string, Parameter> parammap, vector<double>& bkgddata, double rwp, size_t istep)
  {
    if (rwp < m_bestRwp)
    {
      // A better solution
      m_bestRwp = rwp;
      m_bestMCStep = istep;

      if (m_bestParameters.size() == 0)
    {
      // If not be initialized, initialize it!
      m_bestParameters = parammap;
      // copyParameterMap(parammap, m_bestParameters);
    }
    else
    {
      applyParameterValues(parammap, m_bestParameters);
      }

      m_bestBackgroundData = bkgddata;
    }
    else
    {
      g_log.warning("Shouldn't be here!");
    }

    return;
  }

  //------------------------------------------------------------------------------------------------
  /** Apply the value of parameters in the source to target
    * @param srcparammap:  map of Parameters whose values to be copied to others;
    * @param tgtparammap:  map of Parameters whose values to be copied from others;
    */
  void LeBailFit::applyParameterValues(map<string, Parameter>& srcparammap, map<string, Parameter>& tgtparammap)
  {
    map<string, Parameter>::iterator srcmapiter;
    map<string, Parameter>::iterator tgtmapiter;
    for (srcmapiter = srcparammap.begin(); srcmapiter != srcparammap.end(); ++srcmapiter)
    {
      string parname = srcmapiter->first;
      Parameter srcparam = srcmapiter->second;

      tgtmapiter = tgtparammap.find(parname);
      if (tgtmapiter == tgtparammap.end())
      {
        stringstream errss;
        errss << "Parameter " << parname << " cannot be found in target Parameter map containing "
              << tgtparammap.size() << " entries. ";
        g_log.error(errss.str());
        throw runtime_error("Programming or memory error!  This situation cannot happen!");
      }

      tgtmapiter->second.value = srcparam.value;
    }

    return;
  }

  //===============================  Background Functions ========================================
  //----------------------------------------------------------------------------------------------
  /** Re-fit background according to the new values
    * FIXME: Still in development
   *
   * @param wsindex   raw data's workspace index
   * @param domain    domain of X's
   * @param values    values
   * @param background  background
   */
  void LeBailFit::fitBackground(size_t wsindex, FunctionDomain1DVector domain,
                                 FunctionValues values, vector<double>& background)
  {
    UNUSED_ARG(background);

    MantidVec& vecSmoothBkgd = m_outputWS->dataY(SMOOTHEDBACKGROUND);

    smoothBackgroundAnalytical(wsindex, domain, values, vecSmoothBkgd);
    // smoothBackgroundExponential(wsindex, domain, values, vecSmoothBkgd);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Smooth background by exponential smoothing algorithm
   *
   * @param wsindex  :  raw data's workspace index
   * @param domain      domain of X's
   * @param peakdata:   pattern of pure peaks
   * @param background: output of smoothed background
   */
  void LeBailFit::smoothBackgroundExponential(size_t wsindex, FunctionDomain1DVector domain,
                                               FunctionValues peakdata, vector<double>& background)
  {
    const MantidVec& vecRawX = m_dataWS->readX(wsindex);
    const MantidVec& vecRawY = m_dataWS->readY(wsindex);

    // 1. Check input
    if (vecRawX.size() != domain.size() || vecRawY.size() != peakdata.size() ||
        background.size() != peakdata.size())
      throw runtime_error("Vector sizes cannot be matched.");

    // 2. Set up peak density
    vector<double> peakdensity(vecRawX.size(), 1.0);
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
      double height = thispeak->height();
      if (height > m_minimumHeight)
      {
        // a) Calculate boundary
        double fwhm = thispeak->fwhm();
        double centre = thispeak->centre();
        double leftbound = centre-3*fwhm;
        double rightbound = centre+3*fwhm;

        // b) Locate boundary positions
        vector<double>::const_iterator viter;
        viter = find(vecRawX.begin(), vecRawX.end(), leftbound);
        int ileft = static_cast<int>(viter-vecRawX.begin());
        viter = find(vecRawX.begin(), vecRawX.end(), rightbound);
        int iright = static_cast<int>(viter-vecRawX.begin());
        if (iright >= static_cast<int>(vecRawX.size()))
          -- iright;

        // c) Update peak density
        for (int i = ileft; i <= iright; ++i)
        {
          peakdensity[i] += 1.0;
        }
      }
    }

    // FIXME : What is bk_prm2???
    double bk_prm2 = 1.0;

    // 3. Get starting and end points value
    size_t numdata = peakdata.size();

    background[0] = vecRawY[0] - peakdata[0];
    background.back() = vecRawY.back() - peakdata[numdata-1];

    // 4. Calculate the backgrouind points
    for (size_t i = numdata-2; i >0; --i)
    {
      double bk_prm1 = (bk_prm2 * (7480.0/vecRawX[i])) / sqrt(peakdensity[i] + 1.0);
      background[i] = bk_prm1*(vecRawY[i]-peakdata[i]) + (1.0-bk_prm1)*background[i+1];
      if (background[i] < 0)
        background[i] = 0.0;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Smooth background by fitting the background to specified background function
   * @param wsindex  :  raw data's workspace index
   * @param domain      domain of X's
   * @param peakdata:   pattern of pure peaks
   * @param background: output of smoothed background
    */
  void LeBailFit::smoothBackgroundAnalytical(size_t wsindex, FunctionDomain1DVector domain,
                                              FunctionValues peakdata, vector<double>& background)
  {
    // 1. Make data ready
    MantidVec& vecData = m_dataWS->dataY(wsindex);
    MantidVec& vecFitBkgd = m_outputWS->dataY(FITTEDBACKGROUNDINDEX);
    MantidVec& vecFitBkgdErr = m_outputWS->dataE(FITTEDBACKGROUNDINDEX);
    size_t numpts = vecFitBkgd.size();
    for (size_t i = 0; i < numpts; ++i)
    {
      vecFitBkgd[i] = vecData[i] - peakdata[i];
      if (vecFitBkgd[i] > 1.0)
        vecFitBkgdErr[i] = sqrt(vecFitBkgd[i]);
      else
        vecFitBkgdErr[i] = 1.0;
    }

    // 2. Fit
    Chebyshev_sptr bkgdfunc(new Chebyshev);
    bkgdfunc->setAttributeValue("n", 6);

    API::IAlgorithm_sptr calalg = this->createChildAlgorithm("Fit", -1.0, -1.0, true);
    calalg->initialize();
    calalg->setProperty("Function", boost::shared_ptr<API::IFunction>(bkgdfunc));
    calalg->setProperty("InputWorkspace", m_outputWS);
    calalg->setProperty("WorkspaceIndex", FITTEDBACKGROUNDINDEX);
    calalg->setProperty("StartX", domain[0]);
    calalg->setProperty("EndX", domain[numpts-1]);
    calalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    calalg->setProperty("CostFunction", "Least squares");
    calalg->setProperty("MaxIterations", 1000);
    calalg->setProperty("CreateOutput", false);

    // 3. Result
    bool successfulfit = calalg->execute();
    if (!calalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      stringstream errss;
      errss << "Fit to Chebyshev background failed in smoothBackgroundAnalytical.";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    double chi2 = calalg->getProperty("OutputChi2overDoF");
    g_log.information() << "Fit to chebysheve background successful with chi^2 = " << chi2 << "\n";

    // 4. Output
    FunctionValues values(domain);
    bkgdfunc->function(domain, values);

    for (size_t i = 0; i < numpts; ++i)
      background[i] = values[i];

    return;
  }


  // ============================ External Auxiliary Functions   =================================

  /** Write a set of (XY) data to a column file
    */
  void exportXYDataToFile(vector<double> vecX, vector<double> vecY, string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    for (size_t i = 0; i < vecX.size(); ++i)
      ofile << setw(15) << setprecision(5) << vecX[i] << setw(15) << setprecision(5)
            << vecY[i] << "\n";

    ofile.close();

    return;
  }

  //-----------------------------------------------------------------------------
  /** Convert a Table to space to some vectors of maps
  void convertTableWorkspaceToMaps(TableWorkspace_sptr tablews, vector<map<string, int> > intmaps,
                                 vector<map<string, string> > strmaps, vector<map<string, double> > dblmaps)
  {
    // 1. Initialize
    intmaps.clear();
    strmaps.clear();
    dblmaps.clear();

    size_t numrows = tablews->rowCount();
    size_t numcols = tablews->columnCount();

    for (size_t i = 0; i < numrows; ++i)
    {
      map<string, int> intmap;
      intmaps.push_back(intmap);

      map<string, string> strmap;
      strmaps.push_back(strmap);

      map<string, double> dblmap;
      dblmaps.push_back(dblmap);
    }

    // 2. Parse
    for (size_t i = 0; i < numcols; ++i)
    {
      Column_sptr column = tablews->getColumn(i);
      string coltype = column->type();
      string colname = column->name();

      for (size_t ir = 0; ir < numrows; ++ir)
      {
      }

    }

    return;
  }
  */


} // namespace CurveFitting
} // namespace Mantid
