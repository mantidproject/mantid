// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ProcessBackground.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/Functions/Chebyshev.h"
#include "MantidCurveFitting/Functions/Polynomial.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <utility>

using namespace Mantid;

using namespace Mantid::API;

using namespace Mantid::Kernel;

using namespace Mantid::DataObjects;

using namespace Mantid::CurveFitting;

using namespace HistogramData;

using namespace std;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

DECLARE_ALGORITHM(ProcessBackground)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProcessBackground::ProcessBackground()
    : m_dataWS(), m_outputWS(), m_wsIndex(-1), m_lowerBound(DBL_MAX), m_upperBound(DBL_MIN), m_bkgdType(),
      m_numFWHM(-1.) {}

//----------------------------------------------------------------------------------------------
/** Define parameters
 */
void ProcessBackground::init() {
  // Input and output Workspace
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("InputWorkspace", "Anonymous", Direction::Input),
                  "Name of the output workspace containing the processed background.");

  // Workspace index
  declareProperty("WorkspaceIndex", 0, "Workspace index for the input workspaces.");

  // Output workspace
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace containing processed background");

  // Function Options
  std::vector<std::string> options{"SelectBackgroundPoints", "RemovePeaks", "DeleteRegion", "AddRegion"};

  auto validator = std::make_shared<Kernel::StringListValidator>(options);
  declareProperty("Options", "RemovePeaks", validator, "Name of the functionality realized by this algorithm.");

  // Boundary
  declareProperty("LowerBound", Mantid::EMPTY_DBL(), "Lower boundary of the data to have background processed.");
  declareProperty("UpperBound", Mantid::EMPTY_DBL(), "Upper boundary of the data to have background processed.");

  auto refwsprop = std::make_unique<WorkspaceProperty<Workspace2D>>("ReferenceWorkspace", "", Direction::Input,
                                                                    PropertyMode::Optional);
  declareProperty(std::move(refwsprop), "Name of the workspace containing the data "
                                        "required by function AddRegion.");
  setPropertySettings("ReferenceWorkspace", std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "AddRegion"));

  // Optional Function Type
  std::vector<std::string> bkgdtype{"Polynomial", "Chebyshev"};
  auto bkgdvalidator = std::make_shared<Kernel::StringListValidator>(bkgdtype);
  declareProperty("BackgroundType", "Polynomial", bkgdvalidator,
                  "Type of the background. Options include Polynomial and Chebyshev.");
  setPropertySettings("BackgroundType",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  vector<string> funcoptions{"N/A", "FitGivenDataPoints", "UserFunction"};
  auto fovalidator = std::make_shared<StringListValidator>(funcoptions);
  declareProperty("SelectionMode", "N/A", fovalidator,
                  "If choise is UserFunction, background will be selected by "
                  "an input background "
                  "function.  Otherwise, background function will be fitted "
                  "from user's input data points.");
  setPropertySettings("SelectionMode",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  declareProperty("BackgroundOrder", 0, "Order of polynomial or chebyshev background. ");
  setPropertySettings("BackgroundOrder",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));
  setPropertySettings("BackgroundOrder",
                      std::make_unique<VisibleWhenProperty>("SelectionMode", IS_EQUAL_TO, "FitGivenDataPoints"));

  // User input background points for "SelectBackground"
  auto arrayproperty = std::make_unique<Kernel::ArrayProperty<double>>("BackgroundPoints");
  declareProperty(std::move(arrayproperty), "Vector of doubles, each of which is the "
                                            "X-axis value of the background point "
                                            "selected by user.");
  setPropertySettings("BackgroundPoints",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));
  setPropertySettings("BackgroundPoints",
                      std::make_unique<VisibleWhenProperty>("SelectionMode", IS_EQUAL_TO, "FitGivenDataPoints"));

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("BackgroundTableWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Name of the table workspace containing background "
                  "parameters for mode SelectBackgroundPoints.");
  setPropertySettings("BackgroundTableWorkspace",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));
  setPropertySettings("BackgroundTableWorkspace",
                      std::make_unique<VisibleWhenProperty>("SelectionMode", IS_EQUAL_TO, "UserFunction"));

  // Mode to select background
  vector<string> pointsselectmode{"All Background Points", "Input Background Points Only"};
  auto modevalidator = std::make_shared<StringListValidator>(pointsselectmode);
  declareProperty("BackgroundPointSelectMode", "All Background Points", modevalidator,
                  "Mode to select background points. ");
  setPropertySettings("BackgroundPointSelectMode",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));
  setPropertySettings("BackgroundPointSelectMode",
                      std::make_unique<VisibleWhenProperty>("SelectionMode", IS_EQUAL_TO, "FitGivenDataPoints"));

  // Background tolerance
  declareProperty("NoiseTolerance", 1.0, "Tolerance of noise range. ");
  setPropertySettings("NoiseTolerance",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Background tolerance
  declareProperty("NegativeNoiseTolerance", EMPTY_DBL(), "Tolerance of noise range for negative number. ");
  setPropertySettings("NegativeNoiseTolerance",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Optional output workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace2D>>("UserBackgroundWorkspace", "_dummy01", Direction::Output),
      "Output workspace containing fitted background from points "
      "specified by users.");
  setPropertySettings("UserBackgroundWorkspace",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Optional output workspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputBackgroundParameterWorkspace", "_dummy02",
                                                                      Direction::Output),
                  "Output parameter table workspace containing the background fitting "
                  "result. ");
  setPropertySettings("OutputBackgroundParameterWorkspace",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Output background type.
  std::vector<std::string> outbkgdtype{"Polynomial", "Chebyshev"};
  auto outbkgdvalidator = std::make_shared<Kernel::StringListValidator>(outbkgdtype);
  declareProperty("OutputBackgroundType", "Polynomial", outbkgdvalidator,
                  "Type of background to fit with selected background points.");
  setPropertySettings("OutputBackgroundType",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Output background type.
  declareProperty("OutputBackgroundOrder", 6, "Order of background to fit with selected background points.");
  setPropertySettings("OutputBackgroundOrder",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "SelectBackgroundPoints"));

  // Peak table workspac for "RemovePeaks"
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("BraggPeakTableWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Name of table workspace containing peaks' parameters. ");
  setPropertySettings("BraggPeakTableWorkspace",
                      std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "RemovePeaks"));

  // Number of FWHM to have peak removed
  declareProperty("NumberOfFWHM", 1.0, "Number of FWHM to as the peak region to have peak removed. ");
  setPropertySettings("NumberOfFWHM", std::make_unique<VisibleWhenProperty>("Options", IS_EQUAL_TO, "RemovePeaks"));
}

//----------------------------------------------------------------------------------------------
/** Main execution function
 */
void ProcessBackground::exec() {
  // Process general properties
  m_dataWS = this->getProperty("InputWorkspace");
  if (!m_dataWS) {
    g_log.error() << "Input Workspace cannot be obtained.\n";
    throw std::invalid_argument("Input Workspace cannot be obtained.");
  }

  m_bkgdType = getPropertyValue("BackgroundType");

  int intemp = getProperty("WorkspaceIndex");
  if (intemp < 0)
    throw std::invalid_argument("WorkspaceIndex is not allowed to be less than 0. ");
  m_wsIndex = intemp;
  if (m_wsIndex >= static_cast<int>(m_dataWS->getNumberHistograms()))
    throw runtime_error("Workspace index is out of boundary.");

  m_lowerBound = getProperty("LowerBound");
  m_upperBound = getProperty("UpperBound");
  if (isEmpty(m_lowerBound))
    m_lowerBound = m_dataWS->x(m_wsIndex).front();
  if (isEmpty(m_upperBound))
    m_upperBound = m_dataWS->x(m_wsIndex).back();

  // 2. Do different work
  std::string option = getProperty("Options");
  if (option == "RemovePeaks") {
    removePeaks();
  } else if (option == "DeleteRegion") {
    deleteRegion();
  } else if (option == "AddRegion") {
    addRegion();
  } else if (option == "SelectBackgroundPoints") {

    selectBkgdPoints();
  } else {
    g_log.error() << "Option " << option << " is not supported. \n";
    throw std::invalid_argument("Unsupported option. ");
  }

  // 3. Set output
  setProperty("OutputWorkspace", m_outputWS);
}

//----------------------------------------------------------------------------------------------
/** Set dummy output workspaces to avoid python error for optional workspaces
 */
void ProcessBackground::setupDummyOutputWSes() {
  // Dummy outputs to make it work with python script
  setPropertyValue("UserBackgroundWorkspace", "dummy0");
  Workspace2D_sptr dummyws =
      std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1));
  setProperty("UserBackgroundWorkspace", dummyws);

  setPropertyValue("OutputBackgroundParameterWorkspace", "dummy1");
  TableWorkspace_sptr dummytbws = std::make_shared<TableWorkspace>();
  setProperty("OutputBackgroundParameterWorkspace", dummytbws);
}

//----------------------------------------------------------------------------------------------
/** Delete a certain region from input workspace
 */
void ProcessBackground::deleteRegion() {
  // Check boundary
  if (m_lowerBound == Mantid::EMPTY_DBL() || m_upperBound == Mantid::EMPTY_DBL()) {
    throw std::invalid_argument("Using DeleteRegion.  Both LowerBound and "
                                "UpperBound must be specified.");
  }
  if (m_lowerBound >= m_upperBound) {
    throw std::invalid_argument("Lower boundary cannot be equal or larger than upper boundary.");
  }

  const auto &dataX = m_dataWS->x(0);
  const auto &dataY = m_dataWS->y(0);
  const auto &dataE = m_dataWS->e(0);

  // Find the dimensions of the region excluded by m_lowerBound and m_upperBound
  std::vector<size_t> incIndexes;
  for (size_t i = 0; i < dataY.size(); i++) {
    if (dataX[i] < m_lowerBound || dataX[i] > m_upperBound) {
      incIndexes.emplace_back(i);
    }
  }
  size_t sizex = incIndexes.size();
  size_t sizey = sizex;
  if (dataX.size() > dataY.size()) {
    sizex++;
  }

  // Create a new workspace with these dimensions and copy data from the defined
  // region
  API::MatrixWorkspace_sptr mws = API::WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
  m_outputWS = std::dynamic_pointer_cast<DataObjects::Workspace2D>(mws);
  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());

  for (size_t i = 0; i < sizey; i++) {
    size_t index = incIndexes[i];
    m_outputWS->mutableX(0)[i] = dataX[index];
    m_outputWS->mutableY(0)[i] = dataY[index];
    m_outputWS->mutableE(0)[i] = dataE[index];
  }
  if (sizex > sizey) {
    m_outputWS->mutableX(0)[sizex - 1] = dataX.back();
  }

  // Set up dummies
  setupDummyOutputWSes();
}

//----------------------------------------------------------------------------------------------
/** Add a certain region from reference workspace
 */
void ProcessBackground::addRegion() {
  // Check boundary, which are required
  if (m_lowerBound == Mantid::EMPTY_DBL() || m_upperBound == Mantid::EMPTY_DBL()) {
    throw std::invalid_argument("Using AddRegion.  Both LowerBound and UpperBound must be specified.");
  }
  if (m_lowerBound >= m_upperBound) {
    throw std::invalid_argument("Lower boundary cannot be equal or larger than upper boundary.");
  }

  // Copy data to a set of vectors
  const auto &vecX = m_dataWS->x(0);
  const auto &vecY = m_dataWS->y(0);
  const auto &vecE = m_dataWS->e(0);

  std::vector<double> vx, vy, ve;
  for (size_t i = 0; i < vecY.size(); ++i) {
    double xtmp = vecX[i];
    if (xtmp < m_lowerBound || xtmp > m_upperBound) {
      vx.emplace_back(vecX[i]);
      vy.emplace_back(vecY[i]);
      ve.emplace_back(vecE[i]);
    }
  }

  // Histogram
  if (vecX.size() > vecY.size())
    vx.emplace_back(vecX.back());

  // Get access to reference workspace
  DataObjects::Workspace2D_const_sptr refWS = getProperty("ReferenceWorkspace");
  if (!refWS)
    throw std::invalid_argument("ReferenceWorkspace is not given. ");

  const auto &refX = refWS->x(0);
  const auto &refY = refWS->y(0);
  const auto &refE = refWS->e(0);

  // 4. Insert the value of the reference workspace from lowerBoundary to
  // upperBoundary
  std::vector<double>::const_iterator refiter;
  refiter = std::lower_bound(refX.begin(), refX.end(), m_lowerBound);
  size_t sindex = size_t(refiter - refX.begin());
  refiter = std::lower_bound(refX.begin(), refX.end(), m_upperBound);
  size_t eindex = size_t(refiter - refX.begin());

  for (size_t i = sindex; i < eindex; ++i) {
    const double tmpx = refX[i];
    const double tmpy = refY[i];
    const double tmpe = refE[i];

    // Locate the position of tmpx in the array to be inserted
    auto newit = std::lower_bound(vx.begin(), vx.end(), tmpx);
    size_t newindex = size_t(newit - vx.begin());

    // insert tmpx, tmpy, tmpe by iterator
    vx.insert(newit, tmpx);

    newit = vy.begin() + newindex;
    vy.insert(newit, tmpy);

    newit = ve.begin() + newindex;
    ve.insert(newit, tmpe);
  }

  // Check
  const auto it = std::adjacent_find(vx.cbegin(), vx.cend(), std::greater_equal<double>());
  if (it != vx.cend()) {
    g_log.error() << "The vector X with value inserted is not ordered incrementally" << std::endl;
    throw std::runtime_error("Build new vector error!");
  }

  // Construct the new Workspace
  m_outputWS = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, vx.size(), vy.size()));
  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());
  m_outputWS->setHistogram(0, Histogram(Points(vx), Counts(vy), CountStandardDeviations(ve)));

  // Write out dummy output workspaces
  setupDummyOutputWSes();
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
  string outbkgdparwsname = getPropertyValue("OutputBackgroundParameterWorkspace");
  if (!outbkgdparwsname.empty() && outbkgdparwsname != "_dummy02") {
    // Will fit the selected background
    string bkgdfunctype = getPropertyValue("OutputBackgroundType");
    fitBackgroundFunction(bkgdfunctype);
  } else {
    setupDummyOutputWSes();
  }

  m_outputWS->getAxis(0)->setUnit(m_dataWS->getAxis(0)->unit()->unitID());
}

//----------------------------------------------------------------------------------------------
/** Select background points
 */
void ProcessBackground::selectFromGivenXValues() {
  // Get special input properties
  std::vector<double> bkgdpoints = getProperty("BackgroundPoints");
  string mode = getProperty("BackgroundPointSelectMode");

  // Construct background workspace for fit
  std::vector<size_t> realIndexes;
  const auto &vecX = m_dataWS->x(m_wsIndex);
  const auto &vecY = m_dataWS->y(m_wsIndex);
  const auto &vecE = m_dataWS->e(m_wsIndex);
  for (size_t i = 0; i < bkgdpoints.size(); ++i) {
    // Data range validation
    double bkgdpoint = bkgdpoints[i];
    if (bkgdpoint < vecX.front()) {
      g_log.warning() << "Input background point " << bkgdpoint << " is out of lower boundary.  "
                      << "Use X[0] = " << vecX.front() << " instead."
                      << "\n";
      bkgdpoint = vecX.front();
    } else if (bkgdpoint > vecX.back()) {
      g_log.warning() << "Input background point " << bkgdpoint
                      << " is out of upper boundary.  Use X[-1] = " << vecX.back() << " instead."
                      << "\n";
      bkgdpoint = vecX.back();
    }

    // Find the index in
    std::vector<double>::const_iterator it;
    it = std::lower_bound(vecX.begin(), vecX.end(), bkgdpoint);
    size_t index = size_t(it - vecX.begin());

    g_log.debug() << "DBx502 Background Points " << i << " Index = " << index << " For TOF = " << bkgdpoints[i]
                  << " in [" << vecX[0] << ", " << vecX.back() << "] "
                  << "\n";

    // Add index to list
    realIndexes.emplace_back(index);

  } // ENDFOR (i)

  size_t wsSize = realIndexes.size();
  DataObjects::Workspace2D_sptr bkgdWS = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, wsSize, wsSize));
  for (size_t i = 0; i < wsSize; ++i) {
    size_t index = realIndexes[i];
    bkgdWS->mutableX(0)[i] = vecX[index];
    bkgdWS->mutableY(0)[i] = vecY[index];
    bkgdWS->mutableE(0)[i] = vecE[index];
  }

  // Select background points according to mode
  if (mode == "All Background Points") {
    // Select (possibly) all background points
    m_outputWS = autoBackgroundSelection(bkgdWS);
  } else if (mode == "Input Background Points Only") {
    // Use the input background points only
    m_outputWS = bkgdWS;
  } else {
    stringstream errss;
    errss << "Background select mode " << mode << " is not supported by ProcessBackground.";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }
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
      parmap.emplace(parname, parvalue);
  }

  auto bkgdorder = static_cast<int>(parmap.size() - 1); // A0 - A(n) total n+1 parameters
  bkgdfunc->setAttributeValue("n", bkgdorder);
  for (const auto &mit : parmap) {
    string parname = mit.first;
    double parvalue = mit.second;
    bkgdfunc->setParameter(parname, parvalue);
  }

  // Filter out
  m_outputWS = filterForBackground(bkgdfunc);
}

//----------------------------------------------------------------------------------------------
/** Select background automatically
 */
DataObjects::Workspace2D_sptr ProcessBackground::autoBackgroundSelection(const Workspace2D_sptr &bkgdWS) {
  // Get background type and create bakground function
  BackgroundFunction_sptr bkgdfunction = createBackgroundFunction(m_bkgdType);

  int bkgdorder = getProperty("BackgroundOrder");
  if (bkgdorder == 0)
    g_log.warning("(Input) background function order is 0.  It might not be "
                  "able to give a good estimation.");

  bkgdfunction->setAttributeValue("n", bkgdorder);
  bkgdfunction->initialize();

  g_log.information() << "Input background points has " << bkgdWS->x(0).size() << " data points for fit " << bkgdorder
                      << "-th order " << bkgdfunction->name() << " (background) function" << bkgdfunction->asString()
                      << "\n";

  // Fit input (a few) background pionts to get initial guess
  API::IAlgorithm_sptr fit;
  try {
    fit = this->createChildAlgorithm("Fit", 0.0, 0.2, true);
  } catch (Exception::NotFoundError &) {
    g_log.error() << "Requires CurveFitting library.\n";
    throw;
  }

  double startx = m_lowerBound;
  double endx = m_upperBound;
  fit->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
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
  bool allowedfailure = (fitStatus.find("Changes") < fitStatus.size()) && (fitStatus.find("small") < fitStatus.size());
  if (fitStatus != "success" && !allowedfailure) {
    g_log.error() << "ProcessBackground: Fit Status = " << fitStatus << ".  Not to update fit result\n";
    throw std::runtime_error("Bad Fit " + fitStatus);
  }

  // b) check that chi2 got better
  const double chi2 = fit->getProperty("OutputChi2overDoF");
  g_log.information() << "Fit background: Fit Status = " << fitStatus << ", chi2 = " << chi2 << "\n";

  // Filter and construct for the output workspace
  Workspace2D_sptr outws = filterForBackground(bkgdfunction);

  return outws;
} // END OF FUNCTION

//----------------------------------------------------------------------------------------------
/** Create a background function from input properties
 */
BackgroundFunction_sptr ProcessBackground::createBackgroundFunction(const string &backgroundtype) {
  Functions::BackgroundFunction_sptr bkgdfunction;

  if (backgroundtype == "Polynomial") {
    bkgdfunction = std::dynamic_pointer_cast<Functions::BackgroundFunction>(std::make_shared<Functions::Polynomial>());
    bkgdfunction->initialize();
  } else if (backgroundtype == "Chebyshev") {
    Chebyshev_sptr cheby = std::make_shared<Functions::Chebyshev>();
    bkgdfunction = std::dynamic_pointer_cast<Functions::BackgroundFunction>(cheby);
    bkgdfunction->initialize();

    g_log.debug() << "[D] Chebyshev is set to range " << m_lowerBound << ", " << m_upperBound << "\n";
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
Workspace2D_sptr ProcessBackground::filterForBackground(const BackgroundFunction_sptr &bkgdfunction) {
  double posnoisetolerance = getProperty("NoiseTolerance");
  double negnoisetolerance = getProperty("NegativeNoiseTolerance");
  if (isEmpty(negnoisetolerance))
    negnoisetolerance = posnoisetolerance;

  // Calcualte theoretical values
  const auto &x = m_dataWS->x(m_wsIndex);
  API::FunctionDomain1DVector domain(x.rawData());
  API::FunctionValues values(domain);
  bkgdfunction->function(domain, values);

  g_log.information() << "Function used to select background points : " << bkgdfunction->asString() << "\n";

  // Optional output
  string userbkgdwsname = getPropertyValue("UserBackgroundWorkspace");
  if (userbkgdwsname.empty())
    throw runtime_error("In mode SelectBackgroundPoints, "
                        "UserBackgroundWorkspace must be given!");

  size_t sizex = domain.size();
  size_t sizey = values.size();
  MatrixWorkspace_sptr visualws =
      std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 4, sizex, sizey));
  for (size_t i = 0; i < sizex; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      visualws->mutableX(j)[i] = domain[i];
    }
  }
  for (size_t i = 0; i < sizey; ++i) {
    visualws->mutableY(0)[i] = values[i];
    visualws->mutableY(1)[i] = m_dataWS->y(m_wsIndex)[i] - values[i];
    visualws->mutableY(2)[i] = posnoisetolerance;
    visualws->mutableY(3)[i] = -negnoisetolerance;
  }
  setProperty("UserBackgroundWorkspace", visualws);

  // Filter for background
  std::vector<size_t> selectedIndexes;
  for (size_t i = 0; i < domain.size(); ++i) {
    double purey = visualws->y(1)[i];
    if (purey < posnoisetolerance && purey > -negnoisetolerance) {
      selectedIndexes.emplace_back(i);
    }
  }
  size_t wsSize = selectedIndexes.size();
  g_log.information() << "Found " << wsSize << " background points out of " << m_dataWS->x(m_wsIndex).size()
                      << " total data points. "
                      << "\n";

  // Build new workspace for OutputWorkspace
  size_t nspec = 3;
  Workspace2D_sptr outws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nspec, wsSize, wsSize));
  for (size_t i = 0; i < wsSize; ++i) {
    size_t index = selectedIndexes[i];
    for (size_t j = 0; j < nspec; ++j)
      outws->mutableX(j)[i] = domain[index];
    outws->mutableY(0)[i] = m_dataWS->y(m_wsIndex)[index];
    outws->mutableE(0)[i] = m_dataWS->e(m_wsIndex)[index];
  }

  return outws;
}

//----------------------------------------------------------------------------------------------
/** Fit background function
 */
void ProcessBackground::fitBackgroundFunction(const std::string &bkgdfunctiontype) {
  // Get background type and create bakground function
  BackgroundFunction_sptr bkgdfunction = createBackgroundFunction(bkgdfunctiontype);

  int bkgdorder = getProperty("OutputBackgroundOrder");
  bkgdfunction->setAttributeValue("n", bkgdorder);

  if (bkgdfunctiontype == "Chebyshev") {
    double xmin = m_outputWS->x(0).front();
    double xmax = m_outputWS->x(0).back();
    g_log.information() << "Chebyshev Fit range: " << xmin << ", " << xmax << "\n";
    bkgdfunction->setAttributeValue("StartX", xmin);
    bkgdfunction->setAttributeValue("EndX", xmax);
  }

  g_log.information() << "Fit selected background " << bkgdfunctiontype << " to data workspace with "
                      << m_outputWS->getNumberHistograms() << " spectra."
                      << "\n";

  // Fit input (a few) background pionts to get initial guess
  API::IAlgorithm_sptr fit;
  try {
    fit = this->createChildAlgorithm("Fit", 0.9, 1.0, true);
  } catch (Exception::NotFoundError &) {
    g_log.error() << "Requires CurveFitting library.\n";
    throw;
  }

  g_log.information() << "Fitting background function: " << bkgdfunction->asString() << "\n";

  double startx = m_lowerBound;
  double endx = m_upperBound;
  fit->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
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
  bool allowedfailure = (fitStatus.find("Changes") < fitStatus.size()) && (fitStatus.find("small") < fitStatus.size());
  if (fitStatus != "success" && !allowedfailure) {
    g_log.error() << "ProcessBackground: Fit Status = " << fitStatus << ".  Not to update fit result\n";
    throw std::runtime_error("Bad Fit " + fitStatus);
  }

  const double chi2 = fit->getProperty("OutputChi2overDoF");
  g_log.information() << "Fit background: Fit Status = " << fitStatus << ", chi2 = " << chi2 << "\n";

  // Get out the parameter names
  API::IFunction_sptr funcout = fit->getProperty("Function");
  TableWorkspace_sptr outbkgdparws = std::make_shared<TableWorkspace>();
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

  g_log.information() << "Set table workspace (#row = " << outbkgdparws->rowCount()
                      << ") to OutputBackgroundParameterTable. "
                      << "\n";
  setProperty("OutputBackgroundParameterWorkspace", outbkgdparws);

  // Set output workspace
  const auto &vecX = m_outputWS->x(0);
  const auto &vecY = m_outputWS->y(0);
  FunctionDomain1DVector domain(vecX.rawData());
  FunctionValues values(domain);

  funcout->function(domain, values);

  auto &dataModel = m_outputWS->mutableY(1);
  auto &dataDiff = m_outputWS->mutableY(2);
  for (size_t i = 0; i < dataModel.size(); ++i) {
    dataModel[i] = values[i];
    dataDiff[i] = vecY[i] - dataModel[i];
  }
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
    throw runtime_error("Option RemovePeaks requires input to BgraggPeaTablekWorkspace.");

  m_numFWHM = getProperty("NumberOfFWHM");
  if (m_numFWHM <= 0.)
    throw runtime_error("NumberOfFWHM must be larger than 0. ");

  // Remove peaks
  RemovePeaks remove;
  remove.setup(peaktablews);
  m_outputWS = remove.removePeaks(m_dataWS, m_wsIndex, m_numFWHM);

  // Dummy outputs
  setupDummyOutputWSes();
}

//----------------------------------------------------------------------------------------------
/** Set up: parse peak workspace to vectors
 */
void RemovePeaks::setup(const TableWorkspace_sptr &peaktablews) {
  // Parse table workspace
  parsePeakTableWorkspace(peaktablews, m_vecPeakCentre, m_vecPeakFWHM);

  // Check
  if (m_vecPeakCentre.size() != m_vecPeakFWHM.size())
    throw runtime_error("Number of peak centres and FWHMs are different!");
  else if (m_vecPeakCentre.empty())
    throw runtime_error("There is not any peak entry in input table workspace.");
}

//----------------------------------------------------------------------------------------------
/** Remove peaks from a input workspace
 */
Workspace2D_sptr RemovePeaks::removePeaks(const API::MatrixWorkspace_const_sptr &dataws, int wsindex, double numfwhm) {
  // Check
  if (m_vecPeakCentre.empty())
    throw runtime_error("RemovePeaks has not been setup yet. ");

  // Initialize vectors
  const auto &vecX = dataws->x(wsindex);
  const auto &vecY = dataws->y(wsindex);
  const auto &vecE = dataws->e(wsindex);

  size_t sizex = vecX.size();
  vector<bool> vec_useX(sizex, true);

  // Exclude regions
  size_t numbkgdpoints = excludePeaks(vecX.rawData(), vec_useX, m_vecPeakCentre, m_vecPeakFWHM, numfwhm);
  size_t numbkgdpointsy = numbkgdpoints;
  size_t sizey = vecY.size();
  if (sizex > sizey)
    --numbkgdpointsy;

  // Construct output workspace
  Workspace2D_sptr outws = std::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, numbkgdpoints, numbkgdpointsy));
  outws->getAxis(0)->setUnit(dataws->getAxis(0)->unit()->unitID());
  auto &outX = outws->mutableX(0);
  auto &outY = outws->mutableY(0);
  auto &outE = outws->mutableE(0);
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
void RemovePeaks::parsePeakTableWorkspace(const TableWorkspace_sptr &peaktablews, vector<double> &vec_peakcentre,
                                          vector<double> &vec_peakfwhm) {
  // Get peak table workspace information
  vector<string> colnames = peaktablews->getColumnNames();
  int index_centre = -1;
  int index_fwhm = -1;
  for (int i = 0; i < static_cast<int>(colnames.size()); ++i) {
    string colname = colnames[i];
    if (colname == "TOF_h")
      index_centre = i;
    else if (colname == "FWHM")
      index_fwhm = i;
  }

  if (index_centre < 0 || index_fwhm < 0) {
    throw runtime_error("Input Bragg peak table workspace does not have TOF_h and/or FWHM");
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
}

//----------------------------------------------------------------------------------------------
/** Exclude peaks from
 */
size_t RemovePeaks::excludePeaks(vector<double> v_inX, vector<bool> &v_useX, vector<double> v_centre,
                                 vector<double> v_fwhm, double num_fwhm) {
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
    for (int excluded = i_min; excluded <= i_max; ++excluded)
      v_useX[excluded] = false;
  }

  return std::count(v_useX.cbegin(), v_useX.cend(), true);
}

} // namespace Mantid::CurveFitting::Functions
