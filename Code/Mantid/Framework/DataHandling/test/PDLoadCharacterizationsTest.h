#ifndef MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_
#define MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/ITableWorkspace.h"

#include "MantidDataHandling/PDLoadCharacterizations.h"
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;

using Mantid::DataHandling::PDLoadCharacterizations;
class PDLoadCharacterizationsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDLoadCharacterizationsTest *createSuite() { return new PDLoadCharacterizationsTest(); }
  static void destroySuite( PDLoadCharacterizationsTest *suite ) { delete suite; }

  void runAlg(PDLoadCharacterizations &alg, ITableWorkspace_sptr &wksp, const std::string &filename)
  {
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    // run the algorithm
    alg.setProperty("Filename", filename);
    alg.setPropertyValue("OutputWorkspace", filename);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // test the table workspace
    wksp = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
        (Mantid::API::AnalysisDataService::Instance().retrieve(filename));
    TS_ASSERT(wksp);
  }

  // checks the focus positions for NOMAD
  void checkNOMAD(PDLoadCharacterizations &alg)
  {
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"), std::string("NOMAD_11_22_11.prm"));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, 19.5);

    const int NUM_SPEC = 6;

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), NUM_SPEC);
    if (!specIds.empty())
    {
      for (int i = 0; i < NUM_SPEC; ++i)
        TS_ASSERT_EQUALS(specIds[i], i+1);
    }

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), NUM_SPEC);
    if (!l2.empty())
    {
      for (int i = 0; i < NUM_SPEC; ++i)
        TS_ASSERT_EQUALS(l2[i], 2.);
    }

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), NUM_SPEC);
    if (!polar.empty())
    {
      TS_ASSERT_EQUALS(polar[0], 15.);
      TS_ASSERT_EQUALS(polar[1], 31.);
      TS_ASSERT_EQUALS(polar[2], 67.);
      TS_ASSERT_EQUALS(polar[3], 122.);
      TS_ASSERT_EQUALS(polar[4], 154.);
      TS_ASSERT_EQUALS(polar[5], 7.);
    }

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), NUM_SPEC);
    if (!azi.empty())
    {
      for (int i = 0; i < NUM_SPEC; ++i)
        TS_ASSERT_EQUALS(azi[0], 0.);
    }
  }

  void checkPG3(ITableWorkspace_sptr &wksp)
  {
    TS_ASSERT_EQUALS(wksp->columnCount(), 10);
    TS_ASSERT_EQUALS(wksp->rowCount(), 6);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0,0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0,1), 0.900);
    TS_ASSERT_EQUALS(wksp->Int(0,2), 1);
    TS_ASSERT_EQUALS(wksp->Int(0,3), 15030);
    TS_ASSERT_EQUALS(wksp->Int(0,4), 15039);
    TS_ASSERT_EQUALS(wksp->Int(0,5), 0);
    TS_ASSERT_EQUALS(wksp->String(0,6), "0.20");
    TS_ASSERT_EQUALS(wksp->String(0,7), "4.12");
    TS_ASSERT_EQUALS(wksp->Double(0,8), 4700.);
    TS_ASSERT_EQUALS(wksp->Double(0,9), 21200.);

    // check all of the contents of row 5
    TS_ASSERT_EQUALS(wksp->Double(5,0), 10.);
    TS_ASSERT_EQUALS(wksp->Double(5,1), 3.198);
    TS_ASSERT_EQUALS(wksp->Int(5,2), 1);
    TS_ASSERT_EQUALS(wksp->Int(5,3), 15033);
    TS_ASSERT_EQUALS(wksp->Int(5,4), 15042);
    TS_ASSERT_EQUALS(wksp->Int(5,5), 0);
    TS_ASSERT_EQUALS(wksp->String(5,6), "0.05");
    TS_ASSERT_EQUALS(wksp->String(5,7), "15.40");
    TS_ASSERT_EQUALS(wksp->Double(5,8), 0.);
    TS_ASSERT_EQUALS(wksp->Double(5,9), 100000.);
  }

  void test_Init()
  {
    PDLoadCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_FocusAndChar()
  {
    const std::string CHAR_FILE("Test_characterizations_focus_and_char.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    checkPG3(wksp);

    // test the other output properties
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"), std::string("dummy.iparm"));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, 60.);

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), 1);
    if (!specIds.empty())
      TS_ASSERT_EQUALS(specIds[0], 1);

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), 1);
    if (!l2.empty())
      TS_ASSERT_EQUALS(l2[0], 3.18);

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), 1);
    if (!polar.empty())
      TS_ASSERT_EQUALS(polar[0], 90.);

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), 1);
    if (!azi.empty())
      TS_ASSERT_EQUALS(azi[0], 0.);
  }

  void test_FocusAndChar2()
  {
    const std::string CHAR_FILE("Test_characterizations_focus_and_char2.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 10);
    TS_ASSERT_EQUALS(wksp->rowCount(), 1);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0,0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0,1), 1.4);
    TS_ASSERT_EQUALS(wksp->Int(0,2), 1);
    TS_ASSERT_EQUALS(wksp->Int(0,3), 0);
    TS_ASSERT_EQUALS(wksp->Int(0,4), 0);
    TS_ASSERT_EQUALS(wksp->Int(0,5), 0);
    TS_ASSERT_EQUALS(wksp->String(0,6), ".31,.25,.13,.13,.13,.42");
    TS_ASSERT_EQUALS(wksp->String(0,7), "13.66,5.83,3.93,2.09,1.57,31.42");
    TS_ASSERT_EQUALS(wksp->Double(0,8), 300.00);
    TS_ASSERT_EQUALS(wksp->Double(0,9), 16666.67);

    // test the other output properties
    checkNOMAD(alg);
  }

  void test_Focus()
  {
    const std::string CHAR_FILE("Test_characterizations_focus.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 10);
    TS_ASSERT_EQUALS(wksp->rowCount(), 0);

    // test the other output properties
    checkNOMAD(alg);
  }

  void test_Char()
  {
    const std::string CHAR_FILE("Test_characterizations_char.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    checkPG3(wksp);

    // test the other output properties
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"), std::string(""));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, 0.);

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), 0);

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), 0);

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), 0);

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), 0);
  }
};


#endif /* MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_ */
