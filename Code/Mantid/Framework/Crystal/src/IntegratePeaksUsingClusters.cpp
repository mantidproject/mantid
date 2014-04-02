/*WIKI*
Integrates arbitary shaped single crystal peaks defined on an [[MDHistoWorkspace]] using connected component analysis to determine
regions of interest around each peak of the [[PeaksWorkspace]]. The output is an integrated [[PeaksWorkspace]] as well as an image 
containing the labels assigned to each cluster for diagnostic and visualisation purposes.

A threshold for the Peak should be defined below which, parts of the image are treated as background. In addition, a radius estimate
is required to dispose of those clusters which are not to do with peaks, and also to associate clusters in the image with a peak center.
You can view the radius estimate as a radius cut-off.

This algorithm uses an imaging technique, and it is therefore important that the MDHistoWorkspace you are using is binned to a sufficient
resolution via [[BinMD]]. You can overlay the intergrated peaks workspace in the [[MantidPlot:_SliceViewer#Viewing_Peaks_Workspaces|Slice Viewer]] over
the generated Cluster Labeled OutputWorkspaceMD to see what the interation region used for each peak amounts to. 
*WIKI*/

#include "MantidCrystal/IntegratePeaksUsingClusters.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/PeakBackground.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Utils.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <boost/make_shared.hpp>
#include <map>
#include <algorithm>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Crystal::ConnectedComponentMappingTypes;

namespace
{
  

  class IsNearPeak
  {
  public:
    IsNearPeak(const V3D& coordinates, const double& thresholdDistance ) : m_coordinates(coordinates), m_thresholdDistance(thresholdDistance)
    {}
    bool operator()( const PositionToLabelIdMap::value_type& v ) const 
    { 
      return  v.first.distance(m_coordinates) < m_thresholdDistance; 
    }
  private:
    const V3D m_coordinates;
    const double m_thresholdDistance;
  };

}

namespace Mantid
{
  namespace Crystal
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(IntegratePeaksUsingClusters)



    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    IntegratePeaksUsingClusters::IntegratePeaksUsingClusters()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    IntegratePeaksUsingClusters::~IntegratePeaksUsingClusters()
    {
    }


    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string IntegratePeaksUsingClusters::name() const { return "IntegratePeaksUsingClusters";};

    /// Algorithm's version for identification. @see Algorithm::version
    int IntegratePeaksUsingClusters::version() const { return 1;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string IntegratePeaksUsingClusters::category() const { return "MDAlgorithms";}

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void IntegratePeaksUsingClusters::initDocs()
    {
      this->setWikiSummary("Integrate single crystal peaks using connected component analysis");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void IntegratePeaksUsingClusters::init()
    {
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input), "Input md workspace.");
      declareProperty(new WorkspaceProperty<IPeaksWorkspace>("PeaksWorkspace","", Direction::Input),"A PeaksWorkspace containing the peaks to integrate.");
      
      auto positiveValidator = boost::make_shared<BoundedValidator<double> >();
      positiveValidator->setLower(0);
      
      auto compositeValidator = boost::make_shared<CompositeValidator>();
      compositeValidator->add(positiveValidator);
      compositeValidator->add(boost::make_shared<MandatoryValidator<double > >());

      declareProperty(new PropertyWithValue<double>("RadiusEstimate", 0.0, compositeValidator, Direction::Input), "Estimate of Peak Radius. Points beyond this radius will not be considered, so caution towards the larger end.");
      
      declareProperty(new PropertyWithValue<double>("Threshold", 0, positiveValidator->clone(), Direction::Input), "Threshold signal above which to consider peaks");
      declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace","",Direction::Output), "An output integrated peaks workspace."); 
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspaceMD","",Direction::Output), "MDHistoWorkspace containing the labeled clusters used by the algorithm.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void IntegratePeaksUsingClusters::exec()
    {
      IMDHistoWorkspace_sptr mdWS = getProperty("InputWorkspace");
      IPeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");
      IPeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
      if (peakWS != inPeakWS)
      {
        auto cloneAlg = createChildAlgorithm("CloneWorkspace");
        cloneAlg->setProperty("InputWorkspace", inPeakWS);
        cloneAlg->setPropertyValue("OutputWorkspace", "out_ws");
        cloneAlg->execute();
        {
          Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
          peakWS = boost::dynamic_pointer_cast<IPeaksWorkspace>(temp);
        }
      }

      const SpecialCoordinateSystem mdCoordinates = mdWS->getSpecialCoordinateSystem();
      if(mdCoordinates == None)
      {
        throw std::invalid_argument("The coordinate system of the input MDWorkspace cannot be established. Run SetSpecialCoordinates on InputWorkspace.");
      }

      const double threshold = getProperty("Threshold");
      const double radiusEstimate = getProperty("RadiusEstimate");
      PeakBackground backgroundStrategy(peakWS, radiusEstimate, threshold, NoNormalization, mdCoordinates);
      //HardThresholdBackground backgroundStrategy(threshold,NoNormalization); 

      ConnectedComponentLabeling analysis; 
      LabelIdIntensityMap labelMap;
      PositionToLabelIdMap positionMap;

      Progress progress(this, 0, 1, 1);
      IMDHistoWorkspace_sptr clusters = analysis.executeAndIntegrate(mdWS, &backgroundStrategy, labelMap, positionMap, progress);

      // Link integrated values up with peaks.
      const int nPeaks = peakWS->getNumberPeaks();
      progress.resetNumSteps(nPeaks, 0, 1);
      progress.doReport("Writing out PeaksWorkspace");
      PARALLEL_FOR1(peakWS)
      for(int i =0; i < nPeaks; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        IPeak& peak = peakWS->getPeak(i);
        V3D coords;
        if(mdCoordinates==QLab)
        {
          coords= peakWS->getPeak(i).getQLabFrame();
        }
        else if(mdCoordinates==QSample)
        {
          coords= peakWS->getPeak(i).getQSampleFrame();
        }
        else if(mdCoordinates==Mantid::API::HKL)
        {
          coords= peakWS->getPeak(i).getHKL();
        }

        /* Now find the label corresponding to these coordinates. Use the characteristic coordinates of the coordinates
        recorded earlier to do this. A better implemention would be a direct lookup.
        */
        IsNearPeak nearPeak(coords, radiusEstimate);
        auto iterator = std::find_if(positionMap.begin(), positionMap.end(), nearPeak);
        if(iterator != positionMap.end())
        {
          peak.setIntensity(labelMap[ iterator->second ].get<0>());
          peak.setSigmaIntensity(labelMap[ iterator->second ].get<1>());
        }
        progress.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      setProperty("OutputWorkspace", peakWS);
      setProperty("OutputWorkspaceMD", clusters);
    }



  } // namespace Crystal
} // namespace Mantid
