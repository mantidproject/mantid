/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Statistics.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
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
    this->setWikiSummary("Sorts a PeaksWorkspace by HKL.");
    this->setOptionalMessage("Sorts a PeaksWorkspace by HKL.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortHKL::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::InOut),
        "An input PeaksWorkspace with an instrument.");
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("DuplicatesStatisticsTable","Statistics",Direction::Output));

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortHKL::exec()
  {

    PeaksWorkspace_sptr peaksW = getProperty("InputWorkspace");
    API::ITableWorkspace_sptr t = WorkspaceFactory::Instance().createTable("TableWorkspace");
    t->addColumn("double","h");
    t->addColumn("double","k");
    t->addColumn("double","l");
    t->addColumn("int","duplicates");
    t->addColumn("double","Intensity.mean");
    t->addColumn("double","Intensity.std");
    t->addColumn("double","Intensity.min");
    t->addColumn("double","Intensity.max");
    t->addColumn("double","Intensity.median");
    t->addColumn("double","Sigma.mean");
    t->addColumn("double","Sigma.std");
    t->addColumn("double","Sigma.min");
    t->addColumn("double","Sigma.max");
    t->addColumn("double","Sigma.median");

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("H", true) );
    criteria.push_back( std::pair<std::string, bool>("K", true) );
    criteria.push_back( std::pair<std::string, bool>("L", true) );
    peaksW->sort(criteria);

    int NumberPeaks = peaksW->getNumberPeaks();
    std::vector<double> data, err;
    V3D hkl1;
    for (int i = 1; i < NumberPeaks; i++)
    {
      Peak & peak1 = peaksW->getPeaks()[i-1];
      hkl1 = peak1.getHKL();
      if(i == 1)
      {
        data.push_back(peak1.getIntensity());
        err.push_back(peak1.getSigmaIntensity());
      }
      Peak & peak2 = peaksW->getPeaks()[i];
      V3D hkl2 = peak2.getHKL();
      if (hkl1 == hkl2)
      {
        data.push_back(peak2.getIntensity());
        err.push_back(peak2.getSigmaIntensity());
        if(i == NumberPeaks-1 && data.size()>1)
        {
          Statistics stats = getStatistics(data);
          TableRow r = t->appendRow();
          r <<peak1.getH()<<peak1.getK()<<peak1.getL()<<static_cast<int>(data.size())<<stats.mean<< stats.standard_deviation<< stats.minimum<< stats.maximum<< stats.median;
          stats = getStatistics(err);
          r <<stats.mean<< stats.standard_deviation<< stats.minimum<< stats.maximum<< stats.median;
          data.clear();
          err.clear();
        }
      }
      else
      {
        if(data.size()>1)
        {
          Statistics stats = getStatistics(data);
          TableRow r = t->appendRow();
          r <<peak1.getH()<<peak1.getK()<<peak1.getL()<<static_cast<int>(data.size())<<stats.mean<< stats.standard_deviation<< stats.minimum<< stats.maximum<< stats.median;
          stats = getStatistics(err);
          r <<stats.mean<< stats.standard_deviation<< stats.minimum<< stats.maximum<< stats.median;
        }
        data.clear();
        err.clear();
        hkl1 = hkl2;
        data.push_back(peak1.getIntensity());
        err.push_back(peak1.getSigmaIntensity());
      }
    }
    data.clear();
    err.clear();
    setProperty("DuplicatesStatisticsTable",t);

  }


} // namespace Mantid
} // namespace Crystal

