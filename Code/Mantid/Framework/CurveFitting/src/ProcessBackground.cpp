#include "MantidCurveFitting/ProcessBackground.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidCurveFitting/Polynomial.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid;
using namespace Kernel;

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
      return;
  }

  /*
   * Define parameters
   */
  void ProcessBackground::init()
  {
      /// Input and output Workspace
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "Anonymous", Direction::Input),
                            "Input workspace containg background.");
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output),
                            "Output workspace containing processed background");
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("ReferenceWorkspace", "", Direction::Input, API::PropertyMode::Optional),
                            "Optional reference workspace for adding data points. ");

      /// Function Options
      std::vector<std::string> options;
      options.push_back("SimpleRemovePeaks");
      options.push_back("DeleteRegion");
      options.push_back("AddRegion");
      options.push_back("SelectBackgroundPoints");

      auto validator = boost::make_shared<Kernel::StringListValidator>(options);
      this->declareProperty("Options", "SimpleRemovePeaks", validator, "Option to process the background.");

      /// Boundary
      this->declareProperty("LowerBound", Mantid::EMPTY_DBL(), "Lower boundary of the region to be deleted/added.");
      this->declareProperty("UpperBound", Mantid::EMPTY_DBL(), "Upper boundary of the region to be deleted/added.");

      /// Optional Function Type
      std::vector<std::string> bkgdtype;
      bkgdtype.push_back("Polynomial");
      bkgdtype.push_back("Chebyshev");
      auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
      declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Background type");

      /// User input background points for "SelectBackground"
      auto arrayproperty = new Kernel::ArrayProperty<double>("BackgroundPoints");
      declareProperty(arrayproperty, "User specified background points as starting points");

      /// Workspace index
      declareProperty("WorkspaceIndex", 0, "Workspace index for the input workspaces.");

      /// Background tolerance
      declareProperty("NoiseTolerance", 1.0, "Tolerance of noise range. ");

      return;
  }

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
          autoBackgroundSelection();
      }
      else
      {
          g_log.error() << "Option " << option << " is not supported. " << std::endl;
          throw std::invalid_argument("Unsupported option. ");
      }

      // 3. Set output
      // std::cout << "DBx510 OutWorkspace Range: " << outWS->dataX(0)[0] << ", " << outWS->dataX(0).back() << std::endl;
      setProperty("OutputWorkspace", outWS);

      return;
  }

  /*
   * Remove peaks within a specified region
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
              g_log.error() << "The vector X with value inserted is not ordered incremently" << std::endl;
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

  /*
   * Select background automatically
   */
  void ProcessBackground::autoBackgroundSelection()
  {
      // 1. Get data
      std::string backgroundtype = getProperty("BackgroundType");
      std::vector<double> bkgdpoints = getProperty("BackgroundPoints");
      int intemp = getProperty("WorkspaceIndex");
      if (intemp < 0)
          throw std::invalid_argument("WorkspaceIndex is not allowed to be less than 0. ");
      size_t wsindex = size_t(intemp);

      // 2. Construct background workspace for fit
      std::vector<double> realx, realy, reale;
      for (size_t i = 0; i < bkgdpoints.size(); ++i)
      {
          // a) Find the index in
          std::vector<double>::const_iterator it;
          it = std::lower_bound(inpWS->readX(wsindex).begin(), inpWS->readX(wsindex).end(), bkgdpoints[i]);
          size_t index = size_t(it - inpWS->readX(wsindex).begin());

          // std::cout << "DB502: Index = " << index << " For TOF = " << bkgdpoints[i] << " in [" << inpWS->readX(wsindex)[0] << ", "
          //          << inpWS->readX(wsindex).back() << "] " << std::endl;

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

      // 3. Fit input background pionts
      API::IAlgorithm_sptr fit;
      try
      {
          fit = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
      }
      catch (Exception::NotFoundError &)
      {
          g_log.error() << "Requires CurveFitting library." << std::endl;
          throw;
      }

      CurveFitting::BackgroundFunction_sptr bkgdfunction;

      if (backgroundtype.compare("Polynomial") == 0)
      {
          CurveFitting::Polynomial poly;
          bkgdfunction = boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(boost::make_shared<CurveFitting::Polynomial>(poly));
      }
      else if (backgroundtype.compare("Chebyshev") == 0)
      {
          throw std::runtime_error("Chebyshev background has not been implemented.");
      }
      else
      {
          g_log.error() << "Background of type " << backgroundtype << " is not supported. ";
          throw std::invalid_argument("Non-supported background function. ");
      }

      bkgdfunction->setAttributeValue("n", 6);

      fit->setProperty("Function", boost::dynamic_pointer_cast<API::IFunction>(bkgdfunction));
      fit->setProperty("InputWorkspace", bkgdWS);
      fit->setProperty("WorkspaceIndex", 0);
      fit->setProperty("MaxIterations", 50);
      fit->setProperty("StartX", realx[0]);
      fit->setProperty("EndX", realx.back());
      fit->setProperty("Minimizer", "Levenberg-Marquardt");
      fit->setProperty("CostFunction", "Least squares");

      fit->executeAsSubAlg();

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
      outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (API::WorkspaceFactory::Instance().create("Workspace2D", 1, vecx.size(), vecy.size()));

      for (size_t i = 0; i < vecx.size(); ++i)
      {          
          outWS->dataX(0)[i] = vecx[i];
          outWS->dataY(0)[i] = vecy[i];
          outWS->dataE(0)[i] = vece[i];
      }

      // d) Set property
      // this->setProperty("OutputWorkspace", outWS);

      return;
  } // END OF FUNCTION


} // namespace CurveFitting
} // namespace Mantid
