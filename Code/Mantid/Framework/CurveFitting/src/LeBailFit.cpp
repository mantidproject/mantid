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

#include <fstream>

#define FITTEDPUREPEAKINDEX   3
#define PUREPEAKINDEX         7   // Output workspace pure peak (data with background removed)
#define FITTEDBACKGROUNDINDEX 4   // Output workspace background at ws index 4
#define INPUTBACKGROUNDINDEX  6   // Input background

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
  this->declareProperty("BackgroundFunctionOrder", 12, "Order of background function.");

  // Input background parameters (array)
  this->declareProperty(new Kernel::ArrayProperty<double>("BackgroundParameters"),
                        "Optional: enter a comma-separated list of background order parameters from order 0. ");

  // Input background parameters (tableworkspace)
  auto tablewsprop3 = new WorkspaceProperty<TableWorkspace>("BackgroundParametersWorkspace", "", Direction::InOut,
                                                            API::PropertyMode::Optional);
  this->declareProperty(tablewsprop3, "Optional table workspace containing the fit result for background.");

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

  declareProperty("Damping", 1.0, "Damping factor if minizer is 'Damping'");

  declareProperty("NumberMinimizeSteps", 100, "Number of Monte Carlo random walk steps.");

  declareProperty("FitGeometryParameter", false, "Option to choose to fit geometry-related parameters.");

  declareProperty("RandomSeed", 1, "Randum number seed.");

  declareProperty("AnnealingTemperature", 1.0, "Temperature used Monte Carlo.  "
                  "Negative temperature is for simulated annealing. ");

  declareProperty("UseAnnealing", true, "Allow annealing temperature adjusted automatically.");

  declareProperty("DrunkenWalk", false, "Flag to use drunken walk algorithm. "
                  "Otherwise, random walk algorithm is used. ");

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

  // 2. Import parameters from table workspace
  this->parseInstrumentParametersTable();
  this->parseBraggPeaksParametersTable();

  // 3. Create LeBail Function & initialize from input
  // a. All individual peaks
  bool inputparamcorrect = generatePeaksFromInput(m_wsIndex);

  // b. Background information
  std::string backgroundtype = this->getProperty("BackgroundType");
  std::vector<double> bkgdorderparams = this->getProperty("BackgroundParameters");
  DataObjects::TableWorkspace_sptr bkgdparamws = this->getProperty("BackgroundParametersWorkspace");

  // c. Create CompositeFunction
  createLeBailFunction(backgroundtype, bkgdorderparams, bkgdparamws);

  g_log.debug() << "LeBail Composite Function: " << m_lebailFunction->asString() << std::endl;

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
                       "parameters." << endl;
    if (m_fitMode == FIT)
    {
      g_log.warning() << "Function mode FIT is disabled.  Convert to Calculation mode. " << endl;
      m_fitMode = CALCULATION;
    }
  }

  // 5. Create output workspace
  this->createOutputDataWorkspace(m_wsIndex, m_fitMode);

  // 6. Real work
  m_lebailFitChi2 = -1; // Initialize
  m_lebailCalChi2 = -1;

  switch (m_fitMode)
  {
    case FIT:
      // LeBail Fit
      g_log.notice() << "Function: Do LeBail Fit." << std::endl;
      if (inputparamcorrect)
      {
        doLeBailFit(m_wsIndex);
      }
      else
      {
        writeFakedDataToOutputWS(m_wsIndex, m_fitMode);
      }
      break;

    case CALCULATION:
      // Calculation
      g_log.notice() << "Function: Pattern Calculation." << std::endl;
      calculatePattern(m_wsIndex);

      break;

    case BACKGROUNDPROCESS:
      // Calculating background
      // FIXME : Determine later whether this functionality is kept or removed!
      g_log.notice() << "Function: Calculate Background (Precisely). " << std::endl;
      calBackground(m_wsIndex);
      break;

    case MONTECARLO:
      // Monte carlo Le Bail refinement
      g_log.notice("Function: Do LeBail Fit By Monte Carlo Random Walk.");
      execRandomWalkMinimizer(m_numMinimizeSteps, m_wsIndex, m_funcParameters);

      break;

    default:
      // Impossible
      std::stringstream errmsg;
      errmsg << "FunctionMode = " << m_fitMode <<" is not supported in exec().";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());

      break;
  }

  // 8. Calcualte Chi^2 of Output Data (calcualted or fitted)
  if (m_fitMode == 0 || m_fitMode == 1)
  {
    doResultStatistics();
  }

  // 7. Output peak (table) and parameter workspace
  exportBraggPeakParameterToTable();
  exportInstrumentParameterToTable(m_funcParameters);
  this->setProperty("OutputWorkspace", m_outputWS);

  return;
}


//===================================  Pattern Calculation & Minimizing  ===============================

//-----------------------------------------------------------------------------
/** Calcualte LeBail diffraction pattern:
 *  Output spectra:
 *  0: data;  1: calculated pattern; 3: difference
 *  4: input pattern w/o background
 *  5~5+(N-1): optional individual peak
 */
void LeBailFit::calculatePattern(size_t workspaceindex)
{
  // 1. Generate domain and value
  const vector<double> vecX = m_dataWS->readX(workspaceindex);
  const vector<double> vecY = m_dataWS->readY(workspaceindex);

  API::FunctionDomain1DVector domain(vecX);
  API::FunctionValues values(domain);

  // 2. Calculate diffraction pattern
  bool useinputpeakheights = this->getProperty("UseInputPeakHeights");
  calculateDiffractionPattern(m_dataWS, workspaceindex, domain, values, m_funcParameters, !useinputpeakheights);

  // 3. Generate output workspace : 5 spectra
  //    vecX
  for (size_t isp = 0; isp < 5; ++isp)
  {
#if 0
    for (size_t i = 0; i < domain.size(); ++i)
      outputWS->dataX(isp)[i] = domain[i];
#else
    m_outputWS->dataX(isp) = vecX;
#endif
  }

  // 4. Add data (0: experimental, 1: calcualted, 2: difference)
#if 0
  for (size_t i = 0; i < values.size(); ++i)
  {
    outputWS->dataY(0)[i] = dataWS->readY(workspaceindex)[i];
    outputWS->dataY(1)[i] = values[i];
    outputWS->dataY(2)[i] = outputWS->dataY(0)[i] - outputWS->dataY(1)[i];
  }
#else
  m_outputWS->dataY(0) = vecY;
  for (size_t i = 0; i < values.size(); ++i)
  {
    m_outputWS->dataY(1)[i] = values[i];
    m_outputWS->dataY(2)[i] = vecY[i]-values[i];
  }
#endif

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
    for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
    {
      int hkl2 = mPeakHKL2[ipk];
      CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = m_peaks[hkl2];
      if (!peak)
      {
        g_log.information() << "[Warning] There is no peak corresponding to (HKL)^2 = " << hkl2 << std::endl;
      }
      else
      {
        peak->function(domain, values);
        for (size_t i = 0; i < domain.size(); ++i)
        {
          m_outputWS->dataX(ipk+5)[i] = domain[i];
        }
        for (size_t i = 0; i < values.size(); ++i)
        {
          m_outputWS->dataY(ipk+5)[i] = values[i];
        }
      }
    }
  }

  return;
}

//-----------------------------------------------------------------------------
/** LeBail Fitting for one self-consistent iteration
 */
void LeBailFit::doLeBailFit(size_t workspaceindex)
{
  // FIXME: m_maxStep should be an instance variable and evaluate in exec as an input property
  int m_maxSteps = 1;

  // 1. Get a copy of input function parameters (map)
  std::map<std::string, Parameter> parammap;
  parammap = m_funcParameters;

  // 2. Do 1 iteration of LeBail fit
  for (int istep = 0; istep < m_maxSteps; ++istep)
  {
    this->unitLeBailFit(workspaceindex, parammap);
  }

  // 3. Output
  m_funcParameters = parammap;

  return;
}

//-----------------------------------------------------------------------------
/** Calculate Le Bail function values with calculating peak intensities
  * Arguments:
  * @param domain :  input data domain
  * @param values :  output function values
  * @param parammap: parameter maps (values)
  */
bool LeBailFit::calculateDiffractionPattern(MatrixWorkspace_sptr dataws, size_t workspaceindex,
                                            FunctionDomain1DVector domain, FunctionValues& values,
                                            map<string, Parameter > parammap, bool recalpeakintesity)
{
  // 1. Set parameters to each peak
  bool allpeaksvalid = true;
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pit;
  for (pit = m_peaks.begin(); pit != m_peaks.end(); ++pit)
  {
    int hkl2 = pit->first;
    double peakheight = mPeakHeights[hkl2];
    setPeakParameters(m_peaks[hkl2], parammap, peakheight, true);
    double d_h, tof_h;
    string errmsg;
    bool localvalid = examinInstrumentParameterValid(m_peaks[hkl2], d_h, tof_h, errmsg);
    if (!localvalid)
    {
      allpeaksvalid = false;
      break;
    }
  }

  // 2. Calculate peak intensities
  if (recalpeakintesity)
  {
    // a) Calcualte peak intensity
    calculatePeaksIntensities(dataws, workspaceindex);

    // b) Debug output
    std::stringstream msg;
    msg << "[DB1209 Pattern Calcuation]  Number of Peaks = " << m_dspPeaks.size() << std::endl;
    for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
    {
      CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = m_dspPeaks[ipk].second;
      int h, k, l;
      peak->getMillerIndex(h, k, l);
      msg << "(" << h << ", " << k << ", " << l << "), H = " << std::setw(7)
          << std::setprecision(5) << peak->height();
      if ((ipk+1) % 4 == 0)
        msg << std::endl;
      else
        msg << ";  ";
    } // ENDFOR: Peak
    g_log.information() << msg.str() << std::endl;
  }

  // 3. Calcualte model pattern
  m_lebailFunction->function(domain, values);

  return allpeaksvalid;
}

//-----------------------------------------------------------------------------
/** Perform one itearation of LeBail fitting
 * Including
 * a) Calculate pattern for peak intensities
 * b) Set peak intensities
 */
// TODO Release 2.4: Arguments: size_t workspaceindex, std::map<std::string, Parameter>& paramma, size_t istep
bool LeBailFit::unitLeBailFit(size_t workspaceindex, std::map<std::string, Parameter>& parammap)
{
  // 1. Generate domain and value
  const std::vector<double> vecX = m_dataWS->readX(workspaceindex);
  API::FunctionDomain1DVector domain(vecX);
  API::FunctionValues values(domain);

  // 2. Calculate peak intensity and etc.
  // FIXME Release 2.4 Single out the process to calculate intensity.
  // FIXME Move calculation of peak out of UnitLeBailFit
  bool calpeakintensity = true;
  this->calculateDiffractionPattern(m_dataWS, workspaceindex, domain, values, parammap, calpeakintensity);

  // a) Apply initial calculated result to output workspace
  mWSIndexToWrite = 5;
  writeToOutputWorkspace(domain, values);

  // b) Calculate input background
  m_backgroundFunction->function(domain, values);
  mWSIndexToWrite = 6;
  writeToOutputWorkspace(domain, values);

  // 3. Construct the tie.  2-level loop. (1) peak parameter (2) peak
  // TODO Release 2.4: setLeBailFitParameters(istep)
  this->setLeBailFitParameters();

  // 4. Construct the Fit
  this->fitLeBailFunction(workspaceindex, parammap);

  // TODO (1) Calculate Rwp, Chi^2, .... for the fitted pattern.

  // 5. Do calculation again and set the output
  // FIXME Move this part our of UnitLeBailFit
  calpeakintensity = true;
  API::FunctionValues newvalues(domain);
  this->calculateDiffractionPattern(m_dataWS, workspaceindex, domain, newvalues, parammap, calpeakintensity);

  // Add final calculated value to output workspace
  mWSIndexToWrite = 1;
  writeToOutputWorkspace(domain, newvalues);

  // Add original data and
  writeInputDataNDiff(workspaceindex, domain);

  return true;
}

//-----------------------------------------------------------------------------
/** Set up the fit/tie/set-parameter for LeBail Fit (mode)
  * All parameters              : set the value
  * Parameters for free fit     : do nothing;
  * Parameters fixed            : set them to be fixed;
  * Parameters for fit with tie : tie all the related up
 */
void LeBailFit::setLeBailFitParameters()
{
  // 1. Set up all the peaks' parameters... tie to a constant value.. or fit by tieing same parameters of among peaks
  std::map<std::string, Parameter>::iterator pariter;
  for (pariter = m_funcParameters.begin(); pariter != m_funcParameters.end(); ++pariter)
  {
    Parameter funcparam = pariter->second;

    g_log.debug() << "Step 1:  Set peak parameter value " << funcparam.name << std::endl;

    std::string parname = pariter->first;
    // double parvalue = funcparam.value;

    // a) Check whether it is a parameter used in Peak
    std::vector<std::string>::iterator sit;
    sit = std::find(m_peakParameterNames.begin(), m_peakParameterNames.end(), parname);
    if (sit == m_peakParameterNames.end())
    {
      // Not a peak profile parameter
      g_log.debug() << "Unable to tie parameter " << parname << " b/c it is not a parameter for peak. " << std::endl;
      continue;
    }

    if (!funcparam.fit)
    {
      // a) Fix the value to a constant number
      std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator peakiter;
      size_t peakindex = 0;
      for (peakiter = m_peaks.begin(); peakiter != m_peaks.end(); ++peakiter)
      {
        std::stringstream ss1, ss2;
        ss1 << "f" << peakindex << "." << parname;
        ss2 << funcparam.value;
        std::string tiepart1 = ss1.str();
        std::string tievalue = ss2.str();
        m_lebailFunction->tie(tiepart1, tievalue);
        g_log.debug() << "LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

        // TODO: Make a map between peak parameter name and index. And use fix() to replace tie
        /*
        size_t paramindex = m_peakParameterMap(parname);
        peakiter->second->fix(paramindex);
        g_log.debug() << "[DBx907] Fix " << parname << " to " << funcparam.value << endl;
        */
        ++ peakindex;
      } // For each peak
    }
    else
    {
      // b) Tie the values among all peaks, but will fit
      for (size_t ipk = 1; ipk < m_peaks.size(); ++ipk)
      {
        std::stringstream ss1, ss2;
        ss1 << "f" << (ipk-1) << "." << parname;
        ss2 << "f" << ipk << "." << parname;
        std::string tiepart1 = ss1.str();
        std::string tiepart2 = ss2.str();
        m_lebailFunction->tie(tiepart1, tiepart2);
        g_log.debug() << "LeBailFit.  Fit(Tie) / " << tiepart1 << " / " << tiepart2 << " /" << std::endl;
      }

      // c) Set the constraint
      std::stringstream parss;
      parss << "f0." << parname;
      string parnamef0 = parss.str();
      CurveFitting::BoundaryConstraint* bc = new BoundaryConstraint(m_lebailFunction.get(), parnamef0, funcparam.minvalue, funcparam.maxvalue);
      m_lebailFunction->addConstraint(bc);

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

  // 2. Set 'Height' to be fixed
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator peakiter;
  size_t peakindex = 0;
  for (peakiter = m_peaks.begin(); peakiter != m_peaks.end(); ++peakiter)
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

    m_lebailFunction->tie(tiepart1, tievalue);

    ++peakindex;

  } // For each peak

  // 3. Fix all background paramaters to constants/current values
  size_t funcindex = m_peaks.size();
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

    g_log.debug() << "Step 2: LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

    m_lebailFunction->tie(tiepart1, tievalue);

    // TODO: Prefer to use fix other than tie().  Need to figure out the parameter index from name
    /*
    mLeBailFunction->fix(paramindex);
    */
  }

  return;
}

//-----------------------------------------------------------------------------
/** Fit LeBailFunction by calling Fit()
  * NO NEED to set up the parameter value, fix the parameter or tie.
  * Be called after all functions in LeBailFunction (composite) are set up (tie, constrain)
  * Output: a parameter name-value map
 */
bool LeBailFit::fitLeBailFunction(size_t workspaceindex, std::map<std::string, Parameter> &parammap)
{
  // 1. Prepare fitting boundary parameters.
  double tof_min = m_dataWS->dataX(workspaceindex)[0];
  double tof_max = m_dataWS->dataX(workspaceindex).back();
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
  minimizeFunction(m_dataWS, workspaceindex, boost::shared_ptr<API::IFunction>(m_lebailFunction),
                   tof_min, tof_max, mMinimizer, m_dampingFactor, numiterations, fitstatus, m_lebailFitChi2, true);

  /*----------------------------------------------------------------------------------
  // a) Initialize
  std::string fitoutputwsrootname("xLeBailOutput");

  API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
  fitalg->initialize();

  g_log.debug() << "[Before Fit] Function To Fit: " << m_lebailFunction->asString() << std::endl;

  // b) Set property
  m_lebailFunction->useNumericDerivatives( true );
  fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(m_lebailFunction));
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
    g_log.notice() << "[Error] Fitting to LeBail function failed. " << std::endl;
    return false;
  }
  else
  {
    g_log.debug() << "[DBx523] Fitting successful. " << std::endl;
  }

  // d) Process output of fit
  //    chi^2 and status
  m_lebailFitChi2 = fitalg->getProperty("OutputChi2overDoF");
  std::string fitstatus = fitalg->getProperty("OutputStatus");
  ---------------------------------------------------------------------------------------------*/


  // 3. Get parameters
#if 0
  API::IFunction_sptr fitout = fitalg->getProperty("Function");
#else
  g_log.warning("Need to check whether this is valid or not!");
  IFunction_sptr fitout = boost::dynamic_pointer_cast<IFunction>(m_lebailFunction);
#endif
  std::vector<std::string> parnames = fitout->getParameterNames();

  std::stringstream rmsg;
  rmsg << "Fitting Result: " << std::endl;

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
        g_log.warning() << "[Fitting Result] Parameter " << parnamex << " is not set to refine.  "
                        << "But its chi^2 =" << error << std::endl;
      }
    }
  }

  g_log.notice(rmsg.str());

  // 4. Calculate Chi^2 wih all parmeters fixed
  // a) Fit all parameters
  vector<string> lbparnames = m_lebailFunction->getParameterNames();
  for (size_t i = 0; i < lbparnames.size(); ++i)
  {
    m_lebailFunction->fix(i);
  }

  // b) Fit/calculation
  numiterations = 0; //
  minimizeFunction(m_dataWS, workspaceindex, boost::shared_ptr<API::IFunction>(m_lebailFunction),
                   tof_min, tof_max, "Levenberg-MarquardtMD", 0.0, numiterations, fitstatus, m_lebailFitChi2, false);


  /*-----------------------------------------------------------------------
  API::IAlgorithm_sptr calalg = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
  calalg->initialize();
  calalg->setProperty("Function", boost::shared_ptr<API::IFunction>(m_lebailFunction));
  calalg->setProperty("InputWorkspace", dataWS);
  calalg->setProperty("WorkspaceIndex", int(workspaceindex));
  calalg->setProperty("StartX", tof_min);
  calalg->setProperty("EndX", tof_max);
  calalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  calalg->setProperty("CostFunction", "Least squares");
  calalg->setProperty("MaxIterations", 0);
  calalg->setProperty("CreateOutput", false);

  successfulfit = calalg->execute();
  if (!calalg->isExecuted() || ! successfulfit)
  {
    // Early return due to bad fit
    g_log.error() << "Fitting to LeBail function failed. " << std::endl;
    throw runtime_error("DBx457 This is not possible!");
  }

  m_lebailCalChi2 = calalg->getProperty("OutputChi2overDoF");
  --------------------------------------------------------------------------*/
  g_log.notice() << "LeBailFit (LeBailFunction) Fit result:  Chi^2 (Fit) = " << m_lebailFitChi2
                 << ", Chi^2 (Cal) = " << m_lebailCalChi2
                 << ", Fit Status = " << fitstatus << std::endl;

  // TODO: Check the covariant matrix to see whether any NaN or Infty.  If so, return false with reason
  // TODO: (continue).  Code should fit again with Simplex and extends MaxIteration if not enough... ...

  return true;
}


//-----------------------------------------------------------------------------
/** Minimize a give function
  *
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
  API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", -1.0, -1.0, true);
  fitalg->initialize();

  g_log.debug() << "[DBx534 | Before Fit] Function To Fit: " << function->asString() << std::endl;

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
    g_log.notice() << "[Error] Fitting to LeBail function failed. " << std::endl;
    return false;
  }
  else
  {
    g_log.debug() << "[DBx523] Fitting successful. " << std::endl;
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
      g_log.warning() << "Expected covariance matrix cannot be found with algorithm Fit." << endl;
    }
  }

  // VZ: Disable this because it is the same to parse TableWorkspace and m_lebailFunction!
  /* Disabled
  // 5. Export error by parsing Fit's output workspace
  //
  API::ITableWorkspace_sptr fitvaluews = fitalg->getProperty("OutputParameters");
  if (fitvaluews)
  {
    for (size_t ir = 0; ir < fitvaluews->rowCount(); ++ir)
    {
      // a) Get row and parse
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
  else
  {
    g_log.warning() << "OutputParameters TableWorkspace cannot be obtained from Fit. " << endl;
  }

  // e) Get parameter output workspace from it for error
  if (fitvaluews)
  {

  }
  */

  return true;
}

//-----------------------------------------------------------------------------
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
  /*-----------  Disabled -----------------------------
  // 0. Set peak parameters to each peak
  // 1. Set parameters to each peak
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pit;
  for (pit = mPeaks.begin(); pit != mPeaks.end(); ++pit)
  {
    int hkl2 = pit->first;
    double peakheight = mPeakHeights[hkl2];
    setPeakParameters(mPeaks[hkl2], mFuncParameters, peakheight, true);
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

    double tof_min = m_dataWS->readX(workspaceindex).back()+ 1.0;
    double tof_max = m_dataWS->readX(workspaceindex)[0] - 1.0;

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
    // CurveFitting::BackgroundFunction_sptr backgroundfunc = this->generateBackgroundFunction(backgroundtype, orderparm);

    groupedpeaks->addFunction(mBackgroundFunction);

    g_log.debug() << "DB1217 Composite Function of Peak Group: " << groupedpeaks->asString()
                        << std::endl << "Boundary: " << tof_min << ", " << tof_max << std::endl;

    // b. Fit peaks in the peak group
    double unitprog = double(ipg)*0.9/double(peakgroups.size());
    API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", double(ipg)*unitprog, double(ipg+1)*unitprog, true);
    fitalg->initialize();

    fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(groupedpeaks));
    fitalg->setProperty("InputWorkspace", m_dataWS);
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

  API::FunctionDomain1DVector domain(m_dataWS->readX(workspaceindex));
  API::FunctionValues values(domain);
  m_lebailFunction->function(domain, values);

  for (size_t i = 0; i < m_dataWS->readX(workspaceindex).size(); ++i)
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

  */
  return;

}

//===================================  Set up the Le Bail Fit   ===============================
//-----------------------------------------------------------------------------
/** Create LeBailFunction
  */
void LeBailFit::createLeBailFunction(string backgroundtype, vector<double>& bkgdorderparams,
                                     TableWorkspace_sptr bkgdparamws)
{
  // 1. Background parameters are from ...
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
                      << " Order = " << order << std::endl;
  g_log.debug() << "DBx423: Create background function: " << m_backgroundFunction->asString() << std::endl;

  // 3. Generate the composite function
  API::CompositeFunction compfunction;
  m_lebailFunction = boost::make_shared<API::CompositeFunction>(compfunction);
  m_lebailFunction->useNumericDerivatives(true);

  // 4. Add peaks to LeBail Function
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator mit;
  for (mit = m_peaks.begin(); mit != m_peaks.end(); ++ mit)
  {
    m_lebailFunction->addFunction(mit->second);
  }
  m_lebailFunction->addFunction(m_backgroundFunction);

  return;
}

//-----------------------------------------------------------------------------
/** Crop workspace if user required
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


//===================================  Operation with Bragg Peaks   ===========================
//-----------------------------------------------------------------------------
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
bool LeBailFit::generatePeaksFromInput(size_t workspaceindex)
{
  // 0. Set up some constants for peak
  ThermalNeutronBk2BkExpConvPV temppeak;
  temppeak.initialize();
  m_peakParameterNames = temppeak.getParameterNames();
  std::sort(m_peakParameterNames.begin(), m_peakParameterNames.end());

  // 1. Prepare
  size_t numpeaksoutofrange = 0;
  size_t numpeaksparamerror = 0;
  stringstream errss;

  double tofmin = m_dataWS->readX(workspaceindex)[0];
  double tofmax =  m_dataWS->readX(workspaceindex).back();

  // 2. Generate peaks
  //    There is no need to consider peak's order now due to map
  for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
  {
    // a) Generate peak
    int h = mPeakHKLs[ipk][0];
    int k = mPeakHKLs[ipk][1];
    int l = mPeakHKLs[ipk][2];
    int hkl2 = h*h+k*k+l*l;

    CurveFitting::ThermalNeutronBk2BkExpConvPV tmppeak;
    tmppeak.setMillerIndex(h, k, l);
    tmppeak.initialize();
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr speak = boost::make_shared
        <CurveFitting::ThermalNeutronBk2BkExpConvPV>(tmppeak);

    // b) Set peak function
    this->setPeakParameters(speak, this->m_funcParameters, 1.0, false);
    speak->setPeakRadius(mPeakRadius);

    // c) Calculate peak parameters
    double tof_h, d_h;
    string errmsg;
    bool validparam = examinInstrumentParameterValid(speak, d_h, tof_h, errmsg);

    // 6. Add peak to peak map???
    //  Exclude peak out of range
    if (tof_h < tofmin || tof_h > tofmax)
    {
      g_log.debug() << "Input peak (" << h << ", " << k << ", " << l << ") is out of range. "
                    << "TOF_h = " << tof_h << std::endl;
      ++ numpeaksoutofrange;
      continue;
    }

    if (validparam)
    {
      // Add peak to the list
      m_peaks.insert(std::make_pair(hkl2, speak));
      std::vector<int>::iterator fiter = std::find(mPeakHKL2.begin(), mPeakHKL2.end(), hkl2);
      if (fiter == mPeakHKL2.end())
      {
        // Yes.  It is a new peak
        mPeakHKL2.insert(fiter, hkl2);
      }
      else
      {
        // This (HKL)^2 has inserted.  It is not right
        std::stringstream errmsg;
        errmsg << "H^2+K^2+L^2 = " << hkl2 << " already exists. This situation is not considered";
        g_log.error()  << errmsg.str() << std::endl;
        throw std::invalid_argument(errmsg.str());
      }
    }
    else
    {
      // Peak has UNPHYSICAL profile parameter value from model
      ++ numpeaksparamerror;
      errss << "[Warning] Peak (" << h << ", " << k << ", " << l << "), d_h = " << d_h
            << ", TOF_h = " << tof_h << ": "
            << errmsg << endl;
    }
  } // ENDFOR All Input (HKL)

  g_log.information() << "Number of ... Input Peaks = " << mPeakHKLs.size() << "; Peaks Generated: " << mPeakHKL2.size()
                      << "; Peaks With Error Parameters = " << numpeaksparamerror
                      << "; Peaks Outside Range = " <<  numpeaksoutofrange
                      << "; Range: " << setprecision(5) << tofmin << ", " << setprecision(5) << tofmax <<  std::endl;

  if (numpeaksparamerror > 0)
  {
    g_log.information(errss.str());
  }

  // 3. Set up vector of pair(d-spacing, peak)
  map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pairiter;
  for (pairiter = m_peaks.begin(); pairiter != m_peaks.end(); ++pairiter)
  {
    ThermalNeutronBk2BkExpConvPV_sptr peak = pairiter->second;
    double dh = peak->getPeakParameter("d_h");
    m_dspPeaks.push_back(make_pair(dh, peak));
  }
  sort(m_dspPeaks.begin(), m_dspPeaks.end());

  // Return
  return (numpeaksparamerror == 0);
}

//-----------------------------------------------------------------------------
/** Examine whether the insturment parameter set to a peak can cause a valid set of
  * peak profile of that peak
  */
bool LeBailFit::examinInstrumentParameterValid(ThermalNeutronBk2BkExpConvPV_sptr peak, double& d_h, double& tof_h,
                                               string& errmsg)
{
  // 1. Calculate peak parameters
  double eta, alpha, beta, H, sigma2, gamma, N;
  peak->calculateParameters(d_h, tof_h, eta, alpha, beta, H, sigma2, gamma, N, false);

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

//-----------------------------------------------------------------------------
/** From table/map to set parameters to an individual peak.
  * It mostly is called by function in calculation.
  *
 */
void LeBailFit::setPeakParameters(ThermalNeutronBk2BkExpConvPV_sptr peak, map<std::string, Parameter> parammap,
                                  double peakheight, bool setpeakheight)
{
  // 1. Prepare, sort parameters by name
  std::map<std::string, Parameter>::iterator pit;

  // 2. Apply parameters values to peak function
  for (pit = parammap.begin(); pit != parammap.end(); ++pit)
  {
    // a) Check whether the parameter is a peak parameter
    std::string parname = pit->first;
    std::vector<std::string>::iterator ifind =
        std::find(m_peakParameterNames.begin(), m_peakParameterNames.end(), parname);

    // b) Set parameter value
    if (ifind == m_peakParameterNames.end())
    {
      // If not a peak profile parameter, skip
      g_log.debug() << "Parameter " << parname
                    << " in input parameter table workspace is not for peak function. " << std::endl;
    }
    else
    {
      // Set value
      double value = pit->second.value;
      peak->setParameter(parname, value);
      g_log.debug() << "LeBailFit Set " << parname << "= " << value << std::endl;
    }
  } // ENDFOR: parameter iterator

  // 3. Peak height
  if (setpeakheight)
    peak->setParameter("Height", peakheight);

  /* 4. Debug output
  stringstream dbss;
  vector<string> dbparamnames = peak->getParameterNames();
  for (size_t i = 0; i < dbparamnames.size(); ++i)
  {
    dbss << dbparamnames[i] << " = " << peak->getParameter(dbparamnames[i]) << endl;
  }
  g_log.notice(dbss.str());
  */

  return;
}

//============================ Implement Le Bail Formular: Calculate Peak Intensities =========

//-----------------------------------------------------------------------------
/** Calculate peak heights from the model to the observed data
 * Algorithm will deal with
 * (1) Peaks are close enough to overlap with each other
 * The procedure will be
 * (a) Assign peaks into groups; each group contains either (1) one peak or (2) peaks overlapped
 * (b) Calculate peak intensities for every peak per group
 */
bool LeBailFit::calculatePeaksIntensities(MatrixWorkspace_sptr dataws, size_t workspaceindex)
{
  // 1. Group the peak
  vector<vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > > peakgroupvec;
  groupPeaks(peakgroupvec);

  // 2. Calculate each peak's intensity and set
  bool peakheightsphysical = true;
  for (size_t ig = 0; ig < peakgroupvec.size(); ++ig)
  {
    g_log.information() << "[DBx351] Peak group " << ig << " : number of peaks = " << peakgroupvec[ig].size() << endl;
    bool localphysical = calculateGroupPeakIntensities(peakgroupvec[ig], dataws, workspaceindex);
    if (!localphysical)
    {
      peakheightsphysical = false;
    }
  }

  return peakheightsphysical;
}

//----------------------------------------------------------------------------
/** Group peaks together
  * Disabled argument: MatrixWorkspace_sptr dataws, size_t workspaceindex,
  */
void LeBailFit::groupPeaks(vector<vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > >& peakgroupvec)
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
    g_log.error() << errmsg.str() << std::endl;
    throw std::runtime_error(errmsg.str());
  }
  size_t numpeaks = m_dspPeaks.size();

  // 2. Group peaks
  peakgroupvec.clear();

  // a) Starting value
  vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > peakgroup;
  size_t ipk = 0;

  while (ipk < numpeaks)
  {
    peakgroup.push_back(m_dspPeaks[ipk]);
    if (ipk < numpeaks-1)
    {
      // Test whether next peak will be the different group
      ThermalNeutronBk2BkExpConvPV_sptr thispeak = m_dspPeaks[ipk].second;
      ThermalNeutronBk2BkExpConvPV_sptr rightpeak = m_dspPeaks[ipk+1].second;

      double thisrightbound = thispeak->centre() + PEAKRANGECONSTANT * thispeak->fwhm();
      double rightleftbound = rightpeak->centre() - PEAKRANGECONSTANT * rightpeak->fwhm();

      if (thisrightbound < rightleftbound)
      {
        // This peak and right peak are away
        vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > peakgroupcopy = peakgroup;
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
      vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > peakgroupcopy = peakgroup;
      peakgroupvec.push_back(peakgroupcopy);
    }
    ++ ipk;
  } // ENDWHILE

  g_log.information() << "[Calculate Peak Intensity]:  Number of Peak Groups = " << peakgroupvec.size() << std::endl;

  return;
}

//----------------------------------------------------------------------------
/** Calculate peak's intensities in a group and set the calculated peak height
  * to the corresponding peak function.
  */
bool LeBailFit::calculateGroupPeakIntensities(vector<pair<double, ThermalNeutronBk2BkExpConvPV_sptr> > peakgroup,
                                              MatrixWorkspace_sptr dataws, size_t wsindex)
{
  // 1. Sort by d-spacing
  if (peakgroup.size() == 0)
  {
    throw runtime_error("Programming error such that input peak group cannot be empty!");
  }
  else
  {
    g_log.information() << "[DBx155] Peaks group size = " << peakgroup.size() << endl;
  }
  sort(peakgroup.begin(), peakgroup.end());

  const MantidVec& X = dataws->readX(wsindex);
  const MantidVec& Y = dataws->readY(wsindex);

  // 2. Check boundary
  ThermalNeutronBk2BkExpConvPV_sptr leftpeak = peakgroup[0].second;
  double leftbound = leftpeak->centre() - PEAKRANGECONSTANT * leftpeak->fwhm();
  if (leftbound < X[0])
  {
    g_log.information() << "Peak group's left boundary " << leftbound << " is out side of "
                    << "input data workspace's left bound (" << X[0]
                    << ")! Accuracy of its peak intensity might be affected. " << endl;
    leftbound = X[0] + 0.1;
  }
  ThermalNeutronBk2BkExpConvPV_sptr rightpeak = peakgroup.back().second;
  double rightbound = rightpeak->centre() + PEAKRANGECONSTANT * rightpeak->fwhm();
  if (rightbound > X.back())
  {
    g_log.information() << "Peak group's right boundary " << rightbound << " is out side of "
                    << "input data workspace's right bound (" << X.back()
                    << ")! Accuracy of its peak intensity might be affected. " << endl;
    rightbound = X.back() - 0.1;
  }

  // 3. Calculate calculation range to input workspace: [ileft, iright)
  vector<double>::const_iterator cviter;

  cviter = lower_bound(X.begin(), X.end(), leftbound);
  size_t ileft = static_cast<size_t>(cviter-X.begin());
  if (ileft > 0)
    --ileft;

  cviter = lower_bound(X.begin(), X.end(), rightbound);
  size_t iright = static_cast<size_t>(cviter-X.begin());
  if (iright <= X.size()-1)
    ++ iright;

  // 4. Integrate
  // a) Data structure to hold result
  size_t ndata = iright-ileft;
  if (ndata == 0 || ndata > iright)
  {
    stringstream errss;
    errss << "[Calcualte Peak Intensity] Group range is unphysical.  iLeft = " << ileft << ", iRight = "
          << iright << "; Number of peaks = " << peakgroup.size();
    for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
    {
      ThermalNeutronBk2BkExpConvPV_sptr thispeak = peakgroup[ipk].second;
      errss << "Peak " << ipk << ":  d_h = " << peakgroup[ipk].first << ", TOF_h = " << thispeak->centre()
            << ", FWHM = " << thispeak->fwhm() << endl;
      vector<string> peakparamnames = thispeak->getParameterNames();
      for (size_t ipar = 0; ipar < peakparamnames.size(); ++ipar)
      {
        errss << "\t" << peakparamnames[ipar] << " = " << thispeak->getParameter(peakparamnames[ipar]) << endl;
      }
    }

    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  //   Partial data range
  vector<double> datax(X.begin()+ileft, X.begin()+iright);
  vector<double> datay(Y.begin()+ileft, Y.begin()+iright);

  FunctionDomain1DVector xvalues(datax);

  g_log.information() << "[DBx356] Number of data points = " << ndata << " index from " << ileft
                      << " to " << iright << ";  Size(datax, datay) = " << datax.size() << endl;

  vector<double> sumYs(ndata, 0.0);
  vector<FunctionValues> peakvalues;

  // b) Integrage peak by peak
  for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
  {
    // calculate peak function value
    ThermalNeutronBk2BkExpConvPV_sptr peak = peakgroup[ipk].second;
    FunctionValues localpeakvalue(xvalues);

    peak->function(xvalues, localpeakvalue);

    // check data
    bool dataisphysical = true;
    size_t numbadpts = 0;

    for (size_t i = 0; i < ndata; ++i)
    {
      if (localpeakvalue[i] < -DBL_MAX || localpeakvalue[i] > DBL_MAX)
      {
        dataisphysical = false;
        ++ numbadpts;
      }
    }

    // report the problem and/or integrate data
    if (dataisphysical)
    {
      // Data is fine.  Integrate them all
      for (size_t i = 0; i < ndata; ++i)
      {
        // If value is physical
        sumYs[i] += localpeakvalue[i];
      }
      peakvalues.push_back(localpeakvalue);
    }
    else
    {
      // Report the problem
      peakvalues.push_back(localpeakvalue);

      int h, k, l;
      peak->getMillerIndex(h, k, l);
      stringstream warnss;
      warnss << "Peak (" << h << ", " << k << ", " << l <<") has " << numbadpts << " data points whose "
             << "values exceed limit (i.e., not physical). " << endl;
      g_log.warning(warnss.str());
    }
  } // For All peaks

  // 5. Calculate intensity of all peaks
  // a) Remove background
  FunctionValues bkgdvalue(xvalues);
  m_backgroundFunction->function(xvalues, bkgdvalue);
  vector<double> pureobspeaksintensity(ndata);
  for (size_t i = 0; i < ndata; ++i)
  {
    pureobspeaksintensity[i] = datay[i] - bkgdvalue[i];
  }

  bool peakheightsphysical = true;
  for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
  {
    ThermalNeutronBk2BkExpConvPV_sptr peak = peakgroup[ipk].second;
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
      g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = NaN!" <<endl;

    }
    else if (intensity <= -DBL_MAX || intensity >= DBL_MAX)
    {
      // Unphysical intensity: NaN
      intensity = 0.0;
      peakheightsphysical = false;

      int h, k, l;
      peak->getMillerIndex(h, k, l);
      g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = Infty!" <<endl;
    }
    else if (intensity < 0.0)
    {
      // No negative intensity
      intensity = 0.0;
    }

    g_log.debug() << "[DBx407] Peak @ " << peak->centre() << ": Set Intensity = " << intensity << endl;
    peak->setHeight(intensity);
  }

  return peakheightsphysical;
}


//================================= Import/Parse and Output  ===============================

//-----------------------------------------------------------------------------
/** Parse the input TableWorkspace to some maps for easy access
 */
void LeBailFit::parseInstrumentParametersTable()
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

    m_funcParameters.insert(std::make_pair(newparameter.name, newparameter));
    m_origFuncParameters.insert(std::make_pair(newparameter.name, newparameter.value));

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

//-----------------------------------------------------------------------------
/** Parse the reflections workspace to a list of reflections;
  * Output --> mPeakHKLs
  * It will NOT screen the peaks whether they are in the data range.
 */
void LeBailFit::parseBraggPeaksParametersTable()
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
/** Make output workspace valid if there is some error.
  */
void LeBailFit::writeFakedDataToOutputWS(size_t workspaceindex, int functionmode)
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

//-----------------------------------------------------------------------------
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
  std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), compDescending);

  for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
  {
    // a. Access peak function
    int hkl2 = mPeakHKL2[ipk];
    CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr tpeak = m_peaks[hkl2];

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

    // c. Get peak's fitting and etc.
    size_t peakgroupindex = m_peaks.size()+10; // Far more than max peak group index
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
    if (peakgroupindex < m_peaks.size())
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

  g_log.notice("[DBx403] Set property to OutputPeaksWorkspace.");

  return;
}

//-----------------------------------------------------------------------------
/** Write data (domain, values) to one specified spectrum of output workspace
 */
void LeBailFit::writeToOutputWorkspace(API::FunctionDomain1DVector domain,  API::FunctionValues values)
{
  if (m_outputWS->getNumberHistograms() <= mWSIndexToWrite)
  {
    g_log.error() << "LeBailFit.writeToOutputWorkspace.  Try to write to spectrum " << mWSIndexToWrite << " out of range = "
                  << m_outputWS->getNumberHistograms() << std::endl;
    throw std::invalid_argument("Try to write to a spectrum out of range.");
  }

  for (size_t i = 0; i < domain.size(); ++i)
  {
    m_outputWS->dataX(mWSIndexToWrite)[i] = domain[i];
  }
  for (size_t i = 0; i < values.size(); ++i)
  {
    m_outputWS->dataY(mWSIndexToWrite)[i] = values[i];
    if (fabs(values[i]) > 1.0)
      m_outputWS->dataE(mWSIndexToWrite)[i] = std::sqrt(fabs(values[i]));
    else
      m_outputWS->dataE(mWSIndexToWrite)[i] = 1.0;
  }

  return;
}

//-----------------------------------------------------------------------------
/** Write orignal data and difference b/w data and model to output's workspace
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

//-----------------------------------------------------------------------------
/** Create a new table workspace for parameter values and set to output
 * to replace the input peaks' parameter workspace
 * Old: std::pair<double, char>
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

          // d. chi^2
          double paramerror = 0.0;
          opiter = mFuncParameterErrors.find(parname);
          if (opiter != mFuncParameterErrors.end())
          {
            paramerror = opiter->second;
          }

          // e. create the row
          double diff = origparvalue - parvalue;
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
      m_lebailCalChi2 = 1.0E200;
      m_lebailFitChi2 = 1.0E200;
    }

    API::TableRow fitchi2row = tablews->appendRow();
    fitchi2row << "FitChi2" << m_lebailFitChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    API::TableRow chi2row = tablews->appendRow();
    chi2row << "Chi2" << m_lebailCalChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;

    // 4. Add to output peroperty
    this->setProperty("OutputParameterWorkspace", parameterws);

    g_log.notice("[DBx404] Set Property To Instrument Parameter Workspace.");

    return;
}

//-----------------------------------------------------------------------------
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

  g_log.information() << "[VZ] LeBailFit Result:  chi^2 = " << chi2 << std::endl;

  return;
}

//-----------------------------------------------------------------------------
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
      nspec = 8;

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
        nspec += m_peaks.size();

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
  size_t nbin = m_dataWS->dataX(workspaceindex).size();
  m_outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
        API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
  for (size_t i = 0; i < nbin; ++i)
  {
    for (size_t j = 0; j < m_outputWS->getNumberHistograms(); ++j)
      m_outputWS->dataX(j)[i] = m_dataWS->readX(workspaceindex)[i];
    m_outputWS->dataY(0)[i] = m_dataWS->readY(workspaceindex)[i];
    m_outputWS->dataE(0)[i] = m_dataWS->readE(workspaceindex)[i];
  }

  // 3. Set axis
  m_outputWS->getAxis(0)->setUnit("TOF");

  API::TextAxis* tAxis = 0;

  switch (functionmode)
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
      errmsg << "Function mode " << functionmode << " is not supported in createOutputWorkspace.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());

      break;
  }

  m_outputWS->replaceAxis(1, tAxis);

  return;
}


// ============================ Random Walk Suite ==============================
//-----------------------------------------------------------------------------
/** Refine instrument parameters by random walk algorithm (MC)
  *
  * @param wsindex  :  workspace index of the diffraction data (observed)
  */
void LeBailFit::execRandomWalkMinimizer(size_t maxcycles, size_t wsindex,
                                        map<string, Parameter>& parammap)
{
  // 1. Initialization
  const MantidVec& vecX = m_dataWS->readX(wsindex);
  const MantidVec& vecInY = m_dataWS->readY(wsindex);
  const MantidVec& vecE = m_dataWS->readE(wsindex);
  size_t numpts = vecInY.size();

  FunctionDomain1DVector domain(vecX);
  FunctionValues values(domain);
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
  m_backgroundFunction->function(domain, values);
  MantidVec& background = m_outputWS->dataY(INPUTBACKGROUNDINDEX);
  MantidVec& purepeakdata = m_outputWS->dataY(PUREPEAKINDEX);
  MantidVec& purepeakerror = m_outputWS->dataE(PUREPEAKINDEX);

  for (size_t i = 0; i < numpts; ++i)
  {
    background[i] = values[i];
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
                                                       background, domain, values, startrwp, startrp);

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

  g_log.notice() << "[DBx255] Random-walk Starting Rwp = " << currwp << endl;

  // 4. Random walk loops
  // generate some MC trace structure
  vector<double> vecIndex(maxcycles);
  vector<double> vecRwp(maxcycles);
  size_t numinvalidmoves = 0;
  size_t numacceptance = 0;
  bool prevcyclebetterrwp = true;

  //    Annealing record
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
      // proposeNewValues(m_MCGroups[igroup], currwp, parammap, newparammap);
      proposeNewValues(m_MCGroups[igroup], currp, parammap, newparammap, prevcyclebetterrwp);

      // ii.  Evaluate
      double newrwp, newrp;
      bool validparams = calculateDiffractionPatternMC(m_outputWS, PUREPEAKINDEX, newparammap, background, domain,
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
                    << validparams << endl;

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
          numRecentAcceptance = 0.0;
          numRecentSteps = 0;
        }
      }

      // e) Debug output
      // exportDomainValueToFile(domain, values, "mc_step0_group0.dat");
    } // END FOR Group

    // v. Improve the background
    if (currwp < m_bestRwp)
    {
      fitBackground(wsindex, domain, values, background);
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
                 << " @ Step = " << m_bestMCStep << ", Final Rwp = " << currwp
                 << ", Rp = " << currp
                 << ", Acceptance ratio = " << double(numacceptance)/double(maxcycles*m_numMCGroups) << endl;

  map<string,Parameter>::iterator mapiter;
  for (mapiter = parammap.begin(); mapiter != parammap.end(); ++mapiter)
  {
    Parameter& param = mapiter->second;
    g_log.notice() << setw(10) << param.name << "\t: Average Stepsize = " << setw(10) << setprecision(5) << param.sumstepsize/double(maxcycles)
                   << ", Max Step Size = " << setw(10) << setprecision(5) << param.maxabsstepsize
                   << ", Number of Positive Move = " << setw(4) << param.numpositivemove
                   << ", Number of Negative Move = " << setw(4) << param.numnegativemove
                   << ", Number of No Move = " << setw(4) << param.numnomove << endl;

  }
  g_log.notice() << "Number of invalid proposed moves = " << numinvalidmoves << endl;
  exportXYDataToFile(vecIndex, vecRwp, "rwp_trace.dat");

  // b) Calculate again
  calculateDiffractionPatternMC(m_outputWS, PUREPEAKINDEX, m_bestParameters, background, domain, values, currwp, currp);

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

  return;
}

//-----------------------------------------------------------------------------
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
  dboutss << endl;

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
  dboutss << endl;

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
  dboutss << endl;

  // d. Sig
  vector<string> sigs;
  addParameterToMCMinimize(sigs, "Sig0");
  addParameterToMCMinimize(sigs, "Sig1");
  addParameterToMCMinimize(sigs, "Sig2");
  m_MCGroups.push_back(sigs);

  dboutss << "Sig parameters";
  for (size_t i = 0; i < sigs.size(); ++i)
    dboutss << sigs[i] << "\t\t";
  dboutss << endl;

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
  m_funcParameters["Alpha0"].mcA0 = 0.05;
  m_funcParameters["Alpha1"].mcA0 = 0.02;
  m_funcParameters["Alpha0t"].mcA0 = 0.1;
  m_funcParameters["Alpha1t"].mcA0 = 0.05;

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

  m_funcParameters["Zero"].mcA0 = 5.0;
  m_funcParameters["Zero"].mcA1 = 0.0;
  m_funcParameters["Zero"].nonnegative = false;

  m_funcParameters["Zerot"].mcA0 = 5.0;
  m_funcParameters["Zerot"].mcA1 = 0.0;
  m_funcParameters["Zerot"].nonnegative = false;

  m_funcParameters["Dtt1"].mcA0 = 5.0;
  m_funcParameters["Dtt1"].mcA1 = 0.0;
  m_funcParameters["Dtt1"].nonnegative = true;

  m_funcParameters["Dtt1t"].mcA0 = 5.0;
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


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
/** Calculate diffraction pattern in Le Bail algorithm for MC Random walk
  *
  * @param dataws  :  workspace of the data
  * @param wsindex :  workspace index of the data
  */
bool LeBailFit::calculateDiffractionPatternMC(MatrixWorkspace_sptr dataws, size_t wsindex,
                                              map<string, Parameter> funparammap,
                                              MantidVec& background,
                                              FunctionDomain1DVector& domain, FunctionValues& values,
                                              double &rwp, double& rp)
{
  // 1. Set the parameters
  bool paramsvalid = true;
  for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
  {
    ThermalNeutronBk2BkExpConvPV_sptr peak = m_dspPeaks[ipk].second;
    setPeakParameters(peak, funparammap, 1.0, true);
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

  // exportXYDataToFile(dataws->readX(wsindex), dataws->readY(wsindex), "rawdata.dat");

  // 2. Calcualte intensity/pattern, and then statistic (rwp)
  if (paramsvalid)
  {
    // 3. Calculate
    bool heightphysical = calculatePeaksIntensities(dataws, wsindex);
    if (!heightphysical)
    {
      // If peak heights have some unphyiscal value
      paramsvalid = false;
    }

    /* Debug output
    double dtt1 = m_lebailFunction->getFunction(1)->getParameter("Dtt1");
    double alph0 = m_lebailFunction->getFunction(1)->getParameter("Alph0");
    g_log.notice() << "[Ugly-DBx348] Le Bail Function Component 1: Dtt1 = " << dtt1 << ", Alph0 = " << alph0 << endl;
    */

    // 4. Calculate
    m_lebailFunction->function(domain, values);

    // 5. Calculate Rwp
    calculatePowderPatternStatistic(values, background, rwp, rp);
  }

  if (!paramsvalid)
  {
    // If the propose instrument parameters have some unphysical parameter
    g_log.information() << "Proposed new instrument profile values cause peak(s) to have "
                        << "unphysical parameter values." << endl;
    rwp = 1.0E100;
    rp = 1.0E100;
  }

  return paramsvalid;
}

/** Calculate powder diffraction statistic Rwp
  *
  * @param values     : calcualted data (pattern) w/o background
  * @param background : calcualted background
  */
void LeBailFit::calculatePowderPatternStatistic(FunctionValues& values, vector<double>& background, double& rwp,
                                                double& rp)
{
  // 1. Init the statistics
  const MantidVec& obsdata = m_dataWS->readY(m_wsIndex);
  const MantidVec& stderrs = m_dataWS->readE(m_wsIndex);
  size_t numdata = obsdata.size();

  vector<double> caldata(values.size(), 0.0);
  for (size_t i = 0; i < numdata; ++i)
    caldata[i] = values[i] + background[i];

  // 2. Calculate
  rwp = 0.0;
  rp = 0.0;

  double sumobsdata = 0.0;
  double sumobsdatanobkgd = 0.0;
  double sumwgtobsdatasq = 0.0;
  double sumwgtobsdatnobkgdsq = 0.0;
  for (size_t i = 0; i < numdata; ++i)
  {
    sumobsdata += obsdata[i];
    sumobsdatanobkgd += obsdata[i] - background[i];
    sumwgtobsdatasq += obsdata[i]*obsdata[i]*stderrs[i];
    sumwgtobsdatnobkgdsq += (obsdata[i] - background[i])*(obsdata[i] - background[i])*stderrs[i];

    double tmp1 = fabs(obsdata[i] - caldata[i]); // Rp
    double tmp2 = stderrs[i]*tmp1*tmp1;    // Mp
    // double tmp3 = fabs( tmp1*(obsdata[i] - bkgddata[i])/obsdata[i] );
    // double tmp4 = stderrs[i]*tmp3*tmp3;

    rp += tmp1;
    rwp += tmp2;
    // powstats.Rpb += tmp3;
    // powstats.Rwpb += tmp4;
  }

  rp = rp/sumobsdata;
  rwp = sqrt(rwp/sumwgtobsdatasq);

  // 3. Calculate peak related Rwp
  /* Disabled
  // a) Highest peak
  double maxheight = 0.0;
  for (size_t ipk = 0; ipk < m_dspPeaks.size(); ++ipk)
  {
    ThermalNeutronBk2BkExpConvPV_sptr peak = m_dspPeaks[ipk].second;
    if (peak->height() > maxheight)
      maxheight = peak->height();
  }

  // b) Set the threshold
  // FIXME A magic number is used here.
  size_t numcovered = 0;
  double peakthreshold = maxheight * 1.0E-4;
  double diff2sum = 0.0;
  double obs2sum = 0.0;
  for (size_t i = 0; i < numdata; ++i)
  {
    if (values[i] > peakthreshold)
    {
      double diff = obsdata[i] - caldata[i];
      diff2sum += diff * diff / stderrs[i];
      obs2sum += obsdata[i] * obsdata[i] / stderrs[i];
      ++ numcovered;
    }
  }
  double peakrwp = sqrt(diff2sum/obs2sum);

  g_log.information() << "Complete Rwp = " << rwp << "; Peak only Rwp = " << peakrwp
                      << " Number of points included = " << numcovered << " out of " << numdata << endl;
  */

  return;
}

/** Propose new parameters
  */
void LeBailFit::proposeNewValues(vector<string> mcgroup, double m_totRwp, map<string, Parameter>& curparammap,
                                 map<string, Parameter>& newparammap, bool prevBetterRwp)
{
  for (size_t i = 0; i < mcgroup.size(); ++i)
  {
    // random number between -1 and 1
    double randomnumber = 2*static_cast<double>(rand())/static_cast<double>(RAND_MAX) - 1.0;

    // parameter information
    string paramname = mcgroup[i];
    Parameter param = curparammap[paramname];
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
        // No preferance
        stepsize = stepsize;
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
                  << stepsize << "), totRwp = " << m_totRwp << endl;
  }

  return;
}

//-----------------------------------------------------------------------------
/** Determine whether the proposed value should be accepted or denied
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
    // g_log.notice() << "[DBx329] Bar = " << bar << ", Dice = " << dice << endl;
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

//-----------------------------------------------------------------------------
/** Book keep the (sopposed) best MC result
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

//-----------------------------------------------------------------------------
/** Apply the value of parameters in the source to target
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

//===============  Background Functions ========================================

//------------------------------------------------------------------------------
/** Re-fit background according to the new values
  */
void LeBailFit::fitBackground(size_t wsindex, FunctionDomain1DVector domain,
                              FunctionValues values, vector<double>& background)
{
  UNUSED_ARG(wsindex);
  UNUSED_ARG(domain);
  UNUSED_ARG(values);
  UNUSED_ARG(background);

  g_log.information() << "fitBackground() has not been implemented yet!" << endl;

  return;
}

//-----------------------------------------------------------------------------
/** Smooth background by exponential smoothing algorithm
  */
void LeBailFit::smoothBackgroundExponential(size_t wsindex, FunctionDomain1DVector domain,
                                            FunctionValues peakdata, vector<double>& background)
{
  const MantidVec& vecRawX = m_dataWS->readX(wsindex);
  const MantidVec& vecRawY = m_dataWS->readY(wsindex);

  throw runtime_error("It is a fake for bk_prm2 and peak density.");
  double bk_prm2 = 1.0;
  vector<double> peakdensity(vecRawX.size(), 1.0);

  if (vecRawX.size() != domain.size() || vecRawY.size() != peakdata.size() ||
      background.size() != peakdata.size())
    throw runtime_error("Vector sizes cannot be matched.");

  // 1. Get starting and end points value
  size_t numdata = peakdata.size();

  background[0] = vecRawY[0] - peakdata[0];
  background.back() = vecRawY.back() - peakdata[numdata-1];

  // 2.
  for (size_t i = numdata-2; i >0; --i)
  {
    double bk_prm1 = (bk_prm2 * (7480.0/vecRawX[i])) / sqrt(peakdensity[i] + 1.0);
    background[i] = bk_prm1*(vecRawY[i]-peakdata[i]) + (1.0-bk_prm1)*background[i+1];
    if (background[i] < 0)
      background[i] = 0.0;
  }

  return;
}


// ============================ External Auxiliary Functions   =================
//------------------------------------------------------------------------------
/** Parse fx.abc to x and abc where x is the index of function and abc is the parameter name
 */
void parseCompFunctionParameterName(std::string fullparname, std::string& parname, size_t& funcindex)
{
  // 1. Split by '.'
  std::vector<std::string> terms;
  boost::split(terms, fullparname, boost::is_any_of("."));

  if (terms.size() != 2)
  {
    // Throw if it is not in format fx.abc
    stringstream errss;
    errss << "Parameter name : " << fullparname << " does not have 1 and only 1 (.).  Cannot support!" << std::endl;
    throw std::runtime_error(errss.str());
  }

  // 2. Parse function index from term[0]
  if (terms[0][0] != 'f')
  {
    stringstream errss;
    errss << "Inside composite function, member function name, " << fullparname
          << " is not started from 'f'.  But " << terms[0] << ".  It is not supported!";
    throw std::runtime_error("Unsupported CompositeFunction parameter name.");
  }
  std::vector<std::string> t2s;
  boost::split(t2s, terms[0], boost::is_any_of("f"));
  std::stringstream ss(t2s[1]);
  ss >> funcindex;

  // 3. Parse fuction name from term[1]
  parname = terms[1];

  return;
}

//-----------------------------------------------------------------------------
/** Write domain and value to a column file
  */
void exportDomainValueToFile(FunctionDomain1DVector domain, FunctionValues values, string filename)
{
  ofstream ofile;
  ofile.open(filename.c_str());

  for (size_t i = 0; i < domain.size(); ++i)
    ofile << setw(15) << setprecision(5) << domain[i] << setw(15) << setprecision(5)
          << values[i] << endl;

  ofile.close();

  return;
}

//-----------------------------------------------------------------------------
/** Write a set of (XY) data to a column file
  */
void exportXYDataToFile(vector<double> vecX, vector<double> vecY, string filename)
{
  ofstream ofile;
  ofile.open(filename.c_str());

  for (size_t i = 0; i < vecX.size(); ++i)
    ofile << setw(15) << setprecision(5) << vecX[i] << setw(15) << setprecision(5)
          << vecY[i] << endl;

  ofile.close();

  return;
}

//-----------------------------------------------------------------------------
/** Convert a Table to space to some vectors of maps
  */
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



} // namespace CurveFitting
} // namespace Mantid
