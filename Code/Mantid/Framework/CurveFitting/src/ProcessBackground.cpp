#include "MantidAlgorithms/ProcessBackground.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid;
using namespace Kernel;

namespace Mantid
{
namespace Algorithms
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
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "Anonymous", Direction::Input),
                            "Input workspace containg background.");
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output),
                            "Output workspace containing processed background");
      this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("ReferenceWorkspace", "", Direction::Input, API::PropertyMode::Optional),
                            "Optional reference workspace for adding data points. ");


      std::vector<std::string> options;
      options.push_back("SimpleRemovePeaks");
      options.push_back("DeleteRegion");
      options.push_back("AddRegion");

      auto validator = boost::make_shared<Kernel::StringListValidator>(options);
      this->declareProperty("Options", "SimpleRemovePeaks", validator, "Option to process the background.");


      this->declareProperty("LowerBound", Mantid::EMPTY_DBL(), "Lower boundary of the region to be deleted/added.");
      this->declareProperty("UpperBound", Mantid::EMPTY_DBL(), "Upper boundary of the region to be deleted/added.");

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
      else
      {
          g_log.error() << "Option " << option << " is not supported. " << std::endl;
          throw std::invalid_argument("Unsupported option. ");
      }

      // 3. Set output
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


} // namespace Algorithms
} // namespace Mantid
