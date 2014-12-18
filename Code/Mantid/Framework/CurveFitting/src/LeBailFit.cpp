/* COMMIT NOTES *
  1. Rename calculateDiffractionPatternMC to calculateDiffractionPattern
  2.

 * COMMIT NOTES */

#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/TextAxis.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iomanip>

#include <fstream>

const int OBSDATAINDEX(0);
const int CALDATAINDEX(1);
const int DATADIFFINDEX(2);
const int CALPUREPEAKINDEX(3);
const int CALBKGDINDEX(4); // Output workspace background at ws index 4
const int INPUTBKGDINDEX(6);  // Input background
const int INPUTPUREPEAKINDEX(7);  // Output workspace:  pure peak (data with background removed)
const int SMOOTHEDBKGDINDEX(8); //  Smoothed background

const double NOBOUNDARYLIMIT(1.0E10);
const double EPSILON(1.0E-10);

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  const Rfactor badR(DBL_MAX, DBL_MAX);

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

  //----------------------------------------------------------------------------------------------
  /** Declare the input properties for this algorithm
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
    this->declareProperty(new WorkspaceProperty<TableWorkspace>("InputHKLWorkspace", "", Direction::Input),
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
    functions.push_back("RefineBackground");
    auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
    this->declareProperty("Function", "LeBailFit", validator, "Functionality");

    // Peak type
    vector<string> peaktypes;
    peaktypes.push_back("ThermalNeutronBk2BkExpConvPVoigt");
    peaktypes.push_back("NeutronBk2BkExpConvPVoigt");
    auto peaktypevalidator = boost::make_shared<StringListValidator>(peaktypes);
    declareProperty("PeakType", "ThermalNeutronBk2BkExpConvPVoigt", peaktypevalidator, "Peak profile type.");

    /*------------------------  Background Related Properties  ---------------------------------*/
    // About background:  Background type, input (table workspace or array)
    std::vector<std::string> bkgdtype;
    bkgdtype.push_back("Polynomial");
    bkgdtype.push_back("Chebyshev");
    bkgdtype.push_back("FullprofPolynomial");
    auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
    declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Background type");

    // Input background parameters (array)
    this->declareProperty(new Kernel::ArrayProperty<double>("BackgroundParameters"),
                          "Optional: enter a comma-separated list of background order parameters from order 0. ");

    // Input background parameters (tableworkspace)
    auto tablewsprop3 = new WorkspaceProperty<TableWorkspace>("BackgroundParametersWorkspace", "", Direction::InOut,
                                                              API::PropertyMode::Optional);
    this->declareProperty(tablewsprop3, "Optional table workspace containing the fit result for background.");

    // Peak Radius
    this->declareProperty("PeakRadius", 5, "Range (multiplier relative to FWHM) for a full peak. ");

    /*------------------------  Properties for Calculation Mode --------------------------------*/
    // Output option to plot each individual peak
    declareProperty("PlotIndividualPeaks", false,
                          "Option to output each individual peak in mode Calculation.");
    setPropertySettings("PlotIndividualPeaks",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "Calculation"));

    // Make each reflection visible
    declareProperty("IndicationPeakHeight", 0.0,
                    "Heigh of peaks (reflections) if its calculated height is smaller than user-defined minimum.");
    setPropertySettings("IndicationPeakHeight",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "Calculation"));

    // UseInputPeakHeights
    declareProperty("UseInputPeakHeights", true,
                    "For 'Calculation' mode only, use peak heights specified in ReflectionWorkspace. "
                    "Otherwise, calcualte peaks' heights. ");
    setPropertySettings("UseInputPeakHeights",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "Calculation"));

    /*---------------------------  Properties for Fitting Mode ---------------------------------*/
    // Minimizer
    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys(); // :Instance().getKeys();
    declareProperty("Minimizer","Levenberg-MarquardtMD",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(minimizerOptions)),
                    "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Kernel::Direction::InOut);
    setPropertySettings("Minimizer",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "LeBailFit"));

    declareProperty("Damping", 1.0, "Damping factor if minizer is 'Damping'");
    setPropertySettings("Damping",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "LeBailFit"));
    setPropertySettings("Damping",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));

    declareProperty("NumberMinimizeSteps", 100, "Number of Monte Carlo random walk steps.");
    setPropertySettings("NumberMinimizeSteps",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "LeBailFit"));
    setPropertySettings("NumberMinimizeSteps",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));
    setPropertySettings("NumberMinimizeSteps",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "RefineBackground"));

    //-----------------  Parameters for Monte Carlo Simulated Annealing --------------------------
    auto mcwsprop = new WorkspaceProperty<TableWorkspace>("MCSetupWorkspace", "", Direction::Input,
                                                          PropertyMode::Optional);
    declareProperty(mcwsprop,
                    "Name of table workspace containing parameters' setup for Monte Carlo simualted annearling. ");
    setPropertySettings("MCSetupWorkspace",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));


    declareProperty("RandomSeed", 1, "Randum number seed.");
    setPropertySettings("RandomSeed",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));

    declareProperty("AnnealingTemperature", 1.0, "Temperature used Monte Carlo.  "
                    "Negative temperature is for simulated annealing. ");
    setPropertySettings("AnnealingTemperature",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));

    declareProperty("UseAnnealing", true, "Allow annealing temperature adjusted automatically.");
    setPropertySettings("UseAnnealing",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));

    declareProperty("DrunkenWalk", false, "Flag to use drunken walk algorithm. "
                    "Otherwise, random walk algorithm is used. ");
    setPropertySettings("DrunkenWalk",
                        new VisibleWhenProperty("Function", IS_EQUAL_TO,  "MonteCarlo"));

    declareProperty("MinimumPeakHeight", 0.01, "Minimum height of a peak to be counted "
                    "during smoothing background by exponential smooth algorithm. ");

    // Flag to allow input hkl file containing degenerated peaks
    declareProperty("AllowDegeneratedPeaks", false,
                    "Flag to allow degenerated peaks in input .hkl file. "
                    "Otherwise, an exception will be thrown if this situation occurs.");

    // Tolerance of imported peak's position comparing to data range
    declareProperty("ToleranceToImportPeak", EMPTY_DBL(), "Tolerance in TOF to import peak from Bragg "
                    "peaks list.  If it specified, all peaks within Xmin-Tol and Xmax+Tol will be imported. "
                    "It is used in the case that the geometry parameters are close to true values. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
  */
  void LeBailFit::exec()
  {
    // Process input properties
    processInputProperties();

    // Import parameters from table workspace
    parseInstrumentParametersTable();
    parseBraggPeaksParametersTable();

    // Background function and calculation on it
    processInputBackground();

    // Create Le Bail function
    createLeBailFunction();

    // Create output workspace/workspace
    createOutputDataWorkspace();

    // 5. Adjust function mode according to input values
    if (m_lebailFunction->isParameterValid())
    {
      // All peaks within range are physical and good to refine
      m_inputParameterPhysical = true;
    }
    else
    {
      // Some peaks within range have unphysical parameters.  Just calcualtion for reference
      m_inputParameterPhysical = false;
      g_log.warning() << "Input instrument parameters values cause some peaks to have "
                         "unphysical profile parameters.\n";
      if (m_fitMode == FIT || m_fitMode == MONTECARLO)
      {
        g_log.warning() << "Function mode FIT is disabled.  Convert to Calculation mode.\n";
        m_fitMode = CALCULATION;
      }
    }

    // 7. Do calculation or fitting
    m_lebailFitChi2 = -1; // Initialize
    m_lebailCalChi2 = -1;

    switch (m_fitMode)
    {      
      case CALCULATION:
        // Calculation
        g_log.notice() << "Function: Pattern Calculation.\n";
        execPatternCalculation();
        break;

      case FIT:
        // LeBail Fit
        g_log.notice() << "Function: Do LeBail Fit ==> Monte Carlo.\n";

      case MONTECARLO:
        // Monte carlo Le Bail refinement
        g_log.notice("Function: Do LeBail Fit By Monte Carlo Random Walk.");
        execRandomWalkMinimizer(m_numMinimizeSteps, m_funcParameters);

        break;

      case BACKGROUNDPROCESS:
        // Calculating background
        // FIXME : Determine later whether this functionality is kept or removed!
        g_log.notice() << "Function: Refine Background (Precisely).\n";
        execRefineBackground();
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

    setProperty("OutputWorkspace", m_outputWS);

    // 8. Final statistic
    Rfactor finalR = getRFactor(m_outputWS->readY(0), m_outputWS->readY(1), m_outputWS->readE(0));
    g_log.notice() << "\nFinal R factor: Rwp = " << finalR.Rwp
                   << ", Rp = " << finalR.Rp << ", Data points = " << m_outputWS->readY(1).size()
                   << ", Range = " << m_outputWS->readX(0)[0] << ", " << m_outputWS->readX(0).back() << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input background properties and do the calculation upon it
    * and also calculate the input data with (input) background reduced
    */
  void LeBailFit::processInputBackground()
  {
    // FIXME - Need to think of FullprofPolynomial
    // Type
    m_backgroundType = getPropertyValue("BackgroundType");

    // Parameters
    m_backgroundParameters = getProperty("BackgroundParameters");
    TableWorkspace_sptr bkgdparamws = getProperty("BackgroundParametersWorkspace");

    // Determine where the background parameters are from
    if (!bkgdparamws)
    {
      // Set up parameter name
      m_backgroundParameterNames.clear();
      size_t i0 = 0;
      if (m_backgroundType == "FullprofPolynomial")
      {
        // TODO - Add this special case to Wiki
        m_backgroundParameterNames.push_back("Bkpos");
        if (m_backgroundParameters[0] < m_startX || m_backgroundParameters[0] > m_endX)
          g_log.warning("Bkpos is out side of data range.  It MIGHT NOT BE RIGHT. ");
        i0 = 1;
      }

      size_t numparams = m_backgroundParameters.size();
      for (size_t i = i0; i < numparams; ++i)
      {
        stringstream parss;
        parss << "A" << (i-i0);
        m_backgroundParameterNames.push_back(parss.str());
      }

      g_log.information() << "[Input] Use background specified with vector with input vector sized "
                          << numparams << ".\n";
    }
    else
    {
      g_log.information() << "[Input] Use background specified by table workspace.\n";
      parseBackgroundTableWorkspace(bkgdparamws, m_backgroundParameterNames, m_backgroundParameters);
    }

    // Set up background order
    m_bkgdorder = static_cast<unsigned int>(m_backgroundParameterNames.size());
    if (m_backgroundType == "FullprofPolynomial")
    {
      // Consider 1 extra Bkpos
      if (m_bkgdorder == 0)
        throw runtime_error("FullprofPolynomial: Bkpos must be given! ");
      else if (m_bkgdorder <= 7)
        m_bkgdorder = 6;
      else if (m_bkgdorder <= 13)
        m_bkgdorder = 12;
      else
        throw runtime_error("There is something wrong to set up FullprofPolynomial. ");
    }
    else
    {
      // order n will have n+1 parameters
      if (m_bkgdorder == 0)
        throw runtime_error("Polynomial and Chebyshev at least be order 0 (1 parameter). ");
      -- m_bkgdorder;
    }

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
    // Generate domain and values vectors
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);
    MantidVec& vecY = m_outputWS->dataY(CALDATAINDEX);

    // Calculate diffraction pattern
    Rfactor rfactor(-DBL_MAX, -DBL_MAX);
    // FIXME - It should be a new ticket to turn on this option (use user-specified peak height)
    bool useinputpeakheights = this->getProperty("UseInputPeakHeights");
    if (useinputpeakheights)
      g_log.warning("UseInputPeakHeights is temporarily turned off now. ");

    // Set the parameters to LeBailFunction
    map<string, double> profilemap = convertToDoubleMap(m_funcParameters);
    m_lebailFunction->setProfileParameterValues(profilemap);

    // Calculate peak intensities and diffraction pattern
    vector<double> emptyvec;
    bool resultphysical = calculateDiffractionPattern(m_dataWS->readX(m_wsIndex), m_dataWS->readY(m_wsIndex),
                                                      true, true, emptyvec, vecY, rfactor);

    size_t numpts = vecY.size();
    // Calculate background
    MantidVec& vec_bkgd = m_outputWS->dataY(INPUTBKGDINDEX);
    m_lebailFunction->function(vec_bkgd, vecX, false, true);
    for (size_t i = 0; i < numpts; ++i)
      m_outputWS->dataY(INPUTPUREPEAKINDEX)[i] = m_outputWS->readY(OBSDATAINDEX)[i] -
          m_outputWS->readY(INPUTBKGDINDEX)[i];

    // Set up output workspaces
    for (size_t i = 0; i < numpts; ++i)
    {
      m_outputWS->dataY(DATADIFFINDEX)[i] = m_outputWS->readY(OBSDATAINDEX)[i] - m_outputWS->readY(CALDATAINDEX)[i];
    }

    // Calcualte individual peaks
    bool ploteachpeak = this->getProperty("PlotIndividualPeaks");
    g_log.information() << "Output individual peaks  = " << ploteachpeak << ".\n";
    if (ploteachpeak)
    {
      for (size_t ipk = 0; ipk < m_lebailFunction->getNumberOfPeaks(); ++ipk)
      {
        MantidVec& vecTemp = m_outputWS->dataY(9+ipk);
        m_lebailFunction->calPeak(ipk, vecTemp, vecX);
      }
    }

    // Record for output
    Parameter par_rwp;
    par_rwp.name = "Rwp";
    par_rwp.curvalue = rfactor.Rwp;
    m_funcParameters["Rwp"] = par_rwp;

    g_log.notice() << "Rwp = " << rfactor.Rwp << ", Rp = " << rfactor.Rp << "\n";

    if (!resultphysical)
    {
      g_log.warning() << "Input parameters are unable to generate peaks that are physical." << ".\n";
    }

    return;
  }

  //====================================  Refine background   ====================================
  //----------------------------------------------------------------------------------------------
  /** Calculate background of the specified diffraction pattern
  * by
  * 1. fix the peak parameters but height;
  * 2. fit only heights of the peaks in a peak-group and background coefficients (assumed order 2 or 3 polynomial)
  * 3. remove peaks by the fitting result
  */
  void LeBailFit::execRefineBackground()
  {
    // Set up class variables for background
    m_bkgdParameterNames = m_backgroundFunction->getParameterNames();
    m_numberBkgdParameters = m_bkgdParameterNames.size();
    m_bkgdParameterBuffer.resize(m_numberBkgdParameters);
    m_bestBkgdParams.resize(m_numberBkgdParameters);
    m_roundBkgd = 0;
    m_bkgdParameterStepVec.resize(m_numberBkgdParameters, 0.01);
    for (size_t i = 1; i < m_numberBkgdParameters; ++i)
    {
      m_bkgdParameterStepVec[i] = m_bkgdParameterStepVec[i-1] * 0.0001;
    }

    // 1. Generate domain and value
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);
    const MantidVec& vecY = m_dataWS->readY(m_wsIndex);
    vector<double> valueVec(vecX.size(), 0);
    size_t numpts = vecX.size();

    API::FunctionDomain1DVector domain(vecX);
    API::FunctionValues values(domain);

    // 2. Calculate diffraction pattern
    Rfactor currR(DBL_MAX, DBL_MAX);
    m_backgroundFunction->function(domain, values);
    vector<double> backgroundvalues(numpts);
    for (size_t i = 0; i < numpts; ++i)
    {
      backgroundvalues[i] = values[i];
      m_outputWS->dataY(INPUTPUREPEAKINDEX)[i] = m_dataWS->readY(m_wsIndex)[i] - values[i];
      m_outputWS->dataE(INPUTPUREPEAKINDEX)[i] = m_dataWS->readE(m_wsIndex)[i];
    }
    map<string, double> parammap = convertToDoubleMap(m_funcParameters);
    m_lebailFunction->setProfileParameterValues(parammap);
    calculateDiffractionPattern(m_outputWS->readX(INPUTPUREPEAKINDEX), m_outputWS->readY(INPUTPUREPEAKINDEX),
                                  false, true, backgroundvalues, valueVec, currR);
    Rfactor bestR = currR;
    storeBackgroundParameters(m_bestBkgdParams);
    stringstream bufss;
    bufss << "Starting background parameter ";
    for (size_t i = 0; i < m_bestBkgdParams.size(); ++i)
      bufss << "[" << i << "] = " << m_bestBkgdParams[i] << ", ";
    bufss << ".  Starting Rwp = " << currR.Rwp;
    g_log.notice(bufss.str());

    for (size_t istep = 0; istep < m_numMinimizeSteps; ++istep)
    {
      // a) Store current setup
      storeBackgroundParameters(m_bkgdParameterBuffer);

      // b) Propose new values and evalulate
      proposeNewBackgroundValues();
      Rfactor newR(DBL_MAX, DBL_MAX);
      m_backgroundFunction->function(domain, values);
      for (size_t i = 0; i < numpts; ++i)
      {
        backgroundvalues[i] = values[i];
        m_outputWS->dataY(INPUTPUREPEAKINDEX)[i] = m_dataWS->readY(m_wsIndex)[i] - values[i];
      }
      map<string, double> parammap = convertToDoubleMap(m_funcParameters);
      m_lebailFunction->setProfileParameterValues(parammap);
      calculateDiffractionPattern(m_outputWS->readX(INPUTPUREPEAKINDEX), m_outputWS->readY(INPUTPUREPEAKINDEX),
                                    false, true, backgroundvalues, valueVec, newR);

      g_log.information() << "[DBx800] New Rwp = " << newR.Rwp << ", Rp = " << newR.Rp << ".\n";

      bool accept = acceptOrDeny(currR, newR);

      // c) Process result
      if (!accept)
      {
        // Not accept.  Restore original
        recoverBackgroundParameters(m_bkgdParameterBuffer);
      }
      else
      {
        // Accept
        currR = newR;
        if (newR.Rwp < bestR.Rwp)
        {
          // Is it the best?
          bestR = newR;
          storeBackgroundParameters(m_bestBkgdParams);

          stringstream bufss;
          bufss << "Temp best background parameter ";
          for (size_t i = 0; i < m_bestBkgdParams.size(); ++i)
            bufss << "[" << i << "] = " << m_bestBkgdParams[i] << ", ";
          g_log.information(bufss.str());
        }
      }

      // d) Progress
      progress(static_cast<double>(istep)/static_cast<double>(m_numMinimizeSteps));
    }

    // 3. Recover the best
    recoverBackgroundParameters(m_bestBkgdParams);

    stringstream bufss1;
    bufss1 << "Best background parameter ";
    for (size_t i = 0; i < m_bkgdParameterStepVec.size(); ++i)
      bufss1 << "[" << i << "] = " << m_backgroundFunction->getParameter(i) << ", ";
    g_log.notice(bufss1.str());

    Rfactor outputR(-DBL_MAX, -DBL_MAX);
    m_backgroundFunction->function(domain, values);
    for (size_t i = 0; i < numpts; ++i)
    {
      backgroundvalues[i] = values[i];
      m_outputWS->dataY(INPUTPUREPEAKINDEX)[i] = m_dataWS->readY(m_wsIndex)[i] - values[i];
    }
    parammap = convertToDoubleMap(m_funcParameters);
    m_lebailFunction->setProfileParameterValues(parammap);
    calculateDiffractionPattern(m_outputWS->readX(INPUTPUREPEAKINDEX), m_outputWS->readY(INPUTPUREPEAKINDEX),
                                  false, true, backgroundvalues, valueVec, outputR);

    g_log.notice() << "[RefineBackground] Best Rwp = " << bestR.Rwp << ",  vs. recovered best Rwp = "
                   << outputR.Rwp << ".\n";

    // 4. Add data (0: experimental, 1: calcualted, 2: difference)
    for (size_t i = 0; i < numpts; ++i)
    {
      m_outputWS->dataY(1)[i] = valueVec[i]+backgroundvalues[i];
      m_outputWS->dataY(2)[i] = vecY[i]-(valueVec[i]+backgroundvalues[i]);
    }

    //   (3: peak without background, 4: input background)
    // m_backgroundFunction->function(domain, values);
    for (size_t i = 0; i < values.size(); ++i)
    {
      m_outputWS->dataY(CALBKGDINDEX)[i] = backgroundvalues[i];
      m_outputWS->dataY(CALPUREPEAKINDEX)[i] = valueVec[i];
    }

    // 5. Output background to table workspace
    TableWorkspace_sptr outtablews(new TableWorkspace());
    outtablews->addColumn("str", "Name");
    outtablews->addColumn("double", "Value");
    outtablews->addColumn("double", "Error");

    for (size_t i = 0; i < m_bkgdParameterNames.size(); ++i)
    {
      string parname = m_bkgdParameterNames[i];
      double parvalue = m_backgroundFunction->getParameter(parname);

      TableRow newrow = outtablews->appendRow();
      newrow << parname << parvalue << 1.0;
    }

    setProperty("BackgroundParametersWorkspace", outtablews);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Store/buffer current background parameters
    * @param bkgdparamvec :: vector to save the background parameters whose order is same in background function
    */
  void LeBailFit::storeBackgroundParameters(vector<double>& bkgdparamvec)
  {
    for (size_t i = 0; i < m_numberBkgdParameters; ++i)
    {
      bkgdparamvec[i] = m_backgroundFunction->getParameter(i);
    }

    return;
  }

  /** Restore/recover the buffered background parameters to m_background function
    * @param bkgdparamvec :: vector holding the background parameters whose order is same in background function
    */
  void LeBailFit::recoverBackgroundParameters(const vector<double>& bkgdparamvec)
  {
    for (size_t i = 0; i < m_numberBkgdParameters; ++i)
    {
      m_backgroundFunction->setParameter(i, bkgdparamvec[i]);
    }

    return;
  }

  /** Propose new background parameters
    */
  void LeBailFit::proposeNewBackgroundValues()
  {
    int iparam = m_roundBkgd % static_cast<int>(m_numberBkgdParameters);

    double currvalue = m_backgroundFunction->getParameter(static_cast<int>(iparam));
    double r = 2*(static_cast<double>(rand())/static_cast<double>(RAND_MAX)-0.5);
    double newvalue = currvalue + r * m_bkgdParameterStepVec[iparam];

    g_log.information() << "[DBx804] Background " << iparam << " propose new value = "
                        << newvalue << "  from " << currvalue << ".\n";

    m_backgroundFunction->setParameter(static_cast<size_t>(iparam), newvalue);

    ++ m_roundBkgd;

    return;
  }

  //===================================  Set up the Le Bail Fit   ================================
  //----------------------------------------------------------------------------------------------
  /** Create LeBailFunction, including creating Le Bail function, add peaks and background
   */
  void LeBailFit::createLeBailFunction()
  {
    // Generate Le Bail function
    m_lebailFunction = boost::make_shared<LeBailFunction>(LeBailFunction(m_peakType));

    // Set up profile parameters
    if (m_funcParameters.size() == 0)
      throw runtime_error("Function parameters must be set up by this point.");

    map<string, double> pardblmap = convertToDoubleMap(m_funcParameters);
    m_lebailFunction->setProfileParameterValues(pardblmap);

    // Add peaks
    if (!isEmpty(m_peakCentreTol))
    {
      const MantidVec& vecx = m_dataWS->readX(m_wsIndex);
      m_lebailFunction->setPeakCentreTolerance(m_peakCentreTol, vecx.front(), vecx.back());
    }

    vector<vector<int> > vecHKL;
    vector<pair<vector<int>, double> >::iterator piter;
    for (piter = m_inputPeakInfoVec.begin(); piter != m_inputPeakInfoVec.end(); ++piter)
      vecHKL.push_back(piter->first);
    m_lebailFunction->addPeaks(vecHKL);

    // Add background
    m_lebailFunction->addBackgroundFunction(m_backgroundType, m_bkgdorder, m_backgroundParameterNames, m_backgroundParameters, m_startX, m_endX);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Crop workspace if user required
    * @param inpws :  input workspace to crop
    * @param wsindex: workspace index of the data to fit against
   */
  API::MatrixWorkspace_sptr LeBailFit::cropWorkspace(API::MatrixWorkspace_sptr inpws, size_t wsindex)
  {
    // Process input property 'FitRegion' for range of data to fit/calculate
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

    // Crop workspace
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
      g_log.error("Unable to retrieve a Workspace2D object from ChildAlgorithm CropWorkspace");
      throw runtime_error("Unable to retrieve a Workspace2D object from ChildAlgorithm CropWorkspace");
    }
    else
    {
      g_log.debug() << "DBx307: Cropped Workspace... Range From " << cropws->readX(wsindex)[0] << " To "
                    << cropws->readX(wsindex).back() << " of size " << cropws->readX(wsindex).size()
                    << "\n";
    }

    return cropws;
  }

  //================================= Import/Parse and Output  ===================================
  //----------------------------------------------------------------------------------------------
  /** Process input properties to class variables and do some initial check
    */
  void LeBailFit::processInputProperties()
  {
    // Peak type
    m_peakType = getPropertyValue("PeakType");

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
    m_startX = m_dataWS->readX(0).front();
    m_endX = m_dataWS->readX(0).back();

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
    else if (function.compare("RefineBackground") == 0)
    {
      // Refine background mode
      m_fitMode = BACKGROUNDPROCESS;
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
    if (tempindex > 0)
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

    m_minimumPeakHeight = getProperty("MinimumPeakHeight");
    m_indicatePeakHeight = getProperty("IndicationPeakHeight");

    // Tolerate duplicated input peak or not?
    m_tolerateInputDupHKL2Peaks = getProperty("AllowDegeneratedPeaks");

    // Tolerance in peak positions to import peak
    m_peakCentreTol = getProperty("ToleranceToImportPeak");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse the input TableWorkspace to some maps for easy access
    * Output : m_functionParameters
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
    else
    {
      g_log.information() << "[DB] Starting to parse instrument parameter table workspace "
                          << parameterWS->name() << ".\n";
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
          g_log.debug() << "Col-name = " << colname << ", ";
          trow >> dblvalue;
          g_log.debug() << "Value = " << dblvalue << ".\n";;
          tempdblmap.insert(std::make_pair(colname, dblvalue));
        }
        else
        {
          // string data
          g_log.debug() << "Col-name = " << colname << ", ";
          trow >> strvalue;
          strvalue.erase(std::find_if(strvalue.rbegin(), strvalue.rend(),
                                      std::not1(std::ptr_fun<int, int>(std::isspace))).base(), strvalue.end());

          g_log.debug() << "Value = " << strvalue << ".\n";
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
        newparameter.curvalue = dbliter->second;
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
      newparameter.fiterror = 1.0E10;

      // viii.  some historical records
      // FIXME : Consider to move parameter value's min/max to Monte Carlo table
      newparameter.minrecordvalue = newparameter.maxvalue + 1.0;
      newparameter.maxrecordvalue = newparameter.minvalue - 1.0;

      m_funcParameters.insert(std::make_pair(newparameter.name, newparameter));
      m_origFuncParameters.insert(std::make_pair(newparameter.name, newparameter.curvalue));

      g_log.information() << "Inserting Parameter " << newparameter.name << " = " << newparameter.curvalue << ".\n";

      if (newparameter.fit)
      {
        g_log.debug() << "[Input]: " << newparameter.name << ": value = " << newparameter.curvalue
                            << " Range: [" << newparameter.minvalue << ", " << newparameter.maxvalue
                            << "], MC Step = " << newparameter.stepsize << ", Fit? = "
                            << newparameter.fit << "\n";
      }
    } // ENDFOR rows in Table

    g_log.information() << "[DB]: Successfully Imported Peak Parameters TableWorkspace "
                        << parameterWS->name() << ". Imported " << m_funcParameters.size()
                        << " parameters. " << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse the reflections workspace to a list of reflections;
   * Output --> mPeakHKLs
   * It will NOT screen the peaks whether they are in the data range.
  */
  void LeBailFit::parseBraggPeaksParametersTable()
  {
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

    /* FIXME This section is disabled.  It should be a new ticket to turn on this option.
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

    g_log.information() << "Imported HKL TableWorkspace.   Size of Rows = "
                        << numrows << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse table workspace (from Fit()) containing background parameters to a vector
   */
  void LeBailFit::parseBackgroundTableWorkspace(TableWorkspace_sptr bkgdparamws, vector<string>& bkgdparnames,
                                                vector<double>& bkgdorderparams)
  {
    g_log.debug() << "DB1105A Parsing background TableWorkspace.\n";

    // Clear (output) map
    bkgdorderparams.clear();

    // Check background parameter table workspace
    std::vector<std::string> colnames = bkgdparamws->getColumnNames();
    if (colnames.size() < 2)
    {
      stringstream errss;
      errss << "Input background parameter table workspace " << bkgdparamws->name() << " has only "
            << colnames.size() << " columns, which is fewer than 2 columns as required. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
    else
    {
      if (!(boost::starts_with(colnames[0], "Name") && boost::starts_with(colnames[1], "Value")))
      {
        // Column 0 and 1 must be Name and Value (at least started with)
        stringstream errss;
        errss << "Input parameter table workspace have wrong column definition. "
              << "Column 0 should be Name.  And column 1 should be Value. Current input is: \n";
        for (size_t i = 0; i < 2; ++i)
          errss << "Column " << i << ": " << colnames[0] << "\n";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    }

    // Parse input table workspace to a map.  Valid parameter names must start with A.
    std::map<std::string, double> parmap;
    for (size_t ir = 0; ir < bkgdparamws->rowCount(); ++ir)
    {
      API::TableRow row = bkgdparamws->getRow(ir);
      std::string parname;
      double parvalue;
      row >> parname >> parvalue;

      // Remove extra white spaces
      boost::algorithm::trim(parname);

      if (parname.size() > 0 && (parname[0] == 'A' || parname == "Bkpos"))
      {
        // Insert parameter name starting with A or Bkpos (special case for FullprofPolynomial)
        parmap.insert(std::make_pair(parname, parvalue));
      }
    }

    // Add pair to vector
    bkgdparnames.reserve(parmap.size());
    bkgdorderparams.reserve(parmap.size());

    std::map<std::string, double>::iterator mit;
    for (mit = parmap.begin(); mit != parmap.end(); ++mit)
    {
      std::string parname = mit->first;
      double parvalue = mit->second;
      bkgdparnames.push_back(parname);
      bkgdorderparams.push_back(parvalue);
    }

    // Debug output
    std::stringstream msg;
    msg << "Finished importing background TableWorkspace. " << "Background Order = "
        << bkgdorderparams.size() << ": ";
    for (size_t iod = 0; iod < bkgdorderparams.size(); ++iod)
    {
      msg << bkgdparnames[iod] << " = " << bkgdorderparams[iod] << "; ";
    }
    g_log.information(msg.str());

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create and set up an output TableWorkspace for each individual peaks
   * Parameters include H, K, L, Height, TOF_h, PeakGroup, Chi^2, FitStatus
   * Where chi^2 and fit status are used only in 'CalculateBackground'
   */
  void LeBailFit::exportBraggPeakParameterToTable()
  {
    // Create peaks workspace
    DataObjects::TableWorkspace_sptr peakWS = DataObjects::TableWorkspace_sptr(new DataObjects::TableWorkspace);

    // Add columns to table/peak workspace
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

    // Add each peak in LeBailFunction to peak/table workspace
    for (size_t ipk = 0; ipk < m_lebailFunction->getNumberOfPeaks(); ++ipk)
    {

      // Miller index and peak parameters
      IPowderDiffPeakFunction_sptr peak = m_lebailFunction->getPeak(ipk);

      int h, k, l;
      peak->getMillerIndex(h, k, l);
      double tof_h = peak->centre();
      double height = peak->height();
      double alpha = peak->getPeakParameter("Alpha");
      double beta = peak->getPeakParameter("Beta");
      double sigma2 = peak->getPeakParameter("Sigma2");
      double gamma = peak->getPeakParameter("Gamma");
      double fwhm = peak->fwhm();

      // New row
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

    // Set property
    this->setProperty("OutputPeaksWorkspace", peakWS);

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
    tablews->addColumn("double", "StartValue");
    tablews->addColumn("double", "Diff");

    // 2. Add profile parameter value
    std::map<std::string, Parameter>::iterator paramiter;
    std::map<std::string, double >::iterator opiter;
    for (paramiter = parammap.begin(); paramiter != parammap.end(); ++paramiter)
    {
      std::string parname = paramiter->first;
      if (parname.compare("Height"))
      {
        // Export every parameter except "Height"

        // a) current value
        double parvalue = paramiter->second.curvalue;

        // b) fit or tie?
        char fitortie = 't';
        if (paramiter->second.fit)
        {
          fitortie = 'f';
        }
        std::stringstream ss;
        ss << fitortie;
        std::string fit_tie = ss.str();

        // c) starting value
        opiter = m_origFuncParameters.find(parname);
        double origparvalue = -1.0E100;
        if (opiter != m_origFuncParameters.end())
        {
          origparvalue = opiter->second;
        }
        double diff = origparvalue - parvalue;

        // d. (standard) error
        double paramerror = paramiter->second.fiterror;

        // e. create the row
        double min = paramiter->second.minvalue;
        double max = paramiter->second.maxvalue;
        double step = paramiter->second.stepsize;

        API::TableRow newparam = tablews->appendRow();
        newparam << parname << parvalue << fit_tie << paramerror << min << max << step << origparvalue << diff;
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

    if (m_fitMode == FIT)
    {
      // Do this for FIT mode only
      API::TableRow fitchi2row = tablews->appendRow();
      fitchi2row << "FitChi2" << m_lebailFitChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
      API::TableRow chi2row = tablews->appendRow();
      chi2row << "Chi2" << m_lebailCalChi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    }

    // 4. Add to output peroperty
    setProperty("OutputParameterWorkspace", parameterws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create output data workspace
   * Basic spectra list:
   * (0) original data
   * (1) fitted data
   * (2) difference
   * (3) fitted pattern w/o background
   * (4) background (being fitted after peak)
   * (5) calculation based on input only (no fit)
   * (6) background (input)
   * (7) original data with background removed;
   * (8) Smoothed background
   * In mode of CALCULATION
   * (9+) One spectrum for each peak
   */
  void LeBailFit::createOutputDataWorkspace()
  {
    // 1. Determine number of output spectra
    size_t nspec = 9;

    if (m_fitMode == CALCULATION)
    {
      bool plotindpeak = this->getProperty("PlotIndividualPeaks");
      if (plotindpeak)
      {
        nspec += m_lebailFunction->getNumberOfPeaks();
        g_log.information() << "Number of peaks to add to output data = " << m_lebailFunction->getNumberOfPeaks() << ".\n";
      }
    }

    // 2. Create workspace2D and set the data to spectrum 0 (common among all)
    size_t nbinx = m_dataWS->readX(m_wsIndex).size();
    size_t nbiny = m_dataWS->readY(m_wsIndex).size();
    m_outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbinx, nbiny));

    // 3. Add values
    //    All X.
    for (size_t i = 0; i < nbinx; ++i)
      for (size_t j = 0; j < m_outputWS->getNumberHistograms(); ++j)
        m_outputWS->dataX(j)[i] = m_dataWS->readX(m_wsIndex)[i];

    //    Observation
    for (size_t i = 0; i < nbiny; ++i)
    {
      m_outputWS->dataY(OBSDATAINDEX)[i] = m_dataWS->readY(m_wsIndex)[i];
      m_outputWS->dataE(OBSDATAINDEX)[i] = m_dataWS->readE(m_wsIndex)[i];
    }

    // 4. Set axis
    m_outputWS->getAxis(0)->setUnit("TOF");

    API::TextAxis* tAxis = 0;
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

    if (m_fitMode == CALCULATION)
    {
      // Set the single peak labels
      for (size_t i = 0; i < (nspec-9); ++i)
      {
        std::stringstream ss;
        ss << "Peak_" << i;
        tAxis->setLabel(9+i, ss.str());
      }
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
    // Set up random walk parameters
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);
    const MantidVec& vecInY = m_dataWS->readY(m_wsIndex);
    size_t numpts = vecInY.size();

    const MantidVec& domain = m_dataWS->readX(m_wsIndex);
    MantidVec vecCalPurePeaks(domain.size(), 0.0);

    //    Strategy and map
    TableWorkspace_sptr mctablews = getProperty("MCSetupWorkspace");
    if (mctablews)
    {
      setupRandomWalkStrategyFromTable(mctablews);
    }
    else
    {
      setupBuiltInRandomWalkStrategy();
    }
    //   Walking style/algorithm
    bool usedrunkenwalk = getProperty("DrunkenWalk");
    if (usedrunkenwalk)
      m_walkStyle = DRUNKENWALK;
    else
      m_walkStyle = RANDOMWALK;
    //    Annealing temperature
    m_Temperature = getProperty("AnnealingTemperature");
    if (m_Temperature < 0)
      m_Temperature = fabs(m_Temperature);
    m_useAnnealing = getProperty("UseAnnealing");
    //    Random seed
    int randomseed = getProperty("RandomSeed");

    //    R-factors used for MC procedure
    Rfactor startR(-DBL_MAX, -DBL_MAX);

    // Process background to make a pure peak spectrum in output workspace
    MantidVec& vecBkgd = m_outputWS->dataY(INPUTBKGDINDEX);
    m_lebailFunction->function(vecBkgd, vecX, false, true);
    MantidVec& dataPurePeak = m_outputWS->dataY(INPUTPUREPEAKINDEX);
    transform(vecInY.begin(), vecInY.end(), vecBkgd.begin(), dataPurePeak.begin(), ::minus<double>());

    // Calcualte starting Rwp and etc
    const MantidVec& vecPurePeak = m_outputWS->readY(INPUTPUREPEAKINDEX);

    map<string, double> pardblmap = convertToDoubleMap(parammap);
    m_lebailFunction->setProfileParameterValues(pardblmap);
    bool startvaluevalid = calculateDiffractionPattern(vecX, vecPurePeak, false, false, vecBkgd, vecCalPurePeaks, startR);
    if (!startvaluevalid)
    {
      // Throw exception if starting values are not valid for all
      throw runtime_error("Starting value of instrument profile parameters can generate peaks with"
                          " unphyiscal parameters values.");
    }

    doMarkovChain(parammap, vecX, vecPurePeak, vecBkgd, maxcycles, startR, randomseed);

    // 5. Sum up: retrieve the best result from class variable: m_bestParameters
    Rfactor finalR(-DBL_MAX, -DBL_MAX);
    map<string, double> bestparams = convertToDoubleMap(m_bestParameters);
    m_lebailFunction->setProfileParameterValues(bestparams);
    calculateDiffractionPattern(vecX, vecPurePeak, false, false, vecBkgd, vecCalPurePeaks, finalR);

    MantidVec& vecCalY = m_outputWS->dataY(CALDATAINDEX);
    MantidVec& vecDiff = m_outputWS->dataY(DATADIFFINDEX);
    MantidVec& vecCalPurePeak = m_outputWS->dataY(CALPUREPEAKINDEX);
    MantidVec& vecCalBkgd = m_outputWS->dataY(CALBKGDINDEX);
    for (size_t i = 0; i < numpts; ++i)
    {
      // Calculated (refined) data
      vecCalY[i] = vecCalPurePeaks[i] + vecBkgd[i];
      // Diff
      vecDiff[i] = vecInY[i] - vecCalY[i];
      // Calcualted without background (pure peaks)
      vecCalPurePeak[i] = vecCalPurePeaks[i];
      // Different between calculated peaks and raw data
      vecCalBkgd[i] = vecInY[i] - vecCalPurePeaks[i];
    }

    // c) Apply the best parameters to param
    applyParameterValues(m_bestParameters, parammap);
    Parameter par_rwp;
    par_rwp.name = "Rwp";
    par_rwp.curvalue = m_bestRwp;
    parammap["Rwp"] = par_rwp;

    return;
  } // Main Exec MC


  //----------------------------------------------------------------------------------------------
  /** Work on Markov chain to 'solve' LeBail function
    */
  void LeBailFit::doMarkovChain(const map<string, Parameter>& parammap, const vector<double> &vecX,
                                const vector<double>& vecPurePeak, const vector<double>& vecBkgd,
                                size_t maxcycles, const Rfactor& startR, int randomseed)
  {
    // Rfactors in loop
    Rfactor currR(-DBL_MAX, -DBL_MAX), newR(-DBL_MAX, -DBL_MAX);
    // parameter map for newly proposed values
    map<string, Parameter> mapCurrParameter = parammap;
    map<string, Parameter> newparammap = mapCurrParameter;
    // Output vector from
    MantidVec veccalpurepeaks(vecX.size(), 0.0);

    // Set starting parameters
    currR = startR;
    m_bestRwp = currR.Rwp + EPSILON;
    m_bestRp = currR.Rp + EPSILON;
    bookKeepBestMCResult(mapCurrParameter, vecBkgd, currR, 0);

    g_log.notice() << "[MC-Start] Random-walk Starting Rwp = " << currR.Rwp
                   << ", Rp = " << currR.Rp << "\n";

    // Random walk loops
    // generate some MC trace structure
    vector<double> vecIndex(maxcycles+1);
    vector<Rfactor> vecR(maxcycles+1);
    size_t numinvalidmoves = 0;
    size_t numacceptance = 0;
    bool prevcyclebetterR = true;

    // Annealing record
    int numRecentAcceptance = 0;
    int numRecentSteps = 0;
    int numConsecutiveInvalid = 0;

    // FIXME - It seems that annealing cannot work together with reset invalid if in the region of invalid.
    //         Need to write down the procedure to think it over.
    int numMaxConsecutiveInvalid = 5;
    int m_numStepsCheckTemperature = 10;

    // Loop start
    srand(randomseed);

    for (size_t icycle = 1; icycle <= maxcycles; ++icycle)
    {
      // Refine parameters (for all parameters in turn) to data with background removed
      for (map<int, vector<string> >::iterator giter = m_MCGroups.begin(); giter != m_MCGroups.end(); ++giter)
      {
        // Propose new value for ONE AND ONLY ONE Monte Carlo parameter group
        /*
        int igroup = giter->first; // group id
        g_log.debug() << "BigTrouble: Group " << igroup << "\n";
        */
        bool hasnewvalues = proposeNewValues(giter->second, currR, mapCurrParameter, newparammap,
                                             prevcyclebetterR);

        if (!hasnewvalues)
        {
          // No parameter to have value updated in this MC group.  Skip evaluation of LeBail function.
          continue;
        }

        // Evaluate LeBail function
        map<string, double> newpardblmap = convertToDoubleMap(newparammap);
        m_lebailFunction->setProfileParameterValues(newpardblmap);
        bool validparams = calculateDiffractionPattern(vecX, vecPurePeak, false, false, vecBkgd,
                                                       veccalpurepeaks, newR);
        g_log.debug() << "[Calculation] Rwp = " << newR.Rwp << ", Rp = " << newR.Rp << ".\n";

        // Determine whether to take the change or not
        bool acceptchange;
        if (!validparams)
        {
          ++ numinvalidmoves;
          acceptchange = false;
          prevcyclebetterR = false;
          ++ numConsecutiveInvalid;
        }
        else
        {
          acceptchange = acceptOrDeny(currR, newR);

          if (newR.Rwp < currR.Rwp)
            prevcyclebetterR = true;
          else
            prevcyclebetterR = false;
        }

        g_log.debug() << "[DBx317] Step " << icycle << ": New Rwp = " << setprecision(10)
                      << newR.Rwp << ", Rp = " << setprecision(5) << newR.Rp
                      << "; Accepted = " << acceptchange << "; Proposed parameters valid ="
                      << validparams << "\n";

        // Apply change and book keeping
        if (acceptchange)
        {
          // Apply the change to current
          applyParameterValues(newparammap, mapCurrParameter);
          currR = newR;

          // All tim ebest
          // FIXME - [RPRWP] Use Rp now
          if (currR.Rwp < m_bestRwp)
          {
            // Book keep the best
            bookKeepBestMCResult(mapCurrParameter, vecBkgd, currR, icycle);
          }
          // FIXME - After determining to use Rp or Rwp, this should be got into bookKeepBestMCResult
          if (currR.Rp < m_bestRp)
            m_bestRp = currR.Rp;
          if (currR.Rwp < m_bestRwp)
            m_bestRwp = currR.Rwp;

          // Statistic
          ++ numacceptance;
          ++ numRecentAcceptance;
        }
        ++ numRecentSteps;

        // Annealing or start over
        if (numConsecutiveInvalid >= numMaxConsecutiveInvalid)
        {
          // Exceeds the limit of consecutive invalid proposed new values.  Start over
          mapCurrParameter = m_bestParameters;

          // Reset counters
          numConsecutiveInvalid = 0;
          numRecentAcceptance = 0;
          numRecentSteps = 0;

        }
        else if (m_useAnnealing)
        {
          // Annealing: change temperature to tune the acceptance rate
          // FIXME : Here are some magic numbers
          if (numRecentSteps == m_numStepsCheckTemperature)
          {
            // i. Change temperature
            if (numRecentAcceptance <= 2)
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
      // FIXME - [RPRWP] Use Rp now
      if (currR.Rwp < m_bestRwp)
      {
        // FIXME - Fit background is disabled at this moment
        // fitBackground(m_wsIndex, domainB, valuesB, background);
      }

      // vi. Record some information
      vecIndex[icycle] = static_cast<double>(icycle);
      if (currR.Rwp < 1.0E5)
        vecR[icycle] = currR;
      else
      {
        Rfactor dum(-1, -1);
        vecR[icycle] = dum;
      }

      // vii. progress
      if (icycle%10 == 0)
        progress(double(icycle)/double(maxcycles));

    } // ENDFOR MC Cycles

    progress(1.0);

    // a) Summary output
    g_log.notice() << "[SUMMARY] Random-walk R-factor:  Best step @ " << m_bestMCStep
                   << ", Acceptance ratio = " << double(numacceptance)/double(maxcycles*m_numMCGroups) << ".\n"
                   << "Rwp: Starting = " << startR.Rwp << ", Best = " << m_bestRwp << ", Ending = " << currR.Rwp << "\n"
                   << "Rp : Starting = " << startR.Rp  << ", Best = " << m_bestRp  << ", Ending = " << currR.Rp  << "\n";


    map<string,Parameter>::iterator mapiter;
    for (mapiter = mapCurrParameter.begin(); mapiter != mapCurrParameter.end(); ++mapiter)
    {
      Parameter& param = mapiter->second;
      if (param.fit)
      {
        g_log.notice() << setw(10) << param.name << "\t: Average Stepsize = " << setw(10) << setprecision(5)
                       << param.sumstepsize/double(maxcycles)
                       << ", Max Step Size = " << setw(10) << setprecision(5) << param.maxabsstepsize
                       << ", Number of Positive Move = " << setw(4) << param.numpositivemove
                       << ", Number of Negative Move = " << setw(4) << param.numnegativemove
                       << ", Number of No Move = " << setw(4) << param.numnomove
                       << ", Minimum tried value = " << setw(4) << param.minrecordvalue
                       << ", Maximum tried value = " << setw(4) << param.maxrecordvalue << "\n";
      }
    }
    g_log.notice() << "Number of invalid proposed moves = " << numinvalidmoves << "\n";

    // b) Export trace of R
    stringstream filenamess;
    filenamess << "r_trace_" << vecR.size() << ".dat";
    writeRfactorsToFile(vecIndex, vecR, filenamess.str());
  }


  //----------------------------------------------------------------------------------------------
  /** Set up Monte Carlo random walk strategy
    * @param tablews :: TableWorkspace containing the Monte Carlo setup
   */
  void LeBailFit::setupRandomWalkStrategyFromTable(DataObjects::TableWorkspace_sptr tablews)
  {
    g_log.information("Set up random walk strategy from table.");

    // Scan the table
    size_t numrows = tablews->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      // 1. Get a row and pass out
      TableRow temprow = tablews->getRow(i);
      string parname;
      double a0, a1;
      int nonnegative, group;

      temprow >> parname >> a0 >> a1 >> nonnegative >> group;

      // 2. MC group
      map<int, vector<string> >::iterator giter;
      giter = m_MCGroups.find(group);
      if (giter != m_MCGroups.end())
      {
        giter->second.push_back(parname);
      }
      else
      {
        // First instance in the new group.
        vector<string> newpars;
        newpars.push_back(parname);
        m_MCGroups.insert(make_pair(group, newpars));
      }

      // 3. Set up MC parameters, A0, A1, non-negative
      map<string, Parameter>::iterator piter = m_funcParameters.find(parname);
      if (piter != m_funcParameters.end())
      {
        piter->second.mcA0 = a0;
        piter->second.mcA1 = a1;
        piter->second.nonnegative = (nonnegative != 0);
      }
    }

    m_numMCGroups = m_MCGroups.size();

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
  /** Set up Monte Carlo random walk strategy
   */
  void LeBailFit::setupBuiltInRandomWalkStrategy()
  {
    g_log.information("Set up random walk strategy from build-in. ");

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
    m_MCGroups.insert(make_pair(0, geomparams));

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
    m_MCGroups.insert(make_pair(1, alphs));

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
    m_MCGroups.insert(make_pair(2, betas));

    dboutss << "Beta parameters";
    for (size_t i = 0; i < betas.size(); ++i)
      dboutss << betas[i] << "\t\t";
    dboutss << "\n";

    // d. Sig
    vector<string> sigs;
    addParameterToMCMinimize(sigs, "Sig0");
    addParameterToMCMinimize(sigs, "Sig1");
    addParameterToMCMinimize(sigs, "Sig2");
    m_MCGroups.insert(make_pair(3, sigs));

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
    m_funcParameters["Width"].mcA1 = 0.1;
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
   *  (1) The calculation will be cased on vectors.
   *  (2) m_lebailFunction will NOT be used;
   *  (3) background will not be calculated.
   *
   * @param vecX :: vector of X
   * @param vecY ::  vector of Y (may be raw data or pure peak data/background removed)
   * @param inputraw ::  True if vecY is raw data. Otherwise, with background removed
   * @param outputwithbkgd :: output vector (values) should include background values
   * @param vecBkgd:: vector of background values (input)
   * @param values :: (output) function values, i.e., summation of all peaks and background in option
   * @param rfactor:  R-factor (Rwp and Rp) as output
   *
   * @return :: boolean value.  whether all the peaks' parameters are physical.
   */
  bool LeBailFit::calculateDiffractionPattern(const MantidVec& vecX, const MantidVec &vecY,
                                              bool inputraw, bool outputwithbkgd,
                                              const MantidVec& vecBkgd,  MantidVec& values,
                                              Rfactor& rfactor)
  {
    vector<double> veccalbkgd;

    // Examine whether all peaks are valid
    double maxfwhm = vecX.back()-vecX.front();
    bool peaksvalid = m_lebailFunction->isParameterValid(maxfwhm);

    // If not valid, then return error message
    if (!peaksvalid)
    {
      g_log.information() << "Proposed new instrument profile values cause peak(s) to have "
                          << "unphysical parameter values.\n";
      rfactor = badR;
      return false;
    }

    // Calculate peaks' height
    if (inputraw)
    {
      // Remove background
      vector<double> vecPureY(vecY.size(), 0.);
      if (vecBkgd.size() == vecY.size())
      {
        // Use input background
        g_log.information() << "Calculate diffraction pattern from raw and input background vector. " << ".\n";
        ::transform(vecY.begin(), vecY.end(), vecBkgd.begin(), vecPureY.begin(), ::minus<double>());
      }
      else
      {
        // Calculate background
        g_log.information() << "Calculate diffraction pattern from input data and newly calculated background. " << ".\n";
        veccalbkgd.assign(vecY.size(), 0.);
        m_lebailFunction->function(veccalbkgd, vecX, false, true);
        ::transform(vecY.begin(), vecY.end(), veccalbkgd.begin(), vecPureY.begin(), ::minus<double>());
      }

      // Calculate peak intensity
      peaksvalid = m_lebailFunction->calculatePeaksIntensities(vecX, vecPureY, values);
    } // [input is raw]
    else
    {
      // Calculate peaks intensities
      g_log.debug() << "Calculate diffraction pattern from input data with background removed. " << ".\n";
      peaksvalid = m_lebailFunction->calculatePeaksIntensities(vecX, vecY, values);
    }

    // Calculate Le Bail function
    if (values.size() != vecY.size())
    {
      g_log.error() << "Input/output vector 'values' has a wrong size = " << values.size()
                    << ".  Resize it to " << vecY.size() << ".\n";
      throw runtime_error("Impossible...");
    }

    // Integrated with background if required
    if (outputwithbkgd)
    {
      if (vecBkgd.size() == vecY.size())
      {
        ::transform(values.begin(), values.end(), vecBkgd.begin(), values.begin(), ::plus<double>());
      }
      else
      {
        if (veccalbkgd.size() == 0)
          throw runtime_error("Programming logic error.");
        ::transform(values.begin(), values.end(), veccalbkgd.begin(), values.begin(), ::plus<double>());
      }
    }

    // Calculate Rwp
    if (outputwithbkgd)
    {
      rfactor = getRFactor(m_dataWS->readY(m_wsIndex), values, m_dataWS->readE(m_wsIndex));
    }
    else
    {
      vector<double> caldata(values.size(), 0.0);
      if (vecBkgd.size() == vecY.size())
      {
        // Use input background vector
        std::transform(values.begin(), values.end(), vecBkgd.begin(), caldata.begin(), std::plus<double>());
      }
      else
      {
        // Re-calculate background
        if (veccalbkgd.size() == 0)
          throw runtime_error("Programming logic error (2). ");
        std::transform(values.begin(), values.end(), veccalbkgd.begin(), caldata.begin(), std::plus<double>());
      }
      rfactor = getRFactor(m_dataWS->readY(m_wsIndex), caldata, m_dataWS->readE(m_wsIndex));
    }

    if (!peaksvalid)
    {
      g_log.information("LeBailFit: Some peaks have unphysical peak value. ");
      rfactor = badR;
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Propose new parameters
    * @param mcgroup:  monte carlo group
    * @param r: R factor (Rp, Rwp)
    * @param curparammap:  current map of Parameters whose values are used for propose new values
    * @param newparammap:  map of Parameters hold new values
    * @param prevBetterRwp: boolean.  true if previously proposed value resulted in a better Rwp
    *
    * Return: Boolean to indicate whether there is any parameter that have proposed new values in
    *         this group
    */
  bool LeBailFit::proposeNewValues(vector<string> mcgroup, Rfactor r, map<string, Parameter>& curparammap,
                                    map<string, Parameter>& newparammap, bool prevBetterRwp)
  {
    // TODO: Study the possibility to merge curparammap and newparammap

    // Initialize some flags
    bool anyparamtorefine = false;

    // Find out parameters to refine in this step/MC group
    g_log.debug() << "Parameter Number In Group = " << mcgroup.size() << "\n";
    for (size_t i = 0; i < mcgroup.size(); ++i)
    {
      // Find out the i-th parameter to be refined or not
      string paramname = mcgroup[i];
      map<string, Parameter>::iterator mapiter = curparammap.find(paramname);
      if (mapiter == curparammap.end())
      {
        stringstream errmsg;
        errmsg << "Parameter to update (" << paramname << ") is not in the pool of parameters to get updated. "
               << ".\n"
               << "Number of parameters to update in this group = " << curparammap.size() << ".  They are ";
        for (mapiter = curparammap.begin(); mapiter != curparammap.end(); ++mapiter)
        {
          errmsg << mapiter->first << ", ";
        }
        g_log.error(errmsg.str());
        throw runtime_error(errmsg.str());
      }
      Parameter& param = mapiter->second;

      if (param.fit)
        anyparamtorefine = true;
      else
        continue;

      // Pick a random number between -1 and 1 and calculate step size
      double randomnumber = 2*static_cast<double>(rand())/static_cast<double>(RAND_MAX) - 1.0;

      // FIXME - [RPRWP] Try using Rp this time.
      double weight = r.Rwp;
      if (weight > 1.0)
        weight = 1.0;
      double stepsize = m_dampingFactor * weight * (param.curvalue * param.mcA1 + param.mcA0) * randomnumber;
      if (fabs(stepsize) > 0.5*(param.maxvalue - param.minvalue))
        stepsize = fabs(stepsize)/stepsize * 0.5 * (param.maxvalue - param.minvalue);

      // Direction of new value: drunk walk or random walk
      double newvalue;
      if (m_walkStyle == RANDOMWALK)
      {
        // Random walk.  No preference on direction
        newvalue = param.curvalue + stepsize;
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
        g_log.debug() << "[TestRandom] random = " << randirint << "\n";

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

        newvalue = param.curvalue + stepsize;
      }
      else
      {
        newvalue = DBL_MAX;
        throw runtime_error("Unrecoganized walk style. ");
      }

      // Restriction on the new value: non-negative
      if (param.nonnegative && newvalue < 0)
      {
        // If not allowed to be negative
        newvalue = fabs(newvalue);
      }

      // Restriction on the new value: keep the new value in the boundary
      if (newvalue < param.minvalue)
      {
        int toss = rand()%2;
        double direction = -1.0;
        newvalue = limitProposedValueInBound(param, newvalue, direction, toss);
      }
      else if (newvalue > param.maxvalue)
      {
        int toss = rand()%2;
        double direction = 1.0;
        newvalue = limitProposedValueInBound(param, newvalue, direction, toss);
      }

      // Apply to new parameter map
      map<string, Parameter>::iterator newmiter = newparammap.find(paramname);
      if (newmiter == newparammap.end())
        throw runtime_error("New parameter map does not contain parameter that is updated.");
      newmiter->second.curvalue = newvalue;
      g_log.debug() << "[ProposeNewValue] " << paramname << " --> " << newvalue
                    << "; random number = " << randomnumber << "\n";

      // g) record some trace
      Parameter& p = param;
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

      if (newvalue > p.maxrecordvalue)
        p.maxrecordvalue = newvalue;
      else if (newvalue < p.minrecordvalue)
        p.minrecordvalue = newvalue;

      g_log.debug() << "[DBx257] " << paramname << "\t" << "Proposed value = " << setw(15)
                    << newvalue << " (orig = " << param.curvalue << ",  step = "
                    << stepsize << "), totRwp = " << r.Rwp << "\n";
    } // ENDFOR (i): Each parameter in this MC group/step

    return anyparamtorefine;
  }

  //-----------------------------------------------------------------------------------------------
  /** Limit proposed value in the specified boundary
    * @param param     :: Parameter
    * @param newvalue  :: proposed new value that is out of boundary
    * @param direction :: direction of parameter moved.  -1 for lower.  1 for upper
    * @param choice    :: option for various method  0: half distance.  1: periodic / reflection
    *                     based on boundary
    *
    * @return :: new value in boundary
    */
  double LeBailFit::limitProposedValueInBound(Parameter param, double newvalue, double direction, int choice)
  {
    if (choice == 0)
    {
      // Half distance
      if (direction > 0)
      {
        newvalue = (param.maxvalue - param.curvalue)*0.5 + param.curvalue;
      }
      else
      {
        newvalue = param.minvalue + 0.5 * (param.curvalue - param.minvalue);
      }
    }
    else
    {
      double deltaX = param.maxvalue-param.minvalue;

      if (deltaX < NOBOUNDARYLIMIT)
      {
        choice = 1;  // periodic
      }
      else
      {
        choice = 2;  // reflection
      }

      if (choice == 1)
      {
        // Periodic boundary
        if (direction > 0)
        {
          // newvalue = param.minvalue + (newvalue - param.maxvalue) % deltaX;
          double dval = (newvalue - param.maxvalue)/deltaX;
          newvalue = param.minvalue + deltaX * (dval - floor(dval));
        }
        else
        {
          // newvalue = param.maxvalue - (param.minvalue - newvalue) % deltaX;
          double dval = (param.minvalue - newvalue)/deltaX;
          newvalue = param.maxvalue - deltaX * (dval - floor(dval));
        }
      }
      else
      {
        // Reflective boundary
        if (direction > 0)
        {
          newvalue = param.maxvalue - (newvalue - param.maxvalue);
        }
        else
        {
          newvalue = param.minvalue + (param.maxvalue - newvalue);
        }
      }
    }

    return newvalue;
  }

  //-----------------------------------------------------------------------------------------------
  /** Determine whether the proposed value should be accepted or denied
    * @param currR:  current R-factor Rwp
    * @param newR:  R-factor of function whose parameters' values are the proposed.
    */
  bool LeBailFit::acceptOrDeny(Rfactor currR, Rfactor newR)
  {
    bool accept;

    // FIXME - [RPRWP] Using Rp for peak fitting
    double new_goodness = newR.Rwp;
    double cur_goodness = currR.Rwp;

    if (new_goodness < cur_goodness)
    {
      // Lower Rwp.  Take the change
      accept = true;
    }
    else if (new_goodness > 1.0-1.0E-9)
    {
      // Too high
      g_log.debug() << "Goodness > " << 1.0-1.0E-9 << ".  Reject!" << ".\n";
      accept = false;
    }
    else
    {
      // Higher Rwp/Rp. Take a chance to accept
      double dice = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
      g_log.debug() << "[TestRandom] dice " << dice << "\n";
      double bar = exp(-(new_goodness-cur_goodness)/(cur_goodness*m_Temperature));
      // double bar = exp(-(newrwp-currwp)/m_bestRwp);
      // g_log.debug() << "[DBx329] Bar = " << bar << ", Dice = " << dice << "\n";
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
  /** Book keep the (sopposed) best MC result including
    * a) best MC step, Rp, Rwp
    * b) parameter values of these
    * @param parammap:  map of Parameters to book keep with
    * @param bkgddata:  background data to book keep with
    * @param rfactor :: R-factor (Rwp and Rp)
    * @param istep:     current MC step to be recorded
   */
  void LeBailFit::bookKeepBestMCResult(map<string, Parameter> parammap, const vector<double>& bkgddata, Rfactor rfactor, size_t istep)
  {
    // TODO : [RPRWP] Here is a metric of goodness of it.
    double goodness = rfactor.Rwp;
    bool better = goodness < m_bestRwp;

    if (better)
    {
      // In case obtain the best solution so far

      // a) Record goodness and step
      m_bestRwp = rfactor.Rwp;
      m_bestRp = rfactor.Rp;
      m_bestMCStep = istep;

      // b) Record parameters
      if (m_bestParameters.size() == 0)
      {
        // If not be initialized, initialize it!
        m_bestParameters = parammap;
      }
      else
      {
        // in case initialized, copy the value over
        applyParameterValues(parammap, m_bestParameters);
      }

      // c) Background
      m_bestBackgroundData = bkgddata;
    }
    else
    {
      // In code calling this function, it should be better always.
      g_log.warning("[Book keep best MC result] Shouldn't be here as it is found that it is not the best solution ");
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

      tgtmapiter->second.curvalue = srcparam.curvalue;
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

    MantidVec& vecSmoothBkgd = m_outputWS->dataY(SMOOTHEDBKGDINDEX);

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
    throw runtime_error("Need to figure out how to deal with this part!");

  //  for (size_t ipk = 0; ipk < m_lebailFunction->getNumberOfPeaks(); ++ipk)
  //  {

      /* Below are original code for modifying from
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
      double height = thispeak->height();
      if (height > m_minimumPeakHeight)
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
        }*/
 //     }

    /*}

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

    return;*/
  }

  //----------------------------------------------------------------------------------------------
  /** Smooth background by fitting the background to specified background function
    * Algorithm: 1. calculate background by removing calculated peaks from raw data
    *            2. fit background by a specified background function.
    * @param wsindex  :  raw data's workspace index
    * @param domain      domain of X's
    * @param peakdata:   pattern of pure peaks
    * @param background: output of smoothed background
    */
  void LeBailFit::smoothBackgroundAnalytical(size_t wsindex, FunctionDomain1DVector domain,
                                             FunctionValues peakdata, vector<double>& background)
  {
    // FIXME - This method may not be a good solution.
    // TODO  - Create a new ticket to use the algorithm in ProcessBackground here.

    UNUSED_ARG(wsindex);
    UNUSED_ARG(peakdata);
    UNUSED_ARG(domain);
    UNUSED_ARG(background);

    throw runtime_error("Need to re-consider this method.");

    /* Below is the original code to modifying from
    // 1. Make data ready
    MantidVec& vecData = m_dataWS->dataY(wsindex);
    MantidVec& vecFitBkgd = m_outputWS->dataY(CALBKGDINDEX);
    MantidVec& vecFitBkgdErr = m_outputWS->dataE(CALBKGDINDEX);
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
    calalg->setProperty("WorkspaceIndex", CALDATAINDEX);
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
    */

    return;
  }

  //----------------------------------------------------------------------------------------------
  /// Convert a map of Parameter to a map of double
  std::map<std::string, double> LeBailFit::convertToDoubleMap(std::map<std::string, Parameter>& inmap)
  {
    std::map<std::string, double> outmap;
    std::map<std::string, Parameter>::iterator miter;
    for (miter = inmap.begin(); miter != inmap.end(); ++miter)
    {
      outmap.insert(std::make_pair(miter->first, miter->second.curvalue));
    }

    return outmap;
  }


  // ============================ External Auxiliary Functions   =================================

  /** Write a set of (XY) data to a column file
    */
  void writeRfactorsToFile(vector<double> vecX, vector<Rfactor> vecR, string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    for (size_t i = 0; i < vecX.size(); ++i)
      ofile << setw(15) << setprecision(5) << vecX[i]
            << setw(15) << setprecision(5) << vecR[i].Rwp
            << setw(15) << setprecision(5) << vecR[i].Rp
            << "\n";

    ofile.close();

    return;
  }


} // namespace CurveFitting
} // namespace Mantid
