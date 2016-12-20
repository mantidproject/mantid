#ifndef MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_
#define MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveFullprofResolution.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"

#include <Poco/File.h>

#include <fstream>

using namespace std;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using Mantid::DataHandling::SaveFullprofResolution;

class SaveFullprofResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveFullprofResolutionTest *createSuite() {
    return new SaveFullprofResolutionTest();
  }
  static void destroySuite(SaveFullprofResolutionTest *suite) { delete suite; }

  void Ptest_Init() {
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** Test save profile 10
    */
  void test_write1BankProfl10() {
    // Create input workspace
    std::string prof10tablewsname("Bank1InstrumentParameterTable");
    createProfile10TableWS(prof10tablewsname);

    // Init the algorithm
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // Set up properties
    alg.setPropertyValue("InputWorkspace", "Bank1InstrumentParameterTable");
    alg.setProperty("OutputFilename", "bank1.irf");
    alg.setProperty("Bank", 1);

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check file
    std::string filename =
        alg.getProperty("OutputFilename"); // Get full pathname
    bool outputfileexist = Poco::File(filename).exists();
    TS_ASSERT(outputfileexist);

    if (!outputfileexist) {
      return;
    }

    // Check file lines
    int numlines = getFileLines(filename);
    TS_ASSERT_EQUALS(numlines, 22);

    // Clean
    Poco::File(filename).remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test writing out a single bank in a multiple bank table workspace
    */
  void test_write1BankInMultiBankTableProf9() {
    // Generate test table workspace
    string parwsname("HRPD2BankParameterTableWS");
    create2BankProf9Table(parwsname);

    // Create and set up algorithm to test
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("InputWorkspace", parwsname);
    alg.setProperty("OutputFilename", "bank2.irf");
    alg.setProperty("Bank", 2);
    alg.setProperty(
        "ProfileFunction",
        "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Locate file
    std::string filename =
        alg.getProperty("OutputFilename"); // Get full pathname
    Poco::File irffile(filename);
    TS_ASSERT(irffile.exists());
    if (!irffile.exists()) {
      Poco::File(parwsname).remove();
      return;
    }

    // Count number of lines
    int numlines = getFileLines(filename);
    TS_ASSERT_EQUALS(numlines, 18);

    // Clean
    Poco::File(filename).remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test writing out a single bank in a multiple bank table workspace
    */
  void test_appendBankInMultiBankTableProf9() {
    // Generate test table workspace
    string parwsname("HRPD2BankParameterTableWS");
    create2BankProf9Table(parwsname);

    // Write out the first bank
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("InputWorkspace", parwsname);
    alg.setProperty("OutputFilename", "bankall.irf");
    alg.setProperty("Bank", 1);
    alg.setProperty(
        "ProfileFunction",
        "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Append the second bank
    Mantid::DataHandling::SaveFullprofResolution alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());

    alg2.setProperty("InputWorkspace", parwsname);
    alg2.setProperty("OutputFilename", "bankall.irf");
    alg2.setProperty("Bank", 2);
    alg2.setProperty(
        "ProfileFunction",
        "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");
    alg2.setProperty("Append", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg.isExecuted());

    // Locate file
    std::string filename =
        alg.getProperty("OutputFilename"); // Get full pathname
    Poco::File irffile(filename);
    TS_ASSERT(irffile.exists());
    if (!irffile.exists()) {
      Poco::File(parwsname).remove();
      return;
    }

    // Count number of lines
    int numlines = getFileLines(filename);
    TS_ASSERT_EQUALS(numlines, 34);

    // Clean
    Poco::File(filename).remove();
  }

  //----------------------------------------------------------------------------------------------
  /** Find out number of lines in a text file
    */
  int getFileLines(std::string filename) {
    ifstream infile;
    infile.open(filename.c_str());

    int numlines = 0;
    if (infile.is_open()) {
      string line;
      while (getline(infile, line)) {
        if (line.size() > 0)
          ++numlines;
      }
    } else {
      numlines = -1;
    }

    return numlines;
  }

  //----------------------------------------------------------------------------------------------
  /** Write out a TableWorkspace contain 2 banks' parameters
    * ISIS HRPD Data
    */
  void create2BankProf9Table(string workspacename) {
    TableWorkspace_sptr partablews = boost::make_shared<TableWorkspace>();
    partablews->addColumn("str", "Name");
    partablews->addColumn("double", "Value_1");
    partablews->addColumn("double", "Value_2");

    TableRow row0 = partablews->appendRow();
    row0 << "BANK" << 1. << 2.;
    TableRow row1 = partablews->appendRow();
    row1 << "Alph0" << 0. << 0.;
    TableRow row2 = partablews->appendRow();
    row2 << "Alph1" << 0.081722 << 0.109024;
    TableRow row3 = partablews->appendRow();
    row3 << "Beta0" << 0.023271 << 0.018108;
    TableRow row4 = partablews->appendRow();
    row4 << "Beta1" << 0.006292 << 0.015182;
    TableRow row5 = partablews->appendRow();
    row5 << "CWL" << -1. << -1.;
    TableRow row6 = partablews->appendRow();
    row6 << "Dtt1" << 48303.1 << 34837.1;
    TableRow row7 = partablews->appendRow();
    row7 << "Dtt2" << -4.093 << -0.232;
    TableRow row8 = partablews->appendRow();
    row8 << "Gam0" << 6.611 << 0.;
    TableRow row9 = partablews->appendRow();
    row9 << "Gam1" << 0. << 5.886;
    TableRow row10 = partablews->appendRow();
    row10 << "Gam2" << 0. << 0.;
    TableRow row11 = partablews->appendRow();
    row11 << "Sig0" << 0. << 0.;
    TableRow row12 = partablews->appendRow();
    row12 << "Sig1" << 10.6313 << 61.5518;
    TableRow row13 = partablews->appendRow();
    row13 << "Sig2" << 0. << 12.1755;
    TableRow row14 = partablews->appendRow();
    row14 << "Zero" << -4.734 << 2.461;
    TableRow row15 = partablews->appendRow();
    row15 << "step" << 1. << 7.85;
    TableRow row16 = partablews->appendRow();
    row16 << "tof-max" << 105100. << 111500.;
    TableRow row17 = partablews->appendRow();
    row17 << "tof-min" << 14364. << 12680.;
    TableRow row18 = partablews->appendRow();
    row18 << "twotheta" << 168.33 << 89.58;

    AnalysisDataService::Instance().addOrReplace(workspacename, partablews);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create instrument geometry parameter/LeBail parameter workspaces of profil
   * 10
    * Source data is from POWGEN's bank 1 calibrated
   */
  void createProfile10TableWS(std::string wsname) {
    // Create a map of string/double for parameters of profile 10
    std::map<std::string, double> parammap{{"BANK", 1.},
                                           {"Alph0", 1.88187},
                                           {"Alph0t", 64.4102},
                                           {"Alph1", 0.},
                                           {"Alph1t", 0.},
                                           {"Beta0", 6.2511},
                                           {"Beta0t", 85.9189},
                                           {"Beta1", 0.},
                                           {"Beta1t", 0.},
                                           {"CWL", 0.533},
                                           {"Dtt1", 22584.5},
                                           {"Dtt1t", 22604.9},
                                           {"Dtt2", 0.},
                                           {"Dtt2t", 0.3},
                                           {"Gam0", 0.},
                                           {"Gam1", 5.744},
                                           {"Gam2", 0.},
                                           {"Sig0", 0.},
                                           {"Sig1", 3.16228},
                                           {"Sig2", 16.7331},
                                           {"Tcross", 0.356},
                                           {"Width", 1.0521},
                                           {"Zero", 0.},
                                           {"Zerot", 11.3175},
                                           {"step", 4.0002},
                                           {"tof-max", 51000.},
                                           {"tof-min", 5000.23},
                                           {"twotheta", 90.0}};

    // Crate table workspace
    DataObjects::TableWorkspace_sptr geomws =
        boost::make_shared<TableWorkspace>();

    geomws->addColumn("str", "Name");
    geomws->addColumn("double", "Value");
    geomws->addColumn("str", "FitOrTie");
    geomws->addColumn("double", "Chi2");
    geomws->addColumn("double", "Min");
    geomws->addColumn("double", "Max");
    geomws->addColumn("double", "StepSize");

    // Add peak parameters' name and values
    map<string, double>::iterator mit;
    string fitortie("f");
    double minvalue = 0.0;
    double maxvalue = 0.0;
    double stepsize = 0.0;
    for (mit = parammap.begin(); mit != parammap.end(); ++mit) {
      string parname = mit->first;
      double parvalue = mit->second;

      API::TableRow newrow = geomws->appendRow();
      newrow << parname << parvalue << fitortie << 1.234 << minvalue << maxvalue
             << stepsize;
    }

    AnalysisDataService::Instance().addOrReplace(wsname, geomws);

    return;
  }
};

#endif /* MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_ */
