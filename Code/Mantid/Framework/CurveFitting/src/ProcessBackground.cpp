/*WIKI*

The algorithm ProcessBackground() provides several functions for user to process background to prepare [[Le Bail Fit]].   

==== Simple Remove Peaks ====
This algorithm is designed for refining the background based on the assumption that the all peaks have been fitted reasonably well. Then by removing the peaks by function 'X0 +/- n*FWHM', the rest data points are background in a very high probability.  
 
An arbitrary background function can be fitted against this background by standard optimizer. 

==== Automatic Background Points Selection ====
This feature is designed to select many background points with user's simple input.  
User is required to select only a few background points in the middle of two adjacent peaks.  
Algorithm will fit these few points (''BackgroundPoints'') to a background function of specified type.

  
*WIKI*/
#include "MantidCurveFitting/ProcessBackground.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/Chebyshev.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

DECLARE_ALGORITHM(ProcessBackground)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ProcessBackground::ProcessBackground()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ProcessBackground::~ProcessBackground()
  {
  }

  void ProcessBackground::initDocs()
  {
      this->setWikiSummary("ProcessBackground provides some tools to process powder diffraction pattern's background in order to help Le Bail Fit.");
      this->setOptionalMessage("ProcessBackground provides some tools to process powder diffraction pattern's background in order to help Le Bail Fit.");
  }

  //----------------------------------------------------------------------------------------------
  /** Define parameters
   */
  void ProcessBackground::init()
  {
    // Input and output Workspace
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace", "Anonymous", Direction::Input),
                            "Name of the output workspace containing the processed background.");
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "", Direction::Output),
                            "Output workspace containing processed background");
    declareProperty(new WorkspaceProperty<Workspace2D>("ReferenceWorkspace", "", Direction::Input,
                                                       PropertyMode::Optional),
                    "Name of the workspace containing the data required by function DeleteRegion.");

    // Function Options
    std::vector<std::string> options;
    options.push_back("SimpleRemovePeaks");
    options.push_back("DeleteRegion");
    options.push_back("AddRegion");
    options.push_back("SelectBackgroundPoints");

    auto validator = boost::make_shared<Kernel::StringListValidator>(options);
    declareProperty("Options", "SimpleRemovePeaks", validator, "Name of the functionality realized by this algorithm.");

    vector<string> pointsselectmode;
    pointsselectmode.push_back("All Background Points");
    pointsselectmode.push_back("Input Background Points Only");

    auto modevalidator = boost::make_shared<StringListValidator>(pointsselectmode);
    declareProperty("BackgroundPointSelectMode", "All Background Points", modevalidator,
                    "Mode to select background points. ");

    // Boundary
    declareProperty("LowerBound", Mantid::EMPTY_DBL(), "Lower boundary of the data to have background processed.");
    declareProperty("UpperBound", Mantid::EMPTY_DBL(), "Upper boundary of the data to have background processed.");

    // Optional Function Type
    std::vector<std::string> bkgdtype;
    bkgdtype.push_back("Polynomial");
    bkgdtype.push_back("Chebyshev");
    auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
    declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Type of the background. Options include Polynomial and Chebyshev.");

    // User input background points for "SelectBackground"
    auto arrayproperty = new Kernel::ArrayProperty<double>("BackgroundPoints");
    declareProperty(arrayproperty, "Vector of doubles, each of which is the X-axis value of the background point selected by user.");

    // Workspace index
    declareProperty("WorkspaceIndex", 0, "Workspace index for the input workspaces.");

    // Background tolerance
    declareProperty("NoiseTolerance", 1.0, "Tolerance of noise range. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution function
    */
  void ProcessBackground::exec()
  {
    // 1. Get workspace
    inpWS = this->getProperty("InputWorkspace");
    if (!inpWS)
    {
      g_log.error() << "Input Workspace cannot be obtained." << std::endl;
      throw std::invalid_argument("Input Workspace cannot be obtained.");
    }

    mLowerBound = getProperty("LowerBound");
    mUpperBound = getProperty("UpperBound");

    mTolerance = getProperty("NoiseTolerance");

    // 2. Do different work
    std::string option = getProperty("Options");
    if (option.compare("SimpleRemovePeaks") == 0)
    {
      removePeaks();
    }
    else if (option.compare("DeleteRegion") == 0)
    {
      deleteRegion();
    }
    else if (option.compare("AddRegion") == 0)
    {
      addRegion();
    }
    else if (option.compare("SelectBackgroundPoints") == 0)
    {
      execSelectBkgdPoints();
    }
    else
    {
      g_log.error() << "Option " << option << " is not supported. " << std::endl;
      throw std::invalid_argument("Unsupported option. ");
    }

    // 3. Set output
    setProperty("OutputWorkspace", outWS);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Remove peaks within a specified region
   */
  void ProcessBackground::removePeaks()
  {

      throw std::runtime_error("To Be Implemented Soon. ");
  }

  /*
   * Delete a certain region from input workspace
   */
  void ProcessBackground::deleteRegion()
  {
      // 1. Check boundary
      if (mLowerBound == Mantid::EMPTY_DBL() || mUpperBound == Mantid::EMPTY_DBL())
      {
          throw std::invalid_argument("Using DeleteRegion.  Both LowerBound and UpperBound must be specified.");
      }
      if (mLowerBound >= mUpperBound)
      {
          throw std::invalid_argument("Lower boundary cannot be equal or larger than upper boundary.");
      }

      // 2. Copy data
      const MantidVec& dataX = inpWS->readX(0);
      const MantidVec& dataY = inpWS->readY(0);
      const MantidVec& dataE = inpWS->readE(0);

      std::vector<double> vx, vy, ve;

      for (size_t i = 0; i < dataY.size(); ++i)
      {
          double xtmp = dataX[i];
          if (xtmp < mLowerBound || xtmp > mUpperBound)
          {
              vx.push_back(dataX[i]);
              vy.push_back(dataY[i]);
              ve.push_back(dataE[i]);
          }
      }
      if (dataX.size() > dataY.size())
      {
          vx.push_back(dataX.back());
      }

      // 4. Create new workspace
      size_t sizex = vx.size();
      size_t sizey = vy.size();
      API::MatrixWorkspace_sptr mws = API::WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
      outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(mws);

      for (size_t i = 0; i < sizey; ++i)
      {
          outWS->dataX(0)[i] = vx[i];
          outWS->dataY(0)[i] = vy[i];
          outWS->dataE(0)[i] = ve[i];
      }
      if (sizex > sizey)
      {
          outWS->dataX(0)[sizex-1] = vx.back();
      }

      return;
  }

  /*
   * Add a certain region from reference workspace
   */
  void ProcessBackground::addRegion()
  {
      // 1. Check boundary
      if (mLowerBound == Mantid::EMPTY_DBL() || mUpperBound == Mantid::EMPTY_DBL())
      {
          throw std::invalid_argument("Using AddRegion.  Both LowerBound and UpperBound must be specified.");
      }
      if (mLowerBound >= mUpperBound)
      {
          throw std::invalid_argument("Lower boundary cannot be equal or larger than upper boundary.");
      }

      // 2. Copy data
      const MantidVec& dataX = inpWS->readX(0);
      const MantidVec& dataY = inpWS->readY(0);
      const MantidVec& dataE = inpWS->readE(0);

      std::vector<double> vx, vy, ve;
      for (size_t i = 0; i < dataY.size(); ++i)
      {
          double xtmp = dataX[i];
          if (xtmp < mLowerBound || xtmp > mUpperBound)
          {
              vx.push_back(dataX[i]);
              vy.push_back(dataY[i]);
              ve.push_back(dataE[i]);
          }
      }
      if (dataX.size() > dataY.size())
      {
          vx.push_back(dataX.back());
      }

      // 3. Reference workspace
      DataObjects::Workspace2D_const_sptr refWS = getProperty("ReferenceWorkspace");
      if (!refWS)
      {
          throw std::invalid_argument("ReferenceWorkspace is not given. ");
      }

      const MantidVec& refX = refWS->dataX(0);
      const MantidVec& refY = refWS->dataY(0);
      const MantidVec& refE = refWS->dataE(0);

      // 4. Insert
      std::vector<double>::const_iterator refiter;
      refiter = std::lower_bound(refX.begin(), refX.end(), mLowerBound);
      size_t sindex = size_t(refiter-refX.begin());
      refiter = std::lower_bound(refX.begin(), refX.end(), mUpperBound);
      size_t eindex = size_t(refiter-refX.begin());

      for (size_t i = sindex; i < eindex; ++i)
      {
          double tmpx = refX[i];
          double tmpy = refY[i];
          double tmpe = refE[i];

          // Locate the position of tmpx in the array to be inserted
          std::vector<double>::iterator newit = std::lower_bound(vx.begin(), vx.end(), tmpx);
          size_t newindex = size_t(newit-vx.begin());

          // insert tmpx, tmpy, tmpe by iterator
          vx.insert(newit, tmpx);

          newit = vy.begin()+newindex;
          vy.insert(newit, tmpy);

          newit = ve.begin()+newindex;
          ve.insert(newit, tmpe);
      }

      // Check
      for (size_t i = 1; i < vx.size(); ++i)
      {
          if (vx[i] <= vx[i-1])
          {
              g_log.error() << "The vector X with value inserted is not ordered incrementally" << std::endl;
              throw std::runtime_error("Build new vector error!");
          }
      }

      // 5. Construct the new Workspace
      outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (API::WorkspaceFactory::Instance().create("Workspace2D", 1, vx.size(), vy.size()));
      for (size_t i = 0; i < vy.size(); ++i)
      {
          outWS->dataX(0)[i] = vx[i];
          outWS->dataY(0)[i] = vy[i];
          outWS->dataE(0)[i] = ve[i];
      }
      if (vx.size() > vy.size())
          outWS->dataX(0)[vx.size()-1] = vx.back();

      return;
  }

  //----------------------------------------------------------------------------------------------
  /** Select background points
    */
  void ProcessBackground::execSelectBkgdPoints()
  {
    // 1. Get data
    std::vector<double> bkgdpoints = getProperty("BackgroundPoints");
    int intemp = getProperty("WorkspaceIndex");
    if (intemp < 0)
        throw std::invalid_argument("WorkspaceIndex is not allowed to be less than 0. ");
    size_t wsindex = size_t(intemp);

    // 2. Construct background workspace for fit
    std::vector<double> realx, realy, reale;
    for (size_t i = 0; i < bkgdpoints.size(); ++i)
    {
      // a) Data validity test
      double bkgdpoint = bkgdpoints[i];
      if (bkgdpoint < inpWS->readX(wsindex)[0])
      {
        g_log.warning() << "Input background point " << bkgdpoint << " is out of lower boundary.  Use X[0] = "
                        << inpWS->readX(wsindex)[0] << " instead." << std::endl;
        bkgdpoint = inpWS->readX(wsindex)[0];
      }
      else if (bkgdpoint > inpWS->readX(wsindex).back())
      {
        g_log.warning() << "Input background point " << bkgdpoint << " is out of upper boundary.  Use X[-1] = "
                        << inpWS->readX(wsindex).back() << " instead." << std::endl;
        bkgdpoint = inpWS->readX(wsindex).back();
      }

      // b) Find the index in
      std::vector<double>::const_iterator it;
      it = std::lower_bound(inpWS->readX(wsindex).begin(), inpWS->readX(wsindex).end(), bkgdpoint);
      size_t index = size_t(it - inpWS->readX(wsindex).begin());

      g_log.debug() << "DBx502 Background Points " << i << " Index = " << index << " For TOF = "
                    << bkgdpoints[i] << " in [" << inpWS->readX(wsindex)[0] << ", "
                    << inpWS->readX(wsindex).back() << "] " << std::endl;

      // b) Add to list
      realx.push_back(inpWS->readX(wsindex)[index]);
      realy.push_back(inpWS->readY(wsindex)[index]);
      reale.push_back(inpWS->readE(wsindex)[index]);

    } // ENDFOR (i)

    DataObjects::Workspace2D_sptr bkgdWS =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, realx.size(), realy.size()));
    for (size_t i = 0; i < realx.size(); ++i)
    {
      bkgdWS->dataX(0)[i] = realx[i];
      bkgdWS->dataY(0)[i] = realy[i];
      bkgdWS->dataE(0)[i] = reale[i];
      // std::cout << "DBx514 Index = " << i << "  Add " << realx[i] << ", " << realy[i] << ", " << reale[i] << std::endl;
    }

    // 3. Select background points according to mode
    string mode = getProperty("BackgroundPointSelectMode");
    if (mode.compare("All Background Points") == 0)
    {
      // Select (possibly) all background points
      outWS = autoBackgroundSelection(wsindex, bkgdWS);
    }
    else if (mode.compare("Input Background Points Only") == 0)
    {
      // Use the input background points only
      outWS = bkgdWS;
    }
    else
    {
      stringstream errss;
      errss << "Background select mode " << mode << " is not supported by ProcessBackground.";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Select background automatically
   */
  DataObjects::Workspace2D_sptr ProcessBackground::autoBackgroundSelection(size_t wsindex, Workspace2D_sptr bkgdWS)
  {
    // 1. Get background type and create bakground function
    std::string backgroundtype = getProperty("BackgroundType");

    CurveFitting::BackgroundFunction_sptr bkgdfunction;

    if (backgroundtype.compare("Polynomial") == 0)
    {
      CurveFitting::Polynomial poly;
      bkgdfunction =
          boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(boost::make_shared<CurveFitting::Polynomial>(poly));
    }
    else if (backgroundtype.compare("Chebyshev") == 0)
    {
      CurveFitting::Chebyshev cheby;
      bkgdfunction = boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>
          (boost::make_shared<CurveFitting::Chebyshev>(cheby));
    }
    else
    {
      stringstream errss;
      errss << "Background of type " << backgroundtype << " is not supported. ";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    bkgdfunction->initialize();;
    bkgdfunction->setAttributeValue("n", 6);

    g_log.debug() << "DBx622 Background Workspace has " << bkgdWS->readX(0).size()
                  << " data points." << std::endl;

    // 2. Fit input background pionts
    API::IAlgorithm_sptr fit;
    try
    {
      fit = this->createChildAlgorithm("Fit", 0.0, 0.2, true);
    }
    catch (Exception::NotFoundError &)
    {
      g_log.error() << "Requires CurveFitting library." << std::endl;
      throw;
    }

    double startx = bkgdWS->readX(0)[0];
    double endx = bkgdWS->readX(0).back();
    fit->setProperty("Function", boost::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
    fit->setProperty("InputWorkspace", bkgdWS);
    fit->setProperty("WorkspaceIndex", 0);
    fit->setProperty("MaxIterations", 500);
    fit->setProperty("StartX", startx);
    fit->setProperty("EndX", endx);
    fit->setProperty("Minimizer", "Levenberg-Marquardt");
    fit->setProperty("CostFunction", "Least squares");

    fit->executeAsChildAlg();

    // 4. Get fit result
    // a) Status
    std::string fitStatus = fit->getProperty("OutputStatus");
    bool allowedfailure = (fitStatus.find("cannot") < fitStatus.size()) &&
        (fitStatus.find("tolerance") < fitStatus.size());
    if (fitStatus.compare("success") != 0 && !allowedfailure)
    {
      g_log.error() << "ProcessBackground: Fit Status = " << fitStatus << ".  Not to update fit result" << std::endl;
      throw std::runtime_error("Bad Fit");
    }

    // b) check that chi2 got better
    const double chi2 = fit->getProperty("OutputChi2overDoF");
    g_log.notice() << "Fit background: Fit Status = " << fitStatus << ", chi2 = " << chi2 << std::endl;

    // c) get out the parameter names
    API::IFunction_sptr func = fit->getProperty("Function");
    std::vector<std::string> parnames = func->getParameterNames();
    std::map<std::string, double> parvalues;
    for (size_t iname = 0; iname < parnames.size(); ++iname)
    {
      double value = func->getParameter(parnames[iname]);
      parvalues.insert(std::make_pair(parnames[iname], value));
    }

    /*
      DataObject::Workspace2D_const_sptr theorybackground = AnalysisDataService::Instance().retrieve(wsname);
      */

    // 5. Filter and construct for the output workspace
    // a) Calcualte theoretical values
    const std::vector<double> x = inpWS->readX(wsindex);
    API::FunctionDomain1DVector domain(x);
    API::FunctionValues values(domain);
    func->function(domain, values);

    // b) Filter
    std::vector<double> vecx, vecy, vece;
    for (size_t i = 0; i < domain.size(); ++i)
    {
      double y = inpWS->readY(wsindex)[i];
      double theoryy = values[i];
      if (y >= (theoryy-mTolerance) && y <= (theoryy+mTolerance) )
      {
        // Selected
        double x = domain[i];
        double e = inpWS->readE(wsindex)[i];
        vecx.push_back(x);
        vecy.push_back(y);
        vece.push_back(e);
      }
    }

    // c) Build new
    Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", 1, vecx.size(), vecy.size()));

    for (size_t i = 0; i < vecx.size(); ++i)
    {
      outws->dataX(0)[i] = vecx[i];
      outws->dataY(0)[i] = vecy[i];
      outws->dataE(0)[i] = vece[i];
    }

    return outws;
  } // END OF FUNCTION


} // namespace CurveFitting
} // namespace Mantid

