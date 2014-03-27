/*WIKI*
Handles integration of arbitary single crystal peaks shapes.

Uses connected component analysis to integrate peaks in an PeaksWorkspace over an MDHistoWorkspace of data.
*WIKI*/

#include "MantidCrystal/IntegratePeaksUsingClusters.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Utils.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace
{
  class PeakBackground : public Mantid::Crystal::HardThresholdBackground
  {
  private:

    IPeaksWorkspace_const_sptr m_peaksWS;
    const double m_radiusEstimate;
    const SpecialCoordinateSystem m_mdCoordinates;

  public:
    PeakBackground(IPeaksWorkspace_const_sptr peaksWS, const double& radiusEstimate, const double& thresholdSignal, const Mantid::API::MDNormalization normalisation, const SpecialCoordinateSystem coordinates) 
      : Mantid::Crystal::HardThresholdBackground(thresholdSignal, normalisation), m_peaksWS(peaksWS), m_radiusEstimate(radiusEstimate), m_mdCoordinates(coordinates)
    {
    }

    virtual bool isBackground(Mantid::API::IMDIterator* iterator) const
    {
      if(!HardThresholdBackground::isBackground(iterator) )
      {
        for(size_t i = 0; i < m_peaksWS->getNumberPeaks(); ++i)
        {
          V3D coords;
          if(m_mdCoordinates==QLab)
          {
            coords= m_peaksWS->getPeak(i).getQLabFrame();
          }
          else if(m_mdCoordinates==QSample)
          {
            coords= m_peaksWS->getPeak(i).getQSampleFrame();
          }
          else if(m_mdCoordinates==Mantid::API::HKL)
          {
            coords= m_peaksWS->getPeak(i).getHKL();
          }
          const VMD& center = iterator->getCenter();
          V3D temp(center[0], center[1], center[2]);
          if(coords.distance(temp) < m_radiusEstimate)
          {
            return false;
          }
        }

      }
      return true;
    }

    void configureIterator(Mantid::API::IMDIterator* const iterator) const
    {
    }

    virtual ~PeakBackground()
    {
    }
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
      std::vector<std::string> propOptions;
      propOptions.push_back("NoNormalization");
      propOptions.push_back("NumberOfEventsNormalization");
      propOptions.push_back("VolumeNormalization");
      declareProperty("Normalization", "NoNormalization",boost::make_shared<StringListValidator>(propOptions),
        "Normalization to use."
        );

      declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","", Direction::Input),"A PeaksWorkspace containing the peaks to integrate.");
      declareProperty(new PropertyWithValue<double>("RadiusEstimate", 0.1, Direction::Input), "Estimate of Peak Radius. This is optional and should not affect the ouput of the integration, but it will remove noise from the resulting MD cluster image.");
      declareProperty(new PropertyWithValue<double>("Threshold", 0, Direction::Input), "Threshold");
      //declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "An output integrated peaks workspace."); 
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspaceMD","",Direction::Output), "MDHistoWorkspace containing the clusters.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void IntegratePeaksUsingClusters::exec()
    {
      IMDHistoWorkspace_sptr mdWS = getProperty("InputWorkspace");
      PeaksWorkspace_sptr peaksWS = getProperty("PeaksWorkspace");

      const SpecialCoordinateSystem mdCoordinates = mdWS->getSpecialCoordinateSystem();


      std::string strNormalization = getPropertyValue("Normalization");
      MDNormalization normalization = NoNormalization;
      if(strNormalization == "NumberOfEventsNormalization")
      {
        normalization = NumEventsNormalization;
      }
      else if(strNormalization == "VolumeNormalization")
      {
        normalization = VolumeNormalization;
      }

      const double threshold = getProperty("Threshold");
      const double radiusEstimate = getProperty("RadiusEstimate");
      PeakBackground background(peaksWS, radiusEstimate, threshold, normalization, mdCoordinates);
      //HardThresholdBackground background(threshold, normalization);

      ConnectedComponentLabeling analysis;
      IMDHistoWorkspace_sptr clusters = analysis.execute(mdWS, &background);

      //setProperty("OutputWorkspace", peaksWS);
      setProperty("OutputWorkspaceMD", clusters);
    }



  } // namespace Crystal
} // namespace Mantid