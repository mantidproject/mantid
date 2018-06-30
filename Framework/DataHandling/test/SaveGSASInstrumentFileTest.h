#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/SaveGSASInstrumentFile.h"
#include "MantidDataObjects/TableWorkspace.h"

#include <Poco/File.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::DataHandling::SaveGSASInstrumentFile;

class SaveGSASInstrumentFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveGSASInstrumentFileTest *createSuite() {
    return new SaveGSASInstrumentFileTest();
  }
  static void destroySuite(SaveGSASInstrumentFileTest *suite) { delete suite; }

  void test_SaveGSSInstrumentFile_1Bank() {
    Mantid::API::FrameworkManager::Instance();

    // Load a (local) table workspace
    loadProfileTable("PG3ProfileTable");
    TableWorkspace_sptr profiletablews =
        boost::dynamic_pointer_cast<TableWorkspace>(
            AnalysisDataService::Instance().retrieve("PG3ProfileTable"));
    TS_ASSERT(profiletablews);

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputWorkspace", "PG3ProfileTable");
    saver.setProperty("OutputFilename", "test.iparm");
    saver.setPropertyValue("BankIDs", "1");
    // saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "PG60_2011B");
    saver.setProperty("Sample", "LaB6");
    saver.setProperty("L1", 60.0);
    saver.setProperty("TwoTheta", 90.0);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file's existence and size
    std::string filename =
        saver.getProperty("OutputFilename"); // get full pathname
    TS_ASSERT(Poco::File(filename).exists());

    vector<size_t> veclineindextoread;
    veclineindextoread.push_back(5);
    veclineindextoread.push_back(20);
    veclineindextoread.push_back(304);

    vector<string> veclines;
    readLines(filename, veclineindextoread, veclines);

    // dmax changes from tabulated value (2.06) to converted-value (2.05263)
    // and thus cause the change of tabulated value in .prm file

    TS_ASSERT_EQUALS(veclines[0], "INS  1 ICONS 22748.017     0.000     0.000  "
                                  "             0.000    0     0.000   ");
    TS_ASSERT_EQUALS(veclines[1], "INS  1PAB3 2   0.11295   3.90798   0.70397  "
                                  " 0.24584                            ");
    ;
    TS_ASSERT_EQUALS(veclines[2], "INS  1PAB589   2.10936  51.75754   0.02659  "
                                  " 0.02265                            ");

    // Clean
    AnalysisDataService::Instance().remove("PG3ProfileTable");
    Poco::File(filename).remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  void test_SaveGSSInstrumentFile_LoadFile() {
    Mantid::API::FrameworkManager::Instance();

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputFileName", "2011B_HR60b2.irf");
    saver.setProperty("OutputFilename", "PG3_Bank2.iparm");
    saver.setProperty("Instrument", "powgen");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "PG60_2011B");
    saver.setProperty("Sample", "LaB6");
    saver.setProperty("L1", 60.0);
    saver.setProperty("TwoTheta", 90.0);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file's existence and size
    std::string filename =
        saver.getProperty("OutputFilename"); // get full pathname
    TS_ASSERT(Poco::File(filename).exists());

    // Clean
    AnalysisDataService::Instance().remove("PG3ProfileTable");
    Poco::File(filename).remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on import FP .irf file and import multiple banks
   */
  void Ptest_SaveGSSInstrumentFile_MultiBank() {
    // Generate a 3-bank .irf file
    string irffilename("pg3_60hz_3b.irf");
    string prmfilename1("test3bank.iparm");

    generate3BankIrfFile(irffilename);
    TS_ASSERT(Poco::File(irffilename).exists());

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputFileName", irffilename);
    saver.setProperty("OutputFilename", prmfilename1);
    saver.setPropertyValue("BankIDs", "1, 3-4");
    // saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "PG60_2011 3 Banks");
    saver.setProperty("Sample", "LaB6");
    saver.setProperty("L1", 60.0);
    saver.setProperty("TwoTheta", 90.0);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check existence of file
    std::string prmfilename = saver.getProperty("OutputFilename");
    TS_ASSERT(Poco::File(prmfilename).exists());

    string filename("test3bank.iparm");
    vector<size_t> veclineindextoread;
    veclineindextoread.push_back(52);
    veclineindextoread.push_back(499);
    veclineindextoread.push_back(906);

    vector<string> veclines;
    readLines(filename, veclineindextoread, veclines);

    TS_ASSERT_EQUALS(veclines[0],
                     "INS  1PAB334   0.99581  -0.99997124999.99996   0.15997");
    TS_ASSERT_EQUALS(veclines[1],
                     "INS  3PAB481   2.13019  -0.8982985923.49391   0.15924");
    TS_ASSERT_EQUALS(veclines[2],
                     "INS  4PAB589   3.91787 173.70816   0.01643   0.01323");

    // Clean
    Poco::File(prmfilename).remove();
    Poco::File(irffilename).remove();
  }

  //----------------------------------------------------------------------------------------------
  /** Load table workspace containing instrument parameters
   */
  void loadProfileTable(string wsname) {
    // The data befow is from Bank1 in pg60_2011B.irf

    auto tablews = boost::make_shared<TableWorkspace>();
    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value_1");

    vector<string> parnames;
    vector<double> parvalues;

    parnames.emplace_back("BANK");
    parvalues.push_back(1.);
    parnames.emplace_back("Alph0");
    parvalues.push_back(0.5);
    parnames.emplace_back("Alph0t");
    parvalues.push_back(65.14);
    parnames.emplace_back("Alph1");
    parvalues.push_back(8.15);
    parnames.emplace_back("Alph1t");
    parvalues.push_back(0);
    parnames.emplace_back("Beta0");
    parvalues.push_back(3.201);
    parnames.emplace_back("Beta0t");
    parvalues.push_back(78.412);
    parnames.emplace_back("Beta1");
    parvalues.push_back(7.674);
    parnames.emplace_back("Beta1t");
    parvalues.push_back(0);
    parnames.emplace_back("Dtt1");
    parvalues.push_back(22780.57);
    parnames.emplace_back("Dtt1t");
    parvalues.push_back(22790.129);
    parnames.emplace_back("Dtt2");
    parvalues.push_back(0);
    parnames.emplace_back("Dtt2t");
    parvalues.push_back(0.3);
    parnames.emplace_back("Gam0");
    parvalues.push_back(0);
    parnames.emplace_back("Gam1");
    parvalues.push_back(0);
    parnames.emplace_back("Gam2");
    parvalues.push_back(0);
    parnames.emplace_back("Sig0");
    parvalues.push_back(0);
    parnames.emplace_back("Sig1");
    parvalues.push_back(sqrt(10.0));
    parnames.emplace_back("Sig2");
    parvalues.push_back(sqrt(403.30));
    parnames.emplace_back("Tcross");
    parvalues.push_back(0.3560);
    parnames.emplace_back("Width");
    parvalues.push_back(1.2141);
    parnames.emplace_back("Zero");
    parvalues.push_back(0);
    parnames.emplace_back("Zerot");
    parvalues.push_back(-70.60);
    parnames.emplace_back("step");
    parvalues.push_back(5);
    parnames.emplace_back("tof-max");
    parvalues.push_back(46760);
    parnames.emplace_back("tof-min");
    parvalues.push_back(2278.06);
    parnames.emplace_back("twotheta");
    parvalues.push_back(90.807);
    parnames.emplace_back("CWL");
    parvalues.push_back(0.533);

    for (size_t i = 0; i < parnames.size(); ++i) {
      TableRow row = tablews->appendRow();
      row << parnames[i] << parvalues[i];
    }

    AnalysisDataService::Instance().addOrReplace(wsname, tablews);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a 3 bank .irf file
   */
  void generate3BankIrfFile(string filename) {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open()) {
      ofile << "  Instrumental resolution function for POWGEN/SNS  A Huq  "
               "2013-12-03  ireso: 6 \n";
      ofile << "! To be used with function NPROF=10 in FullProf  (Res=6)       "
               "                \n";
      ofile << "! ----------------------------------------------  Bank 1  CWL "
               "=   0.5330A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * "
               "pseudo-Voigt          \n";
      ofile << "NPROF 10                                                       "
               "                \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                   "
               "                \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                      "
               "                \n";
      ofile << "!          Zero    Dtt1                                        "
               "                \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                 "
               "                \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width        "
               "                \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957  "
               "                \n";
      ofile << "!     TOF-TWOTH of the bank                                    "
               "                \n";
      ofile << "TWOTH     90.00                                                "
               "                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                      "
               "                \n";
      ofile << "SIGMA     514.546      0.00044      0.355                      "
               "                \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                      "
               "                \n";
      ofile << "GAMMA       0.000       0.000       0.000                      "
               "                \n";
      ofile << "!         alph0       beta0       alph1       beta1            "
               "                \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000          "
               "                \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t           "
               "                \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000           "
               "                \n";
      ofile << "END                                                            "
               "                \n";
      ofile << "! ----------------------------------------------  Bank 3  CWL "
               "=   0.5339A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * "
               "pseudo-Voigt          \n";
      ofile << "NPROF 10                                                       "
               "                \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                   "
               "                \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                      "
               "                \n";
      ofile << "!          Zero    Dtt1                                        "
               "                \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                 "
               "                \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width        "
               "                \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957  "
               "                \n";
      ofile << "!     TOF-TWOTH of the bank                                    "
               "                \n";
      ofile << "TWOTH     90.00                                                "
               "                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                      "
               "                \n";
      ofile << "SIGMA     514.546      0.00044      0.355                      "
               "                \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                      "
               "                \n";
      ofile << "GAMMA       0.000       0.000       0.000                      "
               "                \n";
      ofile << "!         alph0       beta0       alph1       beta1            "
               "                \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000          "
               "                \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t           "
               "                \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000           "
               "                \n";
      ofile << "END                                                            "
               "                \n";
      ofile << "! ----------------------------------------------  Bank 4  CWL "
               "=   1.3330A\n";
      ofile << "!  Type of profile function: back-to-back exponentials * "
               "pseudo-Voigt    \n";
      ofile << "NPROF 10                                                       "
               "          \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                   "
               "          \n";
      ofile << "TOFRG   9800.0000      5.0000   86000.0000                     "
               "          \n";
      ofile << "!       Zero   Dtt1                                            "
               "          \n";
      ofile << "ZD2TOF     0.00  22586.10156                                   "
               "          \n";
      ofile << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width         "
               "          \n";
      ofile << "ZD2TOT -42.76068   22622.76953    0.30    0.3560    2.4135     "
               "          \n";
      ofile << "!     TOF-TWOTH of the bank                                    "
               "          \n";
      ofile << "TWOTH    90.000                                                "
               "          \n";
      ofile << "!       Sig-2     Sig-1     Sig-0                              "
               "          \n";
      ofile << "SIGMA  72.366    10.000     0.000                              "
               "          \n";
      ofile << "!       Gam-2     Gam-1     Gam-0                              "
               "          \n";
      ofile << "GAMMA     0.000     2.742      0.000                           "
               "          \n";
      ofile << "!          alph0       beta0       alph1       beta1           "
               "          \n";
      ofile << "ALFBE        1.500      3.012      5.502      9.639            "
               "          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t           "
               "          \n";
      ofile << "ALFBT       86.059     96.487     13.445      3.435            "
               "          \n";

      ofile.close();
    } else {
      throw runtime_error("Unable to open file to write.");
    }
  }

  //------------------------------------------------------------------------------------
  /** Read several specified lines from a file
   */
  void readLines(const std::string &filename,
                 const std::vector<size_t> &veclineindex,
                 std::vector<std::string> &veclines) {
    // Validate
    if (veclineindex.empty())
      throw std::runtime_error("Vector of line indexes cannot be empty.");

    // Sort line indexes
    std::vector<size_t> vecindex = veclineindex;
    std::sort(vecindex.begin(), vecindex.end());

    // Open file
    ifstream file(filename.c_str(), std::ifstream::in);
    if (!file) {
      std::stringstream errss;
      errss << "Couldn't open the file  " << filename << ".\n";
      throw runtime_error(errss.str());
    }

    // Read
    veclines.assign(veclineindex.size(), "");

    size_t itemindex = 0;
    size_t lineindextoread = veclineindex[0];
    bool islast = itemindex == veclineindex.size() - 1;
    bool lastIsRead = false;

    char linestring[256];
    size_t linenumber = 0;
    while (!file.eof() && !lastIsRead) {
      file.getline(linestring, 256);
      if (linenumber == lineindextoread) {
        // This is the line to read out to output
        veclines[itemindex] = linestring;
        // Update the information
        if (islast)
          lastIsRead = true;
        else {
          ++itemindex;
          islast = itemindex == veclineindex.size() - 1;
          lineindextoread = veclineindex[itemindex];
        }
      }
      ++linenumber;
    }

    // Check
    if (!lastIsRead)
      throw runtime_error("Not all lines are found!");

    return;
  }

  // Compare 2 files
  bool compare2Files(std::string filename1, std::string filename2) {
    ifstream file1(filename1.c_str(), std::ifstream::in);
    ifstream file2(filename2.c_str(), std::ifstream::in);

    // file1.open( filename1.c_str(), ios::binary ); //c_str() returns C-style
    // string pointer
    // file2.open( filename2.c_str(), ios::binary );

    if (!file1) {
      cout << "Couldn't open the file  " << filename1 << '\n';
      return false;
    }

    if (!file2) {
      cout << "Couldn't open the file " << filename2 << '\n';
      return false;
    }

    //---------- compare number of lines in both files ------------------//
    int c1, c2;
    c1 = 0;
    c2 = 0;
    string str;
    while (!file1.eof()) {
      getline(file1, str);
      c1++;
    }

    if (c1 != c2) {
      cout << "Different number of lines in files!"
           << "\n";
      cout << filename1 << " has " << c1 << " lines and " << filename2 << " has"
           << c2 << " lines"
           << "\n";
      return false;
    }

    while (!file2.eof()) {
      getline(file2, str);
      c2++;
    }

    // Reset file stream pointer
    file1.clear();            // add
    file1.seekg(0, ios::beg); // add

    file2.clear();
    file2.seekg(0, ios::beg);

    //---------- compare two files line by line ------------------//
    char string1[256], string2[256];
    int j = 0, error_count = 0;
    while (!file1.eof()) {
      file1.getline(string1, 256);
      file2.getline(string2, 256);
      j++;
      if (strcmp(string1, string2) != 0) {
        cout << j << "-the strings are not equal \n";
        cout << " file1   " << string1 << '\n';
        cout << " file2:  " << string2 << '\n';
        error_count++;
      }
    }
    if (error_count > 0) {
      cout << "files are diffrent\n";
      return false;
    } else {
      cout << "files are the same\n";
    }

    return true;
  }
};

#endif /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_ */
