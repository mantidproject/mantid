// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_
#define MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MantidDataHandling/PDLoadCharacterizations.h"
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;

using Mantid::DataHandling::PDLoadCharacterizations;
class PDLoadCharacterizationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDLoadCharacterizationsTest *createSuite() {
    return new PDLoadCharacterizationsTest();
  }
  static void destroySuite(PDLoadCharacterizationsTest *suite) { delete suite; }

  void runAlg(PDLoadCharacterizations &alg, ITableWorkspace_sptr &wksp,
              const std::string &filename) {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // run the algorithm
    alg.setPropertyValue("Filename", filename);
    alg.setPropertyValue("OutputWorkspace", filename);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // test the table workspace
    wksp = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(filename));
    TS_ASSERT(wksp);
  }

  // checks the focus positions for NOMAD
  void checkNOMAD(PDLoadCharacterizations &alg, bool checkAziValues = true) {
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"),
                     std::string("NOMAD_11_22_11.prm"));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, 19.5);

    const int NUM_SPEC = 6;

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), NUM_SPEC);
    if (!specIds.empty()) {
      for (int i = 0; i < NUM_SPEC; ++i)
        TS_ASSERT_EQUALS(specIds[i], i + 1);
    }

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), NUM_SPEC);
    if (!l2.empty()) {
      for (const auto &value : l2)
        TS_ASSERT_EQUALS(value, 2.);
    }

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), NUM_SPEC);
    if (!polar.empty()) {
      TS_ASSERT_EQUALS(polar[0], 15.);
      TS_ASSERT_EQUALS(polar[1], 31.);
      TS_ASSERT_EQUALS(polar[2], 67.);
      TS_ASSERT_EQUALS(polar[3], 122.);
      TS_ASSERT_EQUALS(polar[4], 154.);
      TS_ASSERT_EQUALS(polar[5], 7.);
    }

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), NUM_SPEC);
    if (checkAziValues && !azi.empty()) {
      for (const auto &value : azi)
        TS_ASSERT_EQUALS(value, 0.);
    }
  }

  void checkPG3(ITableWorkspace_sptr &wksp) {
    TS_ASSERT_EQUALS(wksp->columnCount(), 14);
    TS_ASSERT_EQUALS(wksp->rowCount(), 6);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0, 1), 0.900);
    TS_ASSERT_EQUALS(wksp->Int(0, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(0, 3), "15030");
    TS_ASSERT_EQUALS(wksp->String(0, 4), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 5), "15039");
    TS_ASSERT_EQUALS(wksp->String(0, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 8), "0.20");
    TS_ASSERT_EQUALS(wksp->String(0, 9), "4.12");
    TS_ASSERT_EQUALS(wksp->Double(0, 10), 4700.);
    TS_ASSERT_EQUALS(wksp->Double(0, 11), 21200.);

    // check all of the contents of row 5
    TS_ASSERT_EQUALS(wksp->Double(5, 0), 10.);
    TS_ASSERT_EQUALS(wksp->Double(5, 1), 3.198);
    TS_ASSERT_EQUALS(wksp->Int(5, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(5, 3), "15033");
    TS_ASSERT_EQUALS(wksp->String(5, 4), "0");
    TS_ASSERT_EQUALS(wksp->String(5, 5), "15042");
    TS_ASSERT_EQUALS(wksp->String(5, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(5, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(5, 8), "0.05");
    TS_ASSERT_EQUALS(wksp->String(5, 9), "15.40");
    TS_ASSERT_EQUALS(wksp->Double(5, 10), 0.);
    TS_ASSERT_EQUALS(wksp->Double(5, 11), 100000.);
  }

  void checkPG3WithContainers(ITableWorkspace_sptr &wksp) {
    TS_ASSERT_EQUALS(wksp->columnCount(), 17);
    TS_ASSERT_EQUALS(wksp->rowCount(), 8);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0, 1), 0.900);
    TS_ASSERT_EQUALS(wksp->Int(0, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(0, 3), "15030");
    TS_ASSERT_EQUALS(wksp->String(0, 4), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 5), "15039");
    TS_ASSERT_EQUALS(wksp->String(0, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 8), "0.20");
    TS_ASSERT_EQUALS(wksp->String(0, 9), "4.12");
    TS_ASSERT_EQUALS(wksp->Double(0, 10), 4700.);
    TS_ASSERT_EQUALS(wksp->Double(0, 11), 21200.);
    TS_ASSERT_EQUALS(wksp->Double(0, 12), 0.);
    TS_ASSERT_EQUALS(wksp->Double(0, 13), 0.);
    TS_ASSERT_EQUALS(wksp->String(0, 14), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 15), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 16), "0");

    // check all of the contents of row 4
    TS_ASSERT_EQUALS(wksp->Double(4, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(4, 1), 4.797);
    TS_ASSERT_EQUALS(wksp->Int(4, 2), 5);
    TS_ASSERT_EQUALS(wksp->String(4, 3), "27061");
    TS_ASSERT_EQUALS(wksp->String(4, 4), "27055");
    TS_ASSERT_EQUALS(wksp->String(4, 5), "15085");
    TS_ASSERT_EQUALS(wksp->String(4, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(4, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(4, 8), "2.00");
    TS_ASSERT_EQUALS(wksp->String(4, 9), "15.35");
    TS_ASSERT_EQUALS(wksp->Double(4, 10), 66666.67);
    TS_ASSERT_EQUALS(wksp->Double(4, 11), 83333.67);
    TS_ASSERT_EQUALS(wksp->Double(4, 12), 0.);
    TS_ASSERT_EQUALS(wksp->Double(4, 13), 0.);
    TS_ASSERT_EQUALS(wksp->String(4, 14), "27049");
    TS_ASSERT_EQUALS(wksp->String(4, 15), "27037");
    TS_ASSERT_EQUALS(wksp->String(4, 16), "27043");

    // check all of the contents of row 7
    TS_ASSERT_EQUALS(wksp->Double(7, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(7, 1), 3.731);
    TS_ASSERT_EQUALS(wksp->Int(7, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(7, 3), "27060");
    TS_ASSERT_EQUALS(wksp->String(7, 4), "27054");
    TS_ASSERT_EQUALS(wksp->String(7, 5), "0");
    TS_ASSERT_EQUALS(wksp->String(7, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(7, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(7, 8), "0");
    TS_ASSERT_EQUALS(wksp->String(7, 9), "0");
    TS_ASSERT_EQUALS(wksp->Double(7, 10), 0.);
    TS_ASSERT_EQUALS(wksp->Double(7, 11), 0.);
    TS_ASSERT_EQUALS(wksp->Double(7, 12), 0.);
    TS_ASSERT_EQUALS(wksp->Double(7, 13), 0.);
    TS_ASSERT_EQUALS(wksp->String(7, 14), "27048");
    TS_ASSERT_EQUALS(wksp->String(7, 15), "27036");
    TS_ASSERT_EQUALS(wksp->String(7, 16), "27042");
  }

  void test_Init() {
    PDLoadCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_FocusAndChar() {
    const std::string CHAR_FILE("Test_characterizations_focus_and_char.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    checkPG3(wksp);

    // test the other output properties
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"),
                     std::string("dummy.iparm"));
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

  void test_FocusAndChar2() {
    const std::string CHAR_FILE("Test_characterizations_focus_and_char2.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 14);
    TS_ASSERT_EQUALS(wksp->rowCount(), 2);

    // the two rows are only difference in wavelength setting
    TS_ASSERT_EQUALS(wksp->Double(0, 1), 1.4);
    TS_ASSERT_EQUALS(wksp->Double(1, 1), 1.5);

    // check all of the contents of the rows
    for (size_t i = 0; i < 2; ++i) {
      TS_ASSERT_EQUALS(wksp->Double(i, 0), 60.); // frequency
      TS_ASSERT_EQUALS(wksp->Int(i, 2), 1);      // bank number
      TS_ASSERT_EQUALS(wksp->String(i, 3), "0");
      TS_ASSERT_EQUALS(wksp->String(i, 4), "0");
      TS_ASSERT_EQUALS(wksp->String(i, 5), "0");
      TS_ASSERT_EQUALS(wksp->String(i, 6), "0");
      TS_ASSERT_EQUALS(wksp->String(i, 7), "0");
      TS_ASSERT_EQUALS(wksp->String(i, 8), ".31,.25,.13,.13,.13,.42");
      TS_ASSERT_EQUALS(wksp->String(i, 9), "13.66,5.83,3.93,2.09,1.57,31.42");
      TS_ASSERT_EQUALS(wksp->Double(i, 10), 300.00);
      TS_ASSERT_EQUALS(wksp->Double(i, 11), 16666.67);
      TS_ASSERT_EQUALS(wksp->Double(i, 12), .1);
      TS_ASSERT_EQUALS(wksp->Double(i, 13), 2.9);
    }

    // test the other output properties
    checkNOMAD(alg);
  }

  void test_FocusAndChar2WithAzi() {
    const std::string CHAR_FILE("Test_characterizations_focus_azi.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 14);
    TS_ASSERT_EQUALS(wksp->rowCount(), 1);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0, 1), 1.4);
    TS_ASSERT_EQUALS(wksp->Int(0, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(0, 3), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 4), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 5), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 8), ".31,.25,.13,.13,.13,.42");
    TS_ASSERT_EQUALS(wksp->String(0, 9), "13.66,5.83,3.93,2.09,1.57,31.42");
    TS_ASSERT_EQUALS(wksp->Double(0, 10), 300.00);
    TS_ASSERT_EQUALS(wksp->Double(0, 11), 16666.67);
    TS_ASSERT_EQUALS(wksp->Double(0, 12), .1);
    TS_ASSERT_EQUALS(wksp->Double(0, 13), 2.9);

    // test the other output properties
    checkNOMAD(alg,
               false); // don't check azimuthal angles because it is done here
    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi[0], 0.);
    TS_ASSERT_EQUALS(azi[1], 15.);
    TS_ASSERT_EQUALS(azi[2], 25.);
    TS_ASSERT_EQUALS(azi[3], 0.);
    TS_ASSERT_EQUALS(azi[4], 0.);
    TS_ASSERT_EQUALS(azi[5], 65.);
  }

  void test_BrokenFocusAndChar() {
    const std::string CHAR_FILE("Test_characterizations_broken_focus.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 14);
    TS_ASSERT_EQUALS(wksp->rowCount(), 1);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0, 0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0, 1), 1.4);
    TS_ASSERT_EQUALS(wksp->Int(0, 2), 1);
    TS_ASSERT_EQUALS(wksp->String(0, 3), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 4), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 5), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 6), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 7), "0");
    TS_ASSERT_EQUALS(wksp->String(0, 8), ".31,.25,.13,.13,.13,.42");
    TS_ASSERT_EQUALS(wksp->String(0, 9), "13.66,5.83,3.93,2.09,1.57,31.42");
    TS_ASSERT_EQUALS(wksp->Double(0, 10), 300.00);
    TS_ASSERT_EQUALS(wksp->Double(0, 11), 16666.67);
    TS_ASSERT_EQUALS(wksp->Double(0, 12), .1);
    TS_ASSERT_EQUALS(wksp->Double(0, 13), 2.9);

    // there shouldn't be an instrument
    // test the other output properties
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"),
                     std::string("NOMAD_11_22_11.prm"));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, Mantid::EMPTY_DBL());

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), 0);

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), 0);

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), 0);

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), 0);
  }

  void test_Focus() {
    const std::string CHAR_FILE("Test_characterizations_focus.txt");
    ITableWorkspace_sptr wksp;

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILE);

    // test the table workspace
    TS_ASSERT_EQUALS(wksp->columnCount(), 14);
    TS_ASSERT_EQUALS(wksp->rowCount(), 0);

    // test the other output properties
    checkNOMAD(alg);
  }

  void test_Char() {
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

  void test_ExpIni() {
    // this file doesn't have enough information
    const std::string CHAR_FILE("Test_characterizations_focus_and_char2.txt");
    const std::string WKSP_NAME("test_ExpIni");

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("Filename", CHAR_FILE);
    alg.setPropertyValue("ExpIniFilename", "NOMAD_exp.ini");
    alg.setPropertyValue("OutputWorkspace", WKSP_NAME);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // test the table workspace
    ITableWorkspace_sptr wksp;
    wksp = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(WKSP_NAME));
    TS_ASSERT(wksp);

    TS_ASSERT_EQUALS(wksp->rowCount(), 2);
    for (size_t i = 0; i < 2; ++i) {
      TS_ASSERT_EQUALS(wksp->String(i, 3), "49258"); // vanadium
      TS_ASSERT_EQUALS(wksp->String(i, 4), "49086"); // vanadium_background
      TS_ASSERT_EQUALS(wksp->String(i, 5), "49257"); // container
      TS_ASSERT_EQUALS(wksp->String(i, 6), "0");     // empty_environment
      TS_ASSERT_EQUALS(wksp->String(i, 7), "0");     // empty_instrument
    }
  }

  void test_ExpIni_failing() {
    // this file doesn't have enough information
    const std::string CHAR_FILE("Test_characterizations_focus.txt");
    const std::string WKSP_NAME("test_ExpIni_failing");

    // initialize and run the algorithm
    PDLoadCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("Filename", CHAR_FILE);
    alg.setPropertyValue("ExpIniFilename", "NOMAD_exp.ini");
    alg.setPropertyValue("OutputWorkspace", WKSP_NAME);
    alg.setRethrows(true); // so the exception can be seen by testing
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_version2() {
    const std::string CHAR_FILES("Test_characterizations_char.txt,"
                                 "PG3_char_2016_02_15-PAC-single.txt");

    // initialize and run the algorithm
    ITableWorkspace_sptr wksp;
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILES);

    checkPG3WithContainers(wksp);
  }

  void test_version2_wrongOrder() {
    const std::string CHAR_FILES("PG3_char_2016_02_15-PAC-single.txt,"
                                 "Test_characterizations_char.txt");
    // initialize and run the algorithm
    ITableWorkspace_sptr wksp;
    PDLoadCharacterizations alg;
    runAlg(alg, wksp, CHAR_FILES);

    checkPG3WithContainers(wksp);
  }

  /* TODO this is for a later iteration
  void xtest_version2_extras() {
    const std::string CHAR_FILES("Test_characterizations_char.txt,"
                                 "PG3_char_2016_02_15-PAC-extras.txt");
  }
  */
};

#endif /* MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_ */
