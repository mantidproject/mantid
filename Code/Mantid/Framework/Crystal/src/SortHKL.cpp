/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Statistics.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SortHKL)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SortHKL::SortHKL()
  {
    m_pointGroups = getAllPointGroups();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SortHKL::~SortHKL()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SortHKL::initDocs()
  {
    this->setWikiSummary("Sorts a PeaksWorkspace by HKL. Averages intensities using point group.");
    this->setOptionalMessage("Sorts a PeaksWorkspace by HKL. Averages intensities using point group.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortHKL::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::InOut),
        "An input PeaksWorkspace with an instrument.");
    std::vector<std::string> propOptions;
    for (size_t i=0; i<m_pointGroups.size(); ++i)
      propOptions.push_back( m_pointGroups[i]->getName() );
    declareProperty("PointGroup", propOptions[0],new ListValidator(propOptions),
      "Which point group applies to this crystal?");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output));

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortHKL::exec()
  {

    PeaksWorkspace_sptr InPeaksW = getProperty("InputWorkspace");
    // HKL will be overwritten by equivalent HKL but never seen by user
    PeaksWorkspace_sptr peaksW = InPeaksW->clone();
    peaksW->setName("PeaksByEquivalentHKL");
    
    //Use the primitive by default
    PointGroup_sptr pointGroup(new PointGroupLaue1());
    //Get it from the property
    std::string pointGroupName = getPropertyValue("PointGroup");
    for (size_t i=0; i<m_pointGroups.size(); ++i)
      if (m_pointGroups[i]->getName() == pointGroupName)
        pointGroup = m_pointGroups[i];

    PeaksWorkspace_sptr t = PeaksWorkspace_sptr(new PeaksWorkspace());

    int NumberPeaks = peaksW->getNumberPeaks();
    for (int i = 0; i < NumberPeaks; i++)
    {
      Peak & peak1 = peaksW->getPeaks()[i];
      V3D hkl1 = peak1.getHKL();
      std::string bank1 = peak1.getBankName();
      for (int j = i+1; j < NumberPeaks; j++)
      {
        Peak & peak2 = peaksW->getPeaks()[j];
        V3D hkl2 = peak2.getHKL();
        std::string bank2 = peak2.getBankName();
        if (pointGroup->isEquivalent(hkl1,hkl2) && bank1.compare(bank2) == 0)
          peaksW->getPeaks()[j].setHKL(hkl1);
      }
    }

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    criteria.push_back( std::pair<std::string, bool>("H", true) );
    criteria.push_back( std::pair<std::string, bool>("K", true) );
    criteria.push_back( std::pair<std::string, bool>("L", true) );
    InPeaksW->sort(criteria);
    peaksW->sort(criteria);

    std::vector<double> data, err;
    V3D hkl1;
    std::string bank1;
    for (int i = 1; i < NumberPeaks; i++)
    {
      Peak & peak1 = peaksW->getPeaks()[i-1];
      hkl1 = peak1.getHKL();
      bank1 = peak1.getBankName();
      if(i == 1)
      {
        data.push_back(peak1.getIntensity());
        err.push_back(peak1.getSigmaIntensity());
      }
      Peak & peak2 = peaksW->getPeaks()[i];
      V3D hkl2 = peak2.getHKL();
      std::string bank2 = peak2.getBankName();
      if (pointGroup->isEquivalent(hkl1,hkl2) && bank1.compare(bank2) == 0)
      {
        data.push_back(peak2.getIntensity());
        err.push_back(peak2.getSigmaIntensity());
        if(i == NumberPeaks-1)
        {
          if(static_cast<int>(data.size()) > 0)
          {
            Outliers(data,err);
            Statistics stats = getStatistics(data);
            peak1.setIntensity(stats.mean);
            stats = getStatistics(err);
            peak1.setSigmaIntensity(stats.mean);
            t->addPeak(peak1);
          }
          Outliers(data,err);
          data.clear();
          err.clear();
        }
      }
      else
      {
        if(static_cast<int>(data.size()) > 0)
        {
          Outliers(data,err);
          Statistics stats = getStatistics(data);
          peak1.setIntensity(stats.mean);
          stats = getStatistics(err);
          peak1.setSigmaIntensity(stats.mean);
          t->addPeak(peak1);
        }
        data.clear();
        err.clear();
        hkl1 = hkl2;
        bank1 = bank2;
        data.push_back(peak2.getIntensity());
        err.push_back(peak2.getSigmaIntensity());
      }
    }
    data.clear();
    err.clear();
    setProperty<PeaksWorkspace_sptr>("OutputWorkspace", t);

  }
  void SortHKL::Outliers(std::vector<double>& data, std::vector<double>& err)
  {
      Statistics stats = getStatistics(data);
      if(stats.standard_deviation == 0.)return;
      for (int i = static_cast<int>(data.size())-1; i>=0; i--)
      {
        double zscore = std::fabs((data[i] - stats.mean) / stats.standard_deviation);
        if (zscore > 3.0)
        {
          data.erase(data.begin()+i);
          err.erase(err.begin()+i);
        }
      }
  }


} // namespace Mantid
} // namespace Crystal

