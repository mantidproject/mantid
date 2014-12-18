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
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
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

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortHKL::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::Input),
        "An input PeaksWorkspace with an instrument.");
    std::vector<std::string> propOptions;
    for (size_t i=0; i<m_pointGroups.size(); ++i)
      propOptions.push_back( m_pointGroups[i]->getName() );
    declareProperty("PointGroup", propOptions[0], boost::make_shared<StringListValidator>(propOptions),
      "Which point group applies to this crystal?");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output),"Output PeaksWorkspace");
    declareProperty("OutputChi2",0.0,"Chi-square is available as output", Direction::Output);
    declareProperty(new WorkspaceProperty<ITableWorkspace>("StatisticsTable","StaticticsTable",Direction::Output),
        "An output table workspace for the statistics of the peaks.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortHKL::exec()
  {

    PeaksWorkspace_sptr InPeaksW = getProperty("InputWorkspace");
    // HKL will be overwritten by equivalent HKL but never seen by user
    PeaksWorkspace_sptr peaksW = getProperty("OutputWorkspace");
    if (peaksW != InPeaksW)
      peaksW = InPeaksW->clone();
    
    // Init a table workspace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
    tablews->addColumn("str", "Resolution Shell");
    tablews->addColumn("int", "No. of Unique Reflections");
    tablews->addColumn("double", "Resolution Min");
    tablews->addColumn("double", "Resolution Max");
    tablews->addColumn("double", "Multiplicity");
    tablews->addColumn("double", "Mean ((I)/sd(I))");
    tablews->addColumn("double", "Rmerge");
    tablews->addColumn("double", "Rpim");
    tablews->addColumn("double", "Data Completeness");

    // append to the table workspace
    API::TableRow newrow = tablews->appendRow();
    newrow << "Overall";

    int NumberPeaks = peaksW->getNumberPeaks();
    for (int i = 0; i < NumberPeaks; i++)
    {
      V3D hkl1 = round(peaksW->getPeaks()[i].getHKL());
      peaksW->getPeaks()[i].setHKL(hkl1);
    }
    //Use the primitive by default
    PointGroup_sptr pointGroup(new PointGroupLaue1());
    //Get it from the property
    std::string pointGroupName = getPropertyValue("PointGroup");
    for (size_t i=0; i<m_pointGroups.size(); ++i)
      if (m_pointGroups[i]->getName() == pointGroupName)
        pointGroup = m_pointGroups[i];

    double Chisq = 0.0;
    std::vector<Peak> &peaks = peaksW->getPeaks();
    for (int i= int(peaksW->getNumberPeaks())-1; i>=0; --i)
    {
      if (peaks[i].getIntensity() == 0.0 || peaksW->getPeaks()[i].getHKL() == V3D(0,0,0))
         peaksW->removePeak(i);
    }
    NumberPeaks = peaksW->getNumberPeaks();
    int equivalent = 0;
    for (int i = 0; i < NumberPeaks; i++)
    {
      V3D hkl1 = peaks[i].getHKL();
      std::string bank1 = peaks[i].getBankName();
      for (int j = i+1; j < NumberPeaks; j++)
      {
        V3D hkl2 = peaks[j].getHKL();
        std::string bank2 = peaks[j].getBankName();
        if (pointGroup->isEquivalent(hkl1,hkl2) && bank1.compare(bank2) == 0)
        {
          peaks[j].setHKL(hkl1);
          equivalent++;
        }
      }
    }
    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by wavelength
    criteria.push_back( std::pair<std::string, bool>("wavelength", true) );
    peaksW->sort(criteria);
    int unique = NumberPeaks - equivalent;
    // table workspace output
    newrow << unique << peaks[0].getWavelength()  << peaks[NumberPeaks-1].getWavelength();

    API::IAlgorithm_sptr predictAlg = createChildAlgorithm("PredictPeaks");
    predictAlg->setProperty("InputWorkspace", InPeaksW);
    predictAlg->setPropertyValue("OutputWorkspace", "predictedPeaks");
    predictAlg->setProperty("WavelengthMin",peaks[0].getWavelength());
    predictAlg->setProperty("WavelengthMax",peaks[NumberPeaks-1].getWavelength());
    // Sort by dspacing
    criteria.push_back( std::pair<std::string, bool>("dspacing", true) );
    peaksW->sort(criteria);
    predictAlg->setProperty("MinDSpacing",peaks[0].getDSpacing());
    predictAlg->executeAsChildAlg();
    PeaksWorkspace_sptr predictedWksp = predictAlg->getProperty("OutputWorkspace");
    int predictedPeaks = predictedWksp->getNumberPeaks();

    criteria.clear();
    // Sort by bank and then HKL
    //criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    criteria.push_back( std::pair<std::string, bool>("H", true) );
    criteria.push_back( std::pair<std::string, bool>("K", true) );
    criteria.push_back( std::pair<std::string, bool>("L", true) );
    peaksW->sort(criteria);

    std::vector<size_t> multiplicity;
    std::vector<double> IsigI;
    for (int i = 0; i < NumberPeaks; i++)
    {
    	IsigI.push_back(peaks[i].getIntensity()/peaks[i].getSigmaIntensity());
    }
    Statistics statsIsigI = getStatistics(IsigI);
    IsigI.clear();

    std::vector<double> data, sig2;
    std::vector<int> peakno;
    double rSum = 0, rpSum = 0, f2Sum = 0;
    V3D hkl1;
    std::string bank1;
    for (int i = 1; i < NumberPeaks; i++)
    {
      hkl1 = peaks[i-1].getHKL();
      bank1 = peaks[i-1].getBankName();
      if(i == 1)
      {
        peakno.push_back(0);
        data.push_back(peaks[i-1].getIntensity());
        sig2.push_back(std::pow(peaks[i-1].getSigmaIntensity(),2));
      }
      V3D hkl2 = peaks[i].getHKL();
      std::string bank2 = peaks[i].getBankName();
      if (hkl1 == hkl2 && bank1.compare(bank2) == 0)
      {
        peakno.push_back(i);
        data.push_back(peaks[i].getIntensity());
        sig2.push_back(std::pow(peaks[i].getSigmaIntensity(),2));
        if(i == NumberPeaks-1)
        {
          if(static_cast<int>(data.size()) > 1)
          {
            Outliers(data,sig2);
            Statistics stats = getStatistics(data);
            Chisq += stats.standard_deviation/stats.mean;
            Statistics stats2 = getStatistics(sig2);
            std::vector<int>::iterator itpk;
            for (itpk = peakno.begin(); itpk != peakno.end(); ++itpk)
            {
              double F2 = peaksW->getPeaks()[*itpk].getIntensity();
              f2Sum += F2;
              rSum += std::fabs(F2 - stats.mean);
              rpSum += std::sqrt(1.0/double(data.size()-1))*std::fabs(F2 - stats.mean);
              peaksW->getPeaks()[*itpk].setIntensity(stats.mean);
              peaksW->getPeaks()[*itpk].setSigmaIntensity(std::sqrt(stats2.mean));
            }
          }
          Outliers(data,sig2);
          multiplicity.push_back(data.size());
          peakno.clear();
          data.clear();
          sig2.clear();
        }
      }
      else
      {
        if(static_cast<int>(data.size()) > 1)
        {
          Outliers(data,sig2);
          Statistics stats = getStatistics(data);
          Chisq += stats.standard_deviation/stats.mean;
          Statistics stats2 = getStatistics(sig2);
          std::vector<int>::iterator itpk;
          for (itpk = peakno.begin(); itpk != peakno.end(); ++itpk)
          {
        	double F2 = peaksW->getPeaks()[*itpk].getIntensity();
        	f2Sum += F2;
            rSum += std::fabs(F2 - stats.mean);
            rpSum += std::sqrt(1.0/double(data.size()-1))*std::fabs(F2 - stats.mean);
            peaksW->getPeaks()[*itpk].setIntensity(stats.mean);
            peaksW->getPeaks()[*itpk].setSigmaIntensity(std::sqrt(stats2.mean));
          }
        }
        multiplicity.push_back(data.size());
        peakno.clear();
        data.clear();
        sig2.clear();
        hkl1 = hkl2;
        bank1 = bank2;
        peakno.push_back(i);
        data.push_back(peaks[i].getIntensity());
        sig2.push_back(std::pow(peaks[i].getSigmaIntensity(),2));
      }
    }
    multiplicity.push_back(data.size());
    Statistics statsMult = getStatistics(multiplicity);
    multiplicity.clear();
    // statistics to output table workspace
    newrow  << statsMult.mean << statsIsigI.mean << 100.0 * rSum / f2Sum << 100.0 * rpSum / f2Sum
        << double(NumberPeaks)/double(predictedPeaks);
    data.clear();
    sig2.clear();
    //Reset hkl of equivalent peaks to original value
    for (int i = 0; i < NumberPeaks; i++)
    {
      peaks[i].resetHKL();
    }
    setProperty("OutputWorkspace", peaksW);
    setProperty("OutputChi2", Chisq);
    setProperty("StatisticsTable", tablews);

  }
  void SortHKL::Outliers(std::vector<double>& data, std::vector<double>& sig2)
  {
      std::vector<double> Zscore = getZscore(data);
      std::vector<size_t> banned;
      for (size_t i = 0; i < data.size(); ++i)
      {
        if (Zscore[i] > 3.0)
        {
          banned.push_back(i);
          continue;
        }
      }
      // delete banned peaks
      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
      {
          data.erase(data.begin() + (*it));
          sig2.erase(sig2.begin() + (*it));
      }
  }

  /** Rounds the V3D to integer values
  * @param hkl the input vector
  * @returns The output V3D
  */
  V3D SortHKL::round(V3D hkl)
  {
          V3D hkl1;
	  hkl1.setX(round(hkl.X()));
	  hkl1.setY(round(hkl.Y()));
	  hkl1.setZ(round(hkl.Z()));
	  return hkl1;
  }

  /** Rounds a double using 0.5 as the cut off for rounding down
  * @param d the input value
  * @returns The output value
  */
  double SortHKL::round(double d)
  {
	  return floor(d + 0.5);
  }

} // namespace Mantid
} // namespace Crystal
