/*WIKI*
 *
 Integrates arbitary shaped single crystal peaks defined on an [[MDHistoWorkspace]] using connected component analysis to determine
 regions of interest around each peak of the [[PeaksWorkspace]]. The output is an integrated [[PeaksWorkspace]] as well as an image
 containing the labels assigned to each cluster for diagnostic and visualisation purposes.

 This algorithm is very similar to, [[IntegratePeaksUsingClusters]] but breaks the integration into a series of local image domains rather than integrating an image in one-shot.
 The advantages of this approach are that you can locally define a background rather than using a global setting, and are therefore better able to capture the peak shape. A further advantage
 is that the memory requirement is reduced, given that [[MDHistoWorkspaces]] are generated in the region of the peak, and therefore high resolution can be achieved in the region of the peaks without
 an overall high n-dimensional image cost.

 [[File:ClusterImage.png|400px]]

 Unlike [[IntegratePeaksUsingClusters]] you do not need to specify at Threshold for background detection. You do however need to specify a BackgroundOuterRadius in a similar fashion to
 [[IntegratePeaksMD]]. This is used to determine the region in which to make an [[MDHistoWorkspace]] around each peak. A liberal estimate is a good idea.

 At present, you will need to provide NumberOfBins for binning (via [[BinMD]] axis-aligned). By default, the algorithm will create a 20 by 20 by 20 grid around each peak. The same
 number of bins is applied in each dimension.

 == Warnings and Logging ==
 See [[IntegratePeaksUsingClusters]] for notes on loggs and warnings.

 *WIKI*/

#include "MantidCrystal/IntegratePeaksHybrid.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Crystal
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(IntegratePeaksHybrid)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IntegratePeaksHybrid::IntegratePeaksHybrid()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IntegratePeaksHybrid::~IntegratePeaksHybrid()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string IntegratePeaksHybrid::name() const
    {
      return "IntegratePeaksHybrid";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int IntegratePeaksHybrid::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string IntegratePeaksHybrid::category() const
    {
      return "MDAlgorithms";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void IntegratePeaksHybrid::initDocs()
    {
      this->setWikiSummary("Integrate single crystal peaks using connected component analysis");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void IntegratePeaksHybrid::init()
    {
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "", Direction::Input),
          "Input md workspace.");
      declareProperty(new WorkspaceProperty<IPeaksWorkspace>("PeaksWorkspace", "", Direction::Input),
          "A PeaksWorkspace containing the peaks to integrate.");

      auto positiveIntValidator = boost::make_shared<BoundedValidator<int> >();
      positiveIntValidator->setExclusive(true);
      positiveIntValidator->setLower(0);

      declareProperty(
          new PropertyWithValue<int>("NumberOfBins", 20, positiveIntValidator, Direction::Input),
          "Number of bins to use while creating each local image. Defaults to 20. Increase to reduce pixelation");

      auto compositeValidator = boost::make_shared<CompositeValidator>();
      auto positiveDoubleValidator = boost::make_shared<BoundedValidator<double> >();
            positiveDoubleValidator->setExclusive(true);
            positiveDoubleValidator->setLower(0);
      compositeValidator->add(positiveDoubleValidator);
      compositeValidator->add(boost::make_shared<MandatoryValidator<double> >());

      declareProperty(new PropertyWithValue<double>("BackgroundOuterRadius", 0.0, compositeValidator, Direction::Input), "Background outer radius estimate. Choose liberal value.");

      declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace", "", Direction::Output),
          "An output integrated peaks workspace.");

      declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspaces", "", Direction::Output),
          "MDHistoWorkspaces containing the labeled clusters used by the algorithm.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void IntegratePeaksHybrid::exec()
    {
      // TODO Auto-generated execute stub
      IMDHistoWorkspace_sptr mdWS = getProperty("InputWorkspace");
      IPeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");
      IPeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
      const int numBins = getProperty("NumberOfBins");
      const double peakOuterRadius = getProperty("BackgroundOuterRadius");
      if (peakWS != inPeakWS)
      {
        auto cloneAlg = this->createChildAlgorithm("CloneWorkspace");
        cloneAlg->setProperty("InputWorkspace", inPeakWS);
        cloneAlg->setPropertyValue("OutputWorkspace", "out_ws");
        cloneAlg->execute();
        {
          Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
          peakWS = boost::dynamic_pointer_cast<IPeaksWorkspace>(temp);
        }
      }
    }

  } // namespace Crystal
} // namespace Mantid
