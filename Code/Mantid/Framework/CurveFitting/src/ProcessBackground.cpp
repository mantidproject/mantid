#include "MantidCurveFitting/ProcessBackground.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/Chebyshev.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/TableRow.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

using namespace std;

namespace Mantid {
namespace CurveFitting {

DECLARE_ALGORITHM(ProcessBackground)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProcessBackground::ProcessBackground() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ProcessBackground::~ProcessBackground() {}

//----------------------------------------------------------------------------------------------
/** Define parameters
 */
void ProcessBackground::init() {
  // Input and output Workspace
  declareProperty(
      new WorkspaceProperty<Workspace2D>("InputWorkspace", "Anonymous",
                                         Direction::Input),
      "Name of the output workspace containing the processed background.");

  // Workspace index
  declareProperty("WorkspaceIndex", 0,
                  "Workspace index for the input workspaces.");

  // Output workspace
  declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "",
                                                     Direction::Output),
                  "Output workspace containing processed background");

  // Function Options
  std::vector<std::string> options;
  options.push_back("SelectBackgroundPoints");
  options.push_back("RemovePeaks");
  options.push_back("DeleteRegion");
  options.push_back("AddRegion");

  auto validator = boost::make_shared<Kernel::StringListValidator>(options);
  declareProperty("Options", "RemovePeaks", validator,
                  "Name of the functionality realized by this algorithm.");

  // Boundary
  declareProperty("LowerBound", Mantid::EMPTY_DBL(),
                  "Lower boundary of the data to have background processed.");
  declareProperty("UpperBound", Mantid::EMPTY_DBL(),
                  "Upper boundary of the data to have background processed.");

  auto refwsprop = new WorkspaceProperty<Workspace2D>(
      "ReferenceWorkspace", "", Direction::Input, PropertyMode::Optional);
  declareProperty(refwsprop, "Name of the workspace containing the data "
                             "required by function AddRegion.");
  setPropertySettings(
      "ReferenceWorkspace",
      new VisibleWhenProperty("Options", IS_EQUAL_TO, "AddRegion"));

  // Optional Function Type
  std::vector<std::string> bkgdtype;
  bkgdtype.push_back("Polynomial");
  bkgdtype.push_back("Chebyshev");
  // bkgdtype.push_back("FullprofPolynomial");
  auto bkgdvalidator =
      boost::make_shared<Kernel::StringListValidator>(bkgdtype);
  declareProperty(
      "BackgroundType", "Polynomial", bkgdvalidator,
      "Type of the background. Options include Polynomial and Chebyshev.");
  setPropertySettings("BackgroundType",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  vector<string> funcoptions;
  funcoptions.push_back("N/A");
  funcoptions.push_back("FitGivenDataPoints");
  funcoptions.push_back("UserFunction");
  auto fovalidator = boost::make_shared<StringListValidator>(funcoptions);
  declareProperty("SelectionMode", "N/A", fovalidator,
                  "If choise is UserFunction, background will be selected by "
                  "an input background "
                  "function.  Otherwise, background function will be fitted "
                  "from user's input data points.");
  setPropertySettings("SelectionMode",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  declareProperty("BackgroundOrder", 0,
                  "Order of polynomial or chebyshev background. ");
  setPropertySettings("BackgroundOrder",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));
  setPropertySettings("BackgroundOrder",
                      new VisibleWhenProperty("SelectionMode", IS_EQUAL_TO,
                                              "FitGivenDataPoints"));

  // User input background points for "SelectBackground"
  auto arrayproperty = new Kernel::ArrayProperty<double>("BackgroundPoints");
  declareProperty(arrayproperty, "Vector of doubles, each of which is the "
                                 "X-axis value of the background point "
                                 "selected by user.");
  setPropertySettings("BackgroundPoints",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));
  setPropertySettings("BackgroundPoints",
                      new VisibleWhenProperty("SelectionMode", IS_EQUAL_TO,
                                              "FitGivenDataPoints"));

  declareProperty(new WorkspaceProperty<TableWorkspace>(
                      "BackgroundTableWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of the table workspace containing background "
                  "parameters for mode SelectBackgroundPoints.");
  setPropertySettings("BackgroundTableWorkspace",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));
  setPropertySettings(
      "BackgroundTableWorkspace",
      new VisibleWhenProperty("SelectionMode", IS_EQUAL_TO, "UserFunction"));

  // Mode to select background
  vector<string> pointsselectmode;
  pointsselectmode.push_back("All Background Points");
  pointsselectmode.push_back("Input Background Points Only");
  auto modevalidator =
      boost::make_shared<StringListValidator>(pointsselectmode);
  declareProperty("BackgroundPointSelectMode", "All Background Points",
                  modevalidator, "Mode to select background points. ");
  setPropertySettings("BackgroundPointSelectMode",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));
  setPropertySettings("BackgroundPointSelectMode",
                      new VisibleWhenProperty("SelectionMode", IS_EQUAL_TO,
                                              "FitGivenDataPoints"));

  // Background tolerance
  declareProperty("NoiseTolerance", 1.0, "Tolerance of noise range. ");
  setPropertySettings("NoiseTolerance",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Background tolerance
  declareProperty("NegativeNoiseTolerance", EMPTY_DBL(),
                  "Tolerance of noise range for negative number. ");
  setPropertySettings("NegativeNoiseTolerance",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Optional output workspace
  declareProperty(new WorkspaceProperty<Workspace2D>(
                      "UserBackgroundWorkspace", "_dummy01", Direction::Output),
                  "Output workspace containing fitted background from points "
                  "specified by users.");
  setPropertySettings("UserBackgroundWorkspace",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Optional output workspace
  declareProperty(
      new WorkspaceProperty<TableWorkspace>(
          "OutputBackgroundParameterWorkspace", "_dummy02", Direction::Output),
      "Output parameter table workspace containing the background fitting "
      "result. ");
  setPropertySettings("OutputBackgroundParameterWorkspace",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Output background type.
  std::vector<std::string> outbkgdtype;
  outbkgdtype.push_back("Polynomial");
  outbkgdtype.push_back("Chebyshev");
  auto outbkgdvalidator =
      boost::make_shared<Kernel::StringListValidator>(bkgdtype);
  declareProperty("OutputBackgroundType", "Polynomial", outbkgdvalidator,
                  "Type of background to fit with selected background points.");
  setPropertySettings("OutputBackgroundType",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Output background type.
  declareProperty(
      "OutputBackgroundOrder", 6,
      "Order of background to fit with selected background points.");
  setPropertySettings("OutputBackgroundOrder",
                      new VisibleWhenProperty("Options", IS_EQUAL_TO,
                                              "SelectBackgroundPoints"));

  // Peak table workspac for "RemovePeaks"
  declareProperty(new WorkspaceProperty<TableWorkspace>(
                      "BraggPeakTableWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of table workspace containing peaks' parameters. ");
  setPropertySettings(
      "BraggPeakTableWorkspace",
      new VisibleWhenProperty("Options", IS_EQUAL_TO, "RemovePeaks"));

  // Number of FWHM to have peak removed
  declareProperty(
      "NumberOfFWHM", 1.0,
      "Number of FWHM to as the peak region to have peak removed. ");
  setPropertySettings(
      "NumberOfFWHM",
      new VisibleWhenProperty("Options", IS_EQUAL_TO, "RemovePeaks"));

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execution function
  */
void ProcessBackground::exec() {
  // Process general properties
  m_dataWS = this->getProperty("InputWorkspace");
  if (!m_dataWS) {
    g_log.error() << "Input Workspace cannot be obtained." << std::endl;
    throw std::invalid_argument("Input Workspace cannot be obtained.");
  }

  m_bkgdType = getPropertyValue("BackgroundType");

  int intemp = getProperty("WorkspaceIndex");
  if (intemp < 0)
    throw std::invalid_argument(
        "WorkspaceIndex is not allowed to be less than 0. ");
  m_wsIndex = size_t(intemp);
  if (m_wsIndex >= static_cast<int>(m_dataWS->getNumberHistograms()))
    throw runtime_error("Workspace index is out of boundary.");

  m_lowerBound = getProperty("LowerBound");
  m_upperBound = getProperty("UpperBound");
  if (isEmpty(m_lowerBound))
    m_lowerBound = m_dataWS->readX(m_wsIndex).front();
  if (isEmpty(m_upperBound))
    m_upperBound = m_dataWS->readX(m_wsIndex).back();

  // 2. Do different work
  std::string option = getProperty("Options");
  if (option.compare("RemovePeaks") == 0) {
    removePeaks();
  } else if (option.compare("DeleteRegion") == 0) {
    deleteRegion();
  } else if (option.compare("AddRegion") == 0) {
    addRegion();
  } else if (option.compare("SelectBackgroundPoints") == 0) {

    selectBkgdPoints();
  } else {
    g_log.error() << "Option " << option << " is not supported. " << std::endl;
    throw std::invalid_argument("Unsupported option. ");
  }

  // 3. Set output
  setProperty("OutputWorkspace", m_outputWS);

  return;
}

//----------------------------------------------------------------------------------------------
/** Set dummy output workspaces to avoid python error for optional workspaces
  */
void ProcessBackground::setupDummyOutputWSes() {
  // Dummy outputs to make it work with python script
  setPropertyValue("UserBackgroundWorkspace", "dummy0");
  Workspace2D_sptr dummyws = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1));
  setProperty("UserBackgroundWorkspace", dummyws);

  setPropertyValue("OutputBackgroundParameterWorkspace", "dummy1");
  TableWorkspace_sptr dummytbws = boost::make_shared<TableWorkspace>();
  setProperty("OutputBackgroundParameterWorkspace", dummytbws);

  return;
}

//----------------------------------------------------------------------------------------------
/** Delete a certain region from input workspace
 */
void ProcessBackground::deleteRegion() {
  // Check boundary
  if (m_lowerBound == Mantid::EMPTY_DBL() ||
      m_upperBound == Mantid::EMPTY_DBL()) {
    throw std::invalid_argument("Using DeleteRegion.  Both LowerBound and "
                                "UpperBound must be specified.");
  }
  if (m_lowerBound >= m_upperBound) {
    throw std::invalid_argument(
        "Lower boundary cannot be equal or larger than upper boundary.");
  }

  // Copy original data and exclude region defined by m_lowerBound and
  // m_upperBound
  const MantidVec &dataX = m_dataWS->readX(0);
  const MantidVec &dataY = m_dataWS->readY(0);
  const MantidVec &dataE = m_dataWS->readE(0);

  std::vector<double> vx, vy, ve;

  for (size_t i = 0; i < dataY.size(); ++i) {
    double xtmp = dataX[i];
    if (xtmp < m_lowerBound || xtmp > m_upperBound) {
      vx.push_back(dataX[i]);
      vy.push_back(dataY[i]);
      ve.push_back(dataE[i]);
    }
  }
  if (dataX.size() > dataY.size()) {
    vx.push_back(dataX.back());
  }

  // Create new workspace
  size_t sizex = vx.size();
  size_t sizey = vy.size();
  API::MatrixWorkspace_sptr mws =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
  m_outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(mws);
  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());

  for (size_t i = 0; i < sizey; ++i) {
    m_outputWS->dataX(0)[i] = vx[i];
    m_outputWS->dataY(0)[i] = vy[i];
    m_outputWS->dataE(0)[i] = ve[i];
  }
  if (sizex > sizey) {
    m_outputWS->dataX(0)[sizex - 1] = vx.back();
  }

  // Set up dummies
  setupDummyOutputWSes();

  return;
}

//----------------------------------------------------------------------------------------------
/** Add a certain region from reference workspace
 */
void ProcessBackground::addRegion() {
  // Check boundary, which are required
  if (m_lowerBound == Mantid::EMPTY_DBL() ||
      m_upperBound == Mantid::EMPTY_DBL()) {
    throw std::invalid_argument(
        "Using AddRegion.  Both LowerBound and UpperBound must be specified.");
  }
  if (m_lowerBound >= m_upperBound) {
    throw std::invalid_argument(
        "Lower boundary cannot be equal or larger than upper boundary.");
  }

  // Copy data to a set of vectors
  const MantidVec &vecX = m_dataWS->readX(0);
  const MantidVec &vecY = m_dataWS->readY(0);
  const MantidVec &vecE = m_dataWS->readE(0);

  std::vector<double> vx, vy, ve;
  for (size_t i = 0; i < vecY.size(); ++i) {
    double xtmp = vecX[i];
    if (xtmp < m_lowerBound || xtmp > m_upperBound) {
      vx.push_back(vecX[i]);
      vy.push_back(vecY[i]);
      ve.push_back(vecE[i]);
    }
  }

  // Histogram
  if (vecX.size() > vecY.size())
    vx.push_back(vecX.back());

  // Get access to reference workspace
  DataObjects::Workspace2D_const_sptr refWS = getProperty("ReferenceWorkspace");
  if (!refWS)
    throw std::invalid_argument("ReferenceWorkspace is not given. ");

  const MantidVec &refX = refWS->dataX(0);
  const MantidVec &refY = refWS->dataY(0);
  const MantidVec &refE = refWS->dataE(0);

  // 4. Insert the value of the reference workspace from lowerBoundary to
  // upperBoundary
  std::vector<double>::const_iterator refiter;
  refiter = std::lower_bound(refX.begin(), refX.end(), m_lowerBound);
  size_t sindex = size_t(refiter - refX.begin());
  refiter = std::lower_bound(refX.begin(), refX.end(), m_upperBound);
  size_t eindex = size_t(refiter - refX.begin());

  for (size_t i = sindex; i < eindex; ++i) {
    double tmpx = refX[i];
    double tmpy = refY[i];
    double tmpe = refE[i];

    // Locate the position of tmpx in the array to be inserted
    std::vector<double>::iterator newit =
        std::lower_bound(vx.begin(), vx.end(), tmpx);
    size_t newindex = size_t(newit - vx.begin());

    // insert tmpx, tmpy, tmpe by iterator
    vx.insert(newit, tmpx);

    newit = vy.begin() + newindex;
    vy.insert(newit, tmpy);

    newit = ve.begin() + newindex;
    ve.insert(newit, tmpe);
  }

  // Check
  for (size_t i = 1; i < vx.size(); ++i) {
    if (vx[i] <= vx[i - 1]) {
      g_log.error()
          << "The vector X with value inserted is not ordered incrementally"
          << std::endl;
      throw std::runtime_error("Build new vector error!");
    }
  }

  // Construct the new Workspace
  m_outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, vx.size(),
                                               vy.size()));
  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());
  for (size_t i = 0; i < vy.size(); ++i) {
    m_outputWS->dataX(0)[i] = vx[i];
    m_outputWS->dataY(0)[i] = vy[i];
    m_outputWS->dataE(0)[i] = ve[i];
  }
  if (vx.size() > vy.size())
    m_outputWS->dataX(0)[vx.size() - 1] = vx.back();

  // Write out dummy output workspaces
  setupDummyOutputWSes();

  return;
}

//----------------------------------------------------------------------------------------------
// Methods for selecting background points
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Main method to select background points
  */
void ProcessBackground::selectBkgdPoints() {
  // Select background points
  string smode = getProperty("SelectionMode");
  if (smode == "FitGivenDataPoints") {
    selectFromGivenXValues();
  } else if (smode == "UserFunction") {
    selectFromGivenFunction();
  } else {
    throw runtime_error("N/A is not supported.");
  }

  // Fit the background points if output backgrond parameter workspace is given
  // explicitly
  string outbkgdparwsname =
      getPropertyValue("OutputBackgroundParameterWorkspace");
  if (outbkgdparwsname.size() > 0 &&
      outbkgdparwsname.compare("_dummy02") != 0) {
    // Will fit the selected background
    string bkgdfunctype = getPropertyValue("OutputBackgroundType");
    fitBackgroundFunction(bkgdfunctype);
  } else {
    setupDummyOutputWSes();
  }

  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());

  return;
}

//----------------------------------------------------------------------------------------------
/** Select background points
  */
void ProcessBackground::selectFromGivenXValues() {
  // Get special input properties
  std::vector<double> bkgdpoints = getProperty("BackgroundPoints");
  string mode = getProperty("BackgroundPointSelectMode");

  // Construct background workspace for fit
  std::vector<double> realx, realy, reale;
  const MantidVec &vecX = m_dataWS->readX(m_wsIndex);
  const MantidVec &vecY = m_dataWS->readY(m_wsIndex);
  const MantidVec &vecE = m_dataWS->readE(m_wsIndex);
  for (size_t i = 0; i < bkgdpoints.size(); ++i) {
    // Data range validation
    double bkgdpoint = bkgdpoints[i];
    if (bkgdpoint < vecX.front()) {
      g_log.warning() << "Input background point " << bkgdpoint
                      << " is out of lower boundary.  "
                      << "Use X[0] = " << vecX.front() << " instead."
                      << "\n";
      bkgdpoint = vecX.front();
    } else if (bkgdpoint > vecX.back()) {
      g_log.warning() << "Input background point " << bkgdpoint
                      << " is out of upper boundary.  Use X[-1] = "
                      << vecX.back() << " instead."
                      << "\n";
      bkgdpoint = vecX.back();
    }

    // Find the index in
    std::vector<double>::const_iterator it;
    it = std::lower_bound(vecX.begin(), vecX.end(), bkgdpoint);
    size_t index = size_t(it - vecX.begin());

    g_log.debug() << "DBx502 Background Points " << i << " Index = " << index
                  << " For TOF = " << bkgdpoints[i] << " in [" << vecX[0]
                  << ", " << vecX.back() << "] "
                  << "\n";

    // Add to list
    realx.push_back(vecX[index]);
    realy.push_back(vecY[index]);
    reale.push_back(vecE[index]);

  } // ENDFOR (i)

  DataObjects::Workspace2D_sptr bkgdWS =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 1,
                                                   realx.size(), realy.size()));
  for (size_t i = 0; i < realx.size(); ++i) {
    bkgdWS->dataX(0)[i] = realx[i];
    bkgdWS->dataY(0)[i] = realy[i];
    bkgdWS->dataE(0)[i] = reale[i];
  }

  // Select background points according to mode
  if (mode.compare("All Background Points") == 0) {
    // Select (possibly) all background points
    m_outputWS = autoBackgroundSelection(bkgdWS);
  } else if (mode.compare("Input Background Points Only") == 0) {
    // Use the input background points only
    m_outputWS = bkgdWS;
  } else {
    stringstream errss;
    errss << "Background select mode " << mode
          << " is not supported by ProcessBackground.";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Select background points via a given background function
  */
void ProcessBackground::selectFromGivenFunction() {
  // Process properties
  BackgroundFunction_sptr bkgdfunc = createBackgroundFunction(m_bkgdType);
  TableWorkspace_sptr bkgdtablews = getProperty("BackgroundTableWorkspace");

  // Set up background function from table
  size_t numrows = bkgdtablews->rowCount();
  map<string, double> parmap;
  for (size_t i = 0; i < numrows; ++i) {
    TableRow row = bkgdtablews->getRow(i);
    string parname;
    double parvalue;
    row >> parname >> parvalue;
    if (parname[0] == 'A')
      parmap.insert(make_pair(parname, parvalue));
  }

  int bkgdorder =
      static_cast<int>(parmap.size() - 1); // A0 - A(n) total n+1 parameters
  bkgdfunc->setAttributeValue("n", bkgdorder);
  for (map<string, double>::iterator mit = parmap.begin(); mit != parmap.end();
       ++mit) {
    string parname = mit->first;
    double parvalue = mit->second;
    bkgdfunc->setParameter(parname, parvalue);
  }

  // Filter out
  m_outputWS = filterForBackground(bkgdfunc);

  return;
}

//----------------------------------------------------------------------------------------------
/** Select background automatically
 */
DataObjects::Workspace2D_sptr
ProcessBackground::autoBackgroundSelection(Workspace2D_sptr bkgdWS) {
  // Get background type and create bakground function
  BackgroundFunction_sptr bkgdfunction = createBackgroundFunction(m_bkgdType);

  int bkgdorder = getProperty("BackgroundOrder");
  if (bkgdorder == 0)
    g_log.warning("(Input) background function order is 0.  It might not be "
                  "able to give a good estimation.");

  bkgdfunction->setAttributeValue("n", bkgdorder);
  bkgdfunction->initialize();

  g_log.information() << "Input background points has "
                      << bkgdWS->readX(0).size() << " data points for fit "
                      << bkgdorder << "-th order " << bkgdfunction->name()
                      << " (background) function" << bkgdfunction->asString()
                      << "\n";

  // Fit input (a few) background pionts to get initial guess
  API::IAlgorithm_sptr fit;
  try {
    fit = this->createChildAlgorithm("Fit", 0.0, 0.2, true);
  } catch (Exception::NotFoundError &) {
    g_log.error() << "Requires CurveFitting library." << std::endl;
    throw;
  }

  double startx = m_lowerBound;
  double endx = m_upperBound;
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
  fit->setProperty("InputWorkspace", bkgdWS);
  fit->setProperty("WorkspaceIndex", 0);
  fit->setProperty("MaxIterations", 500);
  fit->setProperty("StartX", startx);
  fit->setProperty("EndX", endx);
  fit->setProperty("Minimizer", "Levenberg-Marquardt");
  fit->setProperty("CostFunction", "Least squares");

  fit->executeAsChildAlg();

  // Get fit result
  // a) Status
  std::string fitStatus = fit->getProperty("OutputStatus");
  bool allowedfailure = (fitStatus.find("cannot") < fitStatus.size()) &&
                        (fitStatus.find("tolerance") < fitStatus.size());
  if (fitStatus.compare("success") != 0 && !allowedfailure) {
    g_log.error() << "ProcessBackground: Fit Status = " << fitStatus
                  << ".  Not to update fit result" << std::endl;
    throw std::runtime_error("Bad Fit");
  }

  // b) check that chi2 got better
  const double chi2 = fit->getProperty("OutputChi2overDoF");
  g_log.information() << "Fit background: Fit Status = " << fitStatus
                      << ", chi2 = " << chi2 << "\n";

  // Filter and construct for the output workspace
  Workspace2D_sptr outws = filterForBackground(bkgdfunction);

  return outws;
} // END OF FUNCTION

//----------------------------------------------------------------------------------------------
/** Create a background function from input properties
  */
BackgroundFunction_sptr
ProcessBackground::createBackgroundFunction(const string backgroundtype) {
  CurveFitting::BackgroundFunction_sptr bkgdfunction;

  if (backgroundtype.compare("Polynomial") == 0) {
    bkgdfunction =
        boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(
            boost::make_shared<CurveFitting::Polynomial>());
    bkgdfunction->initialize();
  } else if (backgroundtype.compare("Chebyshev") == 0) {
    Chebyshev_sptr cheby = boost::make_shared<CurveFitting::Chebyshev>();
    bkgdfunction =
        boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(cheby);
    bkgdfunction->initialize();

    g_log.debug() << "[D] Chebyshev is set to range " << m_lowerBound << ", "
                  << m_upperBound << "\n";
    bkgdfunction->setAttributeValue("StartX", m_lowerBound);
    bkgdfunction->setAttributeValue("EndX", m_upperBound);
  } else {
    stringstream errss;
    errss << "Background of type " << backgroundtype << " is not supported. ";
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  return bkgdfunction;
}

//----------------------------------------------------------------------------------------------
/** Filter non-background data points out and create a background workspace
  */
Workspace2D_sptr
ProcessBackground::filterForBackground(BackgroundFunction_sptr bkgdfunction) {
  double posnoisetolerance = getProperty("NoiseTolerance");
  double negnoisetolerance = getProperty("NegativeNoiseTolerance");
  if (isEmpty(negnoisetolerance))
    negnoisetolerance = posnoisetolerance;

  // Calcualte theoretical values
  const std::vector<double> x = m_dataWS->readX(m_wsIndex);
  API::FunctionDomain1DVector domain(x);
  API::FunctionValues values(domain);
  bkgdfunction->function(domain, values);

  g_log.information() << "Function used to select background points : "
                      << bkgdfunction->asString() << "\n";

  // Optional output
  string userbkgdwsname = getPropertyValue("UserBackgroundWorkspace");
  if (userbkgdwsname.size() == 0)
    throw runtime_error("In mode SelectBackgroundPoints, "
                        "UserBackgroundWorkspace must be given!");

  size_t sizex = domain.size();
  size_t sizey = values.size();
  MatrixWorkspace_sptr visualws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 4, sizex, sizey));
  for (size_t i = 0; i < sizex; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      visualws->dataX(j)[i] = domain[i];
    }
  }
  for (size_t i = 0; i < sizey; ++i) {
    visualws->dataY(0)[i] = values[i];
    visualws->dataY(1)[i] = m_dataWS->readY(m_wsIndex)[i] - values[i];
    visualws->dataY(2)[i] = posnoisetolerance;
    visualws->dataY(3)[i] = -negnoisetolerance;
  }
  setProperty("UserBackgroundWorkspace", visualws);

  // Filter for background
  std::vector<double> vecx, vecy, vece;
  for (size_t i = 0; i < domain.size(); ++i) {
    // double y = m_dataWS->readY(m_wsIndex)[i];
    // double theoryy = values[i]; y-theoryy
    double purey = visualws->readY(1)[i];
    if (purey < posnoisetolerance && purey > -negnoisetolerance) {
      // Selected
      double x = domain[i];
      double y = m_dataWS->readY(m_wsIndex)[i];
      double e = m_dataWS->readE(m_wsIndex)[i];
      vecx.push_back(x);
      vecy.push_back(y);
      vece.push_back(e);
    }
  }
  g_log.information() << "Found " << vecx.size() << " background points out of "
                      << m_dataWS->readX(m_wsIndex).size()
                      << " total data points. "
                      << "\n";

  // Build new workspace for OutputWorkspace
  size_t nspec = 3;
  Workspace2D_sptr outws =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nspec,
                                                   vecx.size(), vecy.size()));
  for (size_t i = 0; i < vecx.size(); ++i) {
    for (size_t j = 0; j < nspec; ++j)
      outws->dataX(j)[i] = vecx[i];
    outws->dataY(0)[i] = vecy[i];
    outws->dataE(0)[i] = vece[i];
  }

  return outws;
}

//----------------------------------------------------------------------------------------------
/** Fit background function
  */
void ProcessBackground::fitBackgroundFunction(std::string bkgdfunctiontype) {
  // Get background type and create bakground function
  BackgroundFunction_sptr bkgdfunction =
      createBackgroundFunction(bkgdfunctiontype);

  int bkgdorder = getProperty("OutputBackgroundOrder");
  bkgdfunction->setAttributeValue("n", bkgdorder);

  if (bkgdfunctiontype == "Chebyshev") {
    double xmin = m_outputWS->readX(0).front();
    double xmax = m_outputWS->readX(0).back();
    g_log.information() << "Chebyshev Fit range: " << xmin << ", " << xmax
                        << "\n";
    bkgdfunction->setAttributeValue("StartX", xmin);
    bkgdfunction->setAttributeValue("EndX", xmax);
  }

  g_log.information() << "Fit selected background " << bkgdfunctiontype
                      << " to data workspace with "
                      << m_outputWS->getNumberHistograms() << " spectra."
                      << "\n";

  // Fit input (a few) background pionts to get initial guess
  API::IAlgorithm_sptr fit;
  try {
    fit = this->createChildAlgorithm("Fit", 0.9, 1.0, true);
  } catch (Exception::NotFoundError &) {
    g_log.error() << "Requires CurveFitting library." << std::endl;
    throw;
  }

  g_log.information() << "Fitting background function: "
                      << bkgdfunction->asString() << "\n";

  double startx = m_lowerBound;
  double endx = m_upperBound;
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
  fit->setProperty("InputWorkspace", m_outputWS);
  fit->setProperty("WorkspaceIndex", 0);
  fit->setProperty("MaxIterations", 500);
  fit->setProperty("StartX", startx);
  fit->setProperty("EndX", endx);
  fit->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fit->setProperty("CostFunction", "Least squares");

  fit->executeAsChildAlg();

  // Get fit status and chi^2
  std::string fitStatus = fit->getProperty("OutputStatus");
  bool allowedfailure = (fitStatus.find("cannot") < fitStatus.size()) &&
                        (fitStatus.find("tolerance") < fitStatus.size());
  if (fitStatus.compare("success") != 0 && !allowedfailure) {
    g_log.error() << "ProcessBackground: Fit Status = " << fitStatus
                  << ".  Not to update fit result" << std::endl;
    throw std::runtime_error("Bad Fit");
  }

  const double chi2 = fit->getProperty("OutputChi2overDoF");
  g_log.information() << "Fit background: Fit Status = " << fitStatus
                      << ", chi2 = " << chi2 << "\n";

  // Get out the parameter names
  API::IFunction_sptr funcout = fit->getProperty("Function");
  TableWorkspace_sptr outbkgdparws = boost::make_shared<TableWorkspace>();
  outbkgdparws->addColumn("str", "Name");
  outbkgdparws->addColumn("double", "Value");

  TableRow typerow = outbkgdparws->appendRow();
  typerow << bkgdfunctiontype << 0.;

  vector<string> parnames = funcout->getParameterNames();
  size_t nparam = funcout->nParams();
  for (size_t i = 0; i < nparam; ++i) {
    TableRow newrow = outbkgdparws->appendRow();
    newrow << parnames[i] << funcout->getParameter(i);
  }

  TableRow chi2row = outbkgdparws->appendRow();
  chi2row << "Chi-square" << chi2;

  g_log.information() << "Set table workspace (#row = "
                      << outbkgdparws->rowCount()
                      << ") to OutputBackgroundParameterTable. "
                      << "\n";
  setProperty("OutputBackgroundParameterWorkspace", outbkgdparws);

  // Set output workspace
  const MantidVec &vecX = m_outputWS->readX(0);
  const MantidVec &vecY = m_outputWS->readY(0);
  FunctionDomain1DVector domain(vecX);
  FunctionValues values(domain);

  funcout->function(domain, values);

  MantidVec &dataModel = m_outputWS->dataY(1);
  MantidVec &dataDiff = m_outputWS->dataY(2);
  for (size_t i = 0; i < dataModel.size(); ++i) {
    dataModel[i] = values[i];
    dataDiff[i] = vecY[i] - dataModel[i];
  }

  return;
}

//----------------------------------------------------------------------------------------------
// Remove peaks
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Remove peaks within a specified region
 */
void ProcessBackground::removePeaks() {
  // Get input
  TableWorkspace_sptr peaktablews = getProperty("BraggPeakTableWorkspace");
  if (!peaktablews)
    throw runtime_error(
        "Option RemovePeaks requires input to BgraggPeaTablekWorkspace.");

  m_numFWHM = getProperty("NumberOfFWHM");
  if (m_numFWHM <= 0.)
    throw runtime_error("NumberOfFWHM must be larger than 0. ");

  // Remove peaks
  RemovePeaks remove;
  remove.setup(peaktablews);
  m_outputWS = remove.removePeaks(m_dataWS, m_wsIndex, m_numFWHM);

  // Dummy outputs
  setupDummyOutputWSes();

  return;
}

//----------------------------------------------------------------------------------------------
/** Constructor
  */
RemovePeaks::RemovePeaks() {}

//----------------------------------------------------------------------------------------------
/** Destructor
  */
RemovePeaks::~RemovePeaks() {}

//----------------------------------------------------------------------------------------------
/** Set up: parse peak workspace to vectors
  */
void RemovePeaks::setup(TableWorkspace_sptr peaktablews) {
  // Parse table workspace
  parsePeakTableWorkspace(peaktablews, m_vecPeakCentre, m_vecPeakFWHM);

  // Check
  if (m_vecPeakCentre.size() != m_vecPeakFWHM.size())
    throw runtime_error("Number of peak centres and FWHMs are different!");
  else if (m_vecPeakCentre.size() == 0)
    throw runtime_error(
        "There is not any peak entry in input table workspace.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Remove peaks from a input workspace
  */
Workspace2D_sptr
RemovePeaks::removePeaks(API::MatrixWorkspace_const_sptr dataws, int wsindex,
                         double numfwhm) {
  // Check
  if (m_vecPeakCentre.size() == 0)
    throw runtime_error("RemovePeaks has not been setup yet. ");

  // Initialize vectors
  const MantidVec &vecX = dataws->readX(wsindex);
  const MantidVec &vecY = dataws->readY(wsindex);
  const MantidVec &vecE = dataws->readE(wsindex);

  size_t sizex = vecX.size();
  vector<bool> vec_useX(sizex, true);

  // Exclude regions
  size_t numbkgdpoints =
      excludePeaks(vecX, vec_useX, m_vecPeakCentre, m_vecPeakFWHM, numfwhm);
  size_t numbkgdpointsy = numbkgdpoints;
  size_t sizey = vecY.size();
  if (sizex > sizey)
    --numbkgdpointsy;

  // Construct output workspace
  Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, numbkgdpoints,
                                          numbkgdpointsy));
  outws->getAxis(0)->setUnit(dataws->getAxis(0)->unit()->unitID());
  MantidVec &outX = outws->dataX(0);
  MantidVec &outY = outws->dataY(0);
  MantidVec &outE = outws->dataE(0);
  size_t index = 0;
  for (size_t i = 0; i < sizex; ++i) {
    if (vec_useX[i]) {
      if (index >= numbkgdpoints)
        throw runtime_error("Programming logic error (1)");
      outX[index] = vecX[i];
      ++index;
    }
  }
  index = 0;
  for (size_t i = 0; i < sizey; ++i) {
    if (vec_useX[i]) {
      if (index >= numbkgdpointsy)
        throw runtime_error("Programming logic error (2)");
      outY[index] = vecY[i];
      outE[index] = vecE[i];
      ++index;
    }
  }

  return outws;
}

//----------------------------------------------------------------------------------------------
/** Parse table workspace
  */
void RemovePeaks::parsePeakTableWorkspace(TableWorkspace_sptr peaktablews,
                                          vector<double> &vec_peakcentre,
                                          vector<double> &vec_peakfwhm) {
  // Get peak table workspace information
  vector<string> colnames = peaktablews->getColumnNames();
  int index_centre = -1;
  int index_fwhm = -1;
  for (int i = 0; i < static_cast<int>(colnames.size()); ++i) {
    string colname = colnames[i];
    if (colname.compare("TOF_h") == 0)
      index_centre = i;
    else if (colname.compare("FWHM") == 0)
      index_fwhm = i;
  }

  if (index_centre < 0 || index_fwhm < 0) {
    throw runtime_error(
        "Input Bragg peak table workspace does not have TOF_h and/or FWHM");
  }

  // Get values
  size_t numrows = peaktablews->rowCount();
  vec_peakcentre.resize(numrows, 0.);
  vec_peakfwhm.resize(numrows, 0.);

  for (size_t i = 0; i < numrows; ++i) {
    double centre = peaktablews->cell<double>(i, index_centre);
    double fwhm = peaktablews->cell<double>(i, index_fwhm);
    vec_peakcentre[i] = centre;
    vec_peakfwhm[i] = fwhm;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Exclude peaks from
  */
size_t RemovePeaks::excludePeaks(vector<double> v_inX, vector<bool> &v_useX,
                                 vector<double> v_centre, vector<double> v_fwhm,
                                 double num_fwhm) {
  // Validate
  if (v_centre.size() != v_fwhm.size())
    throw runtime_error("Input different number of peak centres and fwhm.");
  if (v_inX.size() != v_useX.size())
    throw runtime_error("Input differetn number of vec X and flag X.");

  // Flag peak regions
  size_t numpeaks = v_centre.size();
  for (size_t i = 0; i < numpeaks; ++i) {
    // Define boundary
    double centre = v_centre[i];
    double fwhm = v_fwhm[i];
    double xmin = centre - num_fwhm * fwhm;
    double xmax = centre + num_fwhm * fwhm;

    vector<double>::iterator viter;
    int i_min, i_max;

    // Locate index in v_inX
    if (xmin <= v_inX.front())
      i_min = 0;
    else if (xmin >= v_inX.back())
      i_min = static_cast<int>(v_inX.size()) - 1;
    else {
      viter = lower_bound(v_inX.begin(), v_inX.end(), xmin);
      i_min = static_cast<int>(viter - v_inX.begin());
    }

    if (xmax <= v_inX.front())
      i_max = 0;
    else if (xmax >= v_inX.back())
      i_max = static_cast<int>(v_inX.size()) - 1;
    else {
      viter = lower_bound(v_inX.begin(), v_inX.end(), xmax);
      i_max = static_cast<int>(viter - v_inX.begin());
    }

    // Flag the excluded region
    for (int i = i_min; i <= i_max; ++i)
      v_useX[i] = false;
  }

  // Count non-excluded region
  size_t count = 0;
  for (size_t i = 0; i < v_useX.size(); ++i)
    if (v_useX[i])
      ++count;

  return count;
}

} // namespace CurveFitting
} // namespace Mantid
