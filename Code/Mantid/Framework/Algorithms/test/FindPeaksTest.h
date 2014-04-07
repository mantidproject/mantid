#ifndef FINDPEAKSTEST_H_
#define FINDPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <fstream>

using Mantid::Algorithms::FindPeaks;

using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

class FindPeaksTest : public CxxTest::TestSuite
{
public:
  static FindPeaksTest *createSuite() { return new FindPeaksTest(); }
  static void destroySuite( FindPeaksTest *suite ) { delete suite; }

  FindPeaksTest()
  {
    FrameworkManager::Instance();
  }

  /// Test basic functions
  void test_TheBasics()
  {
    FindPeaks finder;
    TS_ASSERT_EQUALS( finder.name(), "FindPeaks" );
    TS_ASSERT_EQUALS( finder.version(), 1 );
  }

  /// Test initialization
  void testInit()
  {
    FindPeaks finder;
    TS_ASSERT_THROWS_NOTHING( finder.initialize() );
    TS_ASSERT( finder.isInitialized() );
  }

  //----------------------------------------------------------------------------------------------
  /** Test find a single peak with given position
    */
  void test_findSinglePeakGivenPeakPosition()
  {
    FrameworkManager::Instance();

    MatrixWorkspace_sptr dataws = getSinglePeakData();
    std::string wsname("SinglePeakTestData");
    AnalysisDataService::Instance().addOrReplace(wsname, dataws);

    FindPeaks finder1;
    finder1.initialize();
    TS_ASSERT(finder1.isInitialized());

    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("WorkspaceIndex","0"));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("Tolerance", 4));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("FWHM", 8));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("PeakPositions", "1.2356"));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("FitWindows", "1.21, 1.50"));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("HighBackground", true));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("MinGuessedPeakWidth", 2));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("MaxGuessedPeakWidth", 10));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("PeakPositionTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING( finder1.setProperty("RawPeakParameters", true));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("CostFunction", "Chi-Square"));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("Minimizer", "Levenberg-MarquardtMD"));
    TS_ASSERT_THROWS_NOTHING( finder1.setPropertyValue("PeaksList","FoundedSinglePeakTable"));

    TS_ASSERT_THROWS_NOTHING( finder1.execute() );

    TS_ASSERT( finder1.isExecuted() );

    // Get output workspace
    TableWorkspace_sptr outtablews = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("FoundedSinglePeakTable"));
    TS_ASSERT(outtablews);

    // Size of the output workspace
    TS_ASSERT_EQUALS(outtablews->rowCount(), 1);
    if (outtablews->rowCount() == 0)
      return;

    map<string, double> parammap;
    getParameterMap(outtablews, 0, parammap);

    TS_ASSERT_DELTA(parammap["PeakCentre"], 1.2356, 0.03);
    TS_ASSERT_DELTA(parammap["Height"], 595., 3.00);

    // Clean
    AnalysisDataService::Instance().remove(wsname);
    AnalysisDataService::Instance().remove("FoundedSinglePeakTable");
  }

  //----------------------------------------------------------------------------------------------
  /** Test find peaks automaticallyclear
    */
  void test_findMultiPeaksAuto()
  {
    // Load data file
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","focussed.nxs");
    loader.setProperty("OutputWorkspace", "FindPeaksTest_peaksWS");
    loader.execute();

    // Find peaks (Test)
    FindPeaks finder;
    if ( !finder.isInitialized() ) finder.initialize();

    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("InputWorkspace","FindPeaksTest_peaksWS") );
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("WorkspaceIndex","4") );
    // TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("SmoothedData","smoothed") );
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("PeaksList","FindPeaksTest_foundpeaks") );

    TS_ASSERT_THROWS_NOTHING( finder.execute() );
    TS_ASSERT( finder.isExecuted() );

    Mantid::API::ITableWorkspace_sptr peaklist = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
                  (Mantid::API::AnalysisDataService::Instance().retrieve("FindPeaksTest_foundpeaks"));

    TS_ASSERT( peaklist );
    TS_ASSERT_EQUALS( peaklist->rowCount() , 9 );
    TS_ASSERT_DELTA( peaklist->Double(1,1), 0.59, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(2,1), 0.71, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(3,1), 0.81, 0.01 );
    // This is a dodgy value, that comes out different on different platforms
    //TS_ASSERT_DELTA( peaklist->Double(3,1), 1.03, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(5,1), 0.96, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(6,1), 1.24, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(7,1), 1.52, 0.01 );
    TS_ASSERT_DELTA( peaklist->Double(8,1), 2.14, 0.01 );

  }

  void NtestFindMultiPeaksGivenPeaksList()
  {
    this->LoadPG3_733();

    FindPeaks finder;
    if ( !finder.isInitialized() ) finder.initialize();
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("InputWorkspace","FindPeaksTest_vanadium") );
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("WorkspaceIndex","0") );
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("PeakPositions", "0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401") );
//    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("SmoothedData","ignored_smoothed_data") );
    TS_ASSERT_THROWS_NOTHING( finder.setPropertyValue("PeaksList","FindPeaksTest_foundpeaks2") );

    TS_ASSERT_THROWS_NOTHING( finder.execute() );
    TS_ASSERT( finder.isExecuted() );

  }

  //----------------------------------------------------------------------------------------------
  /** Parse a row in output parameter tableworkspace to a string/double parameter name/value map
    */
  void getParameterMap(TableWorkspace_sptr tablews, size_t rowindex, map<string, double>& parammap)
  {
    parammap.clear();

    vector<string> vecnames = tablews->getColumnNames();
    /*
    for (size_t i = 0; i < vecnames.size(); ++i)
      cout << "Column " << i << " : " << vecnames[i] << "\n";

    size_t numrows = tablews->rowCount();
    cout << "Number of rows = " << numrows << "\n";
    */

    for (size_t i = 0; i < vecnames.size(); ++i)
    {
      string parname = vecnames[i];
      if (parname != "spectrum")
      {
        double parvalue = tablews->cell<double>(rowindex, i);
        parammap.insert(make_pair(parname, parvalue));
        cout << "Add parameter " << parname << " = " << parvalue << "\n";
      }
    }

    return;
  }

  /// Load PG3_733 focussed data from AutoTest
  void LoadPG3_733()
  {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","PG3_733_focussed.nxs");
    loader.setProperty("OutputWorkspace", "FindPeaksTest_vanadium");
    loader.execute();
  }

  //----------------------------------------------------------------------------------------------
  /** Create a workspace as a partial data from PG3_4866 around Vanadium peak at d = 1.235
    */
  MatrixWorkspace_sptr getSinglePeakData()
  {
    std::vector<double> vecX, vecY, vecE;
    vecX.push_back(1.21012 );  vecY.push_back(1619);   vecE.push_back(40.2368);
    vecX.push_back(1.2106  );  vecY.push_back(1644);   vecE.push_back(40.5463);
    vecX.push_back(1.21108 );  vecY.push_back(1616);   vecE.push_back(40.1995);
    vecX.push_back(1.21157 );  vecY.push_back(1589);   vecE.push_back(39.8623);
    vecX.push_back(1.21205 );  vecY.push_back(1608);   vecE.push_back(40.0999);
    vecX.push_back(1.21254 );  vecY.push_back(1612);   vecE.push_back(40.1497);
    vecX.push_back(1.21302 );  vecY.push_back(1630);   vecE.push_back(40.3733);
    vecX.push_back(1.21351 );  vecY.push_back(1671);   vecE.push_back(40.8779);
    vecX.push_back(1.21399 );  vecY.push_back(1588);   vecE.push_back(39.8497);
    vecX.push_back(1.21448 );  vecY.push_back(1577);   vecE.push_back(39.7115);
    vecX.push_back(1.21497 );  vecY.push_back(1616);   vecE.push_back(40.1995);
    vecX.push_back(1.21545 );  vecY.push_back(1556);   vecE.push_back(39.4462);
    vecX.push_back(1.21594 );  vecY.push_back(1625);   vecE.push_back(40.3113);
    vecX.push_back(1.21642 );  vecY.push_back(1655);   vecE.push_back(40.6817);
    vecX.push_back(1.21691 );  vecY.push_back(1552);   vecE.push_back(39.3954);
    vecX.push_back(1.2174  );  vecY.push_back(1539);   vecE.push_back(39.2301);
    vecX.push_back(1.21788 );  vecY.push_back(1538);   vecE.push_back(39.2173);
    vecX.push_back(1.21837 );  vecY.push_back(1542);   vecE.push_back(39.2683);
    vecX.push_back(1.21886 );  vecY.push_back(1558);   vecE.push_back(39.4715);
    vecX.push_back(1.21935 );  vecY.push_back(1628);   vecE.push_back(40.3485);
    vecX.push_back(1.21983 );  vecY.push_back(1557);   vecE.push_back(39.4588);
    vecX.push_back(1.22032 );  vecY.push_back(1606);   vecE.push_back(40.0749);
    vecX.push_back(1.22081 );  vecY.push_back(1563);   vecE.push_back(39.5348);
    vecX.push_back(1.2213  );  vecY.push_back(1611);   vecE.push_back(40.1373);
    vecX.push_back(1.22179 );  vecY.push_back(1584);   vecE.push_back(39.7995);
    vecX.push_back(1.22228 );  vecY.push_back(1447);   vecE.push_back(38.0395);
    vecX.push_back(1.22276 );  vecY.push_back(1532);   vecE.push_back(39.1408);
    vecX.push_back(1.22325 );  vecY.push_back(1580);   vecE.push_back(39.7492);
    vecX.push_back(1.22374 );  vecY.push_back(1539);   vecE.push_back(39.2301);
    vecX.push_back(1.22423 );  vecY.push_back(1513);   vecE.push_back(38.8973);
    vecX.push_back(1.22472 );  vecY.push_back(1601);   vecE.push_back(40.0125);
    vecX.push_back(1.22521 );  vecY.push_back(1558);   vecE.push_back(39.4715);
    vecX.push_back(1.2257  );  vecY.push_back(1567);   vecE.push_back(39.5854);
    vecX.push_back(1.22619 );  vecY.push_back(1573);   vecE.push_back(39.6611);
    vecX.push_back(1.22668 );  vecY.push_back(1551);   vecE.push_back(39.3827);
    vecX.push_back(1.22717 );  vecY.push_back(1465);   vecE.push_back(38.2753);
    vecX.push_back(1.22766 );  vecY.push_back(1602);   vecE.push_back(40.025);
    vecX.push_back(1.22816 );  vecY.push_back(1543);   vecE.push_back(39.281);
    vecX.push_back(1.22865 );  vecY.push_back(1538);   vecE.push_back(39.2173);
    vecX.push_back(1.22914 );  vecY.push_back(1515);   vecE.push_back(38.923);
    vecX.push_back(1.22963 );  vecY.push_back(1556);   vecE.push_back(39.4462);
    vecX.push_back(1.23012 );  vecY.push_back(1574);   vecE.push_back(39.6737);
    vecX.push_back(1.23061 );  vecY.push_back(1519);   vecE.push_back(38.9744);
    vecX.push_back(1.23111 );  vecY.push_back(1452);   vecE.push_back(38.1051);
    vecX.push_back(1.2316  );  vecY.push_back(1568);   vecE.push_back(39.598);
    vecX.push_back(1.23209 );  vecY.push_back(1522);   vecE.push_back(39.0128);
    vecX.push_back(1.23258 );  vecY.push_back(1518);   vecE.push_back(38.9615);
    vecX.push_back(1.23308 );  vecY.push_back(1603);   vecE.push_back(40.0375);
    vecX.push_back(1.23357 );  vecY.push_back(1538);   vecE.push_back(39.2173);
    vecX.push_back(1.23406 );  vecY.push_back(1659);   vecE.push_back(40.7308);
    vecX.push_back(1.23456 );  vecY.push_back(1685);   vecE.push_back(41.0488);
    vecX.push_back(1.23505 );  vecY.push_back(1763);   vecE.push_back(41.9881);
    vecX.push_back(1.23554 );  vecY.push_back(1846);   vecE.push_back(42.9651);
    vecX.push_back(1.23604 );  vecY.push_back(1872);   vecE.push_back(43.2666);
    vecX.push_back(1.23653 );  vecY.push_back(2018);   vecE.push_back(44.9222);
    vecX.push_back(1.23703 );  vecY.push_back(2035);   vecE.push_back(45.111);
    vecX.push_back(1.23752 );  vecY.push_back(2113);   vecE.push_back(45.9674);
    vecX.push_back(1.23802 );  vecY.push_back(2131);   vecE.push_back(46.1628);
    vecX.push_back(1.23851 );  vecY.push_back(1921);   vecE.push_back(43.8292);
    vecX.push_back(1.23901 );  vecY.push_back(1947);   vecE.push_back(44.1248);
    vecX.push_back(1.2395  );  vecY.push_back(1756);   vecE.push_back(41.9047);
    vecX.push_back(1.24    );  vecY.push_back(1603);   vecE.push_back(40.0375);
    vecX.push_back(1.2405  );  vecY.push_back(1602);   vecE.push_back(40.025);
    vecX.push_back(1.24099 );  vecY.push_back(1552);   vecE.push_back(39.3954);
    vecX.push_back(1.24149 );  vecY.push_back(1558);   vecE.push_back(39.4715);
    vecX.push_back(1.24199 );  vecY.push_back(1518);   vecE.push_back(38.9615);
    vecX.push_back(1.24248 );  vecY.push_back(1512);   vecE.push_back(38.8844);
    vecX.push_back(1.24298 );  vecY.push_back(1511);   vecE.push_back(38.8716);
    vecX.push_back(1.24348 );  vecY.push_back(1466);   vecE.push_back(38.2884);
    vecX.push_back(1.24397 );  vecY.push_back(1474);   vecE.push_back(38.3927);
    vecX.push_back(1.24447 );  vecY.push_back(1368);   vecE.push_back(36.9865);
    vecX.push_back(1.24497 );  vecY.push_back(1463);   vecE.push_back(38.2492);
    vecX.push_back(1.24547 );  vecY.push_back(1447);   vecE.push_back(38.0395);
    vecX.push_back(1.24597 );  vecY.push_back(1409);   vecE.push_back(37.5366);
    vecX.push_back(1.24646 );  vecY.push_back(1381);   vecE.push_back(37.1618);
    vecX.push_back(1.24696 );  vecY.push_back(1478);   vecE.push_back(38.4448);
    vecX.push_back(1.24746 );  vecY.push_back(1445);   vecE.push_back(38.0132);
    vecX.push_back(1.24796 );  vecY.push_back(1429);   vecE.push_back(37.8021);
    vecX.push_back(1.24846 );  vecY.push_back(1447);   vecE.push_back(38.0395);
    vecX.push_back(1.24896 );  vecY.push_back(1354);   vecE.push_back(36.7967);
    vecX.push_back(1.24946 );  vecY.push_back(1430);   vecE.push_back(37.8153);
    vecX.push_back(1.24996 );  vecY.push_back(1440);   vecE.push_back(37.9473);
    vecX.push_back(1.25046 );  vecY.push_back(1423);   vecE.push_back(37.7227);

    size_t sizex = vecX.size();
    size_t sizey = vecY.size();

    MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey));

    for (size_t i = 0; i < sizex; ++i)
        dataws->dataX(0)[i] = vecX[i];

    for (size_t i = 0; i < sizex; ++i)
    {
        dataws->dataY(0)[i] = vecY[i];
        dataws->dataE(0)[i] = vecE[i];
    }

    return dataws;
  }


private:
};

//=================================================================================================
/** Performance test with large workspaces.
  */

class FindPeaksTestPerformance : public CxxTest::TestSuite
{
  Mantid::API::MatrixWorkspace_sptr m_dataWS;

public:
  static FindPeaksTestPerformance *createSuite() { return new FindPeaksTestPerformance(); }
  static void destroySuite( FindPeaksTestPerformance *suite ) { delete suite; }

  /** Constructor
    */
  FindPeaksTestPerformance()
  {

  }

  /** Set up workspaces
    */
  void setUp()
  {
    // Load data file
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","focussed.nxs");
    loader.setProperty("OutputWorkspace", "FindPeaksTest_peaksWS");
    loader.execute();

    m_dataWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve("FindPeaksTest_peaksWS"));

    return;
  }

  /** Find peaks by auto-determine peaks' positions
    */
  void test_FindPeaksAutoPeakPositions()
  {
    // Find peaks (Test)
    FindPeaks finder;
    if ( !finder.isInitialized() ) finder.initialize();

    if (!m_dataWS)
      throw std::runtime_error("Unable to get input matrix workspace. ");
    finder.setPropertyValue("InputWorkspace","FindPeaksTest_peaksWS");
    finder.setPropertyValue("PeakPositions", "0.8089, 0.9571, 1.0701,1.2356,1.5133,2.1401");
    finder.setPropertyValue("PeaksList","FindPeaksTest_foundpeaks");

    finder.execute();
  }

}; // end of class FindPeaksTestPerformance


#endif /*FINDPEAKSTEST_H_*/
