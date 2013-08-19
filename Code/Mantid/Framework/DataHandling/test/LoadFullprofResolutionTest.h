#ifndef MANTID_DATAHANDLING_LOADFULLPROFRESOLUTIONTEST_H_
#define MANTID_DATAHANDLING_LOADFULLPROFRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <fstream>
#include <Poco/File.h>

using Mantid::DataHandling::LoadFullprofResolution;

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

class LoadFullprofResolutionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadFullprofResolutionTest *createSuite() { return new LoadFullprofResolutionTest(); }
  static void destroySuite( LoadFullprofResolutionTest *suite ) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test import from a 1-bank irf file
    */
  void test_1BankCase()
  {
    // 1. Generate file
    string filename("Test1Bank.irf");
    generate1BankIrfFile(filename);

    // 2. Load
    LoadFullprofResolution alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setPropertyValue("Banks", "1");
    alg.setProperty("OutputWorkspace", "TestBank1Table");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("TestBank1Table"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->columnCount(), 2);
    TS_ASSERT_EQUALS(outws->rowCount(), 27);

    // 3. Verify value
    map<string, double> parammap;
    parseTableWorkspace(outws, parammap);

    TS_ASSERT_DELTA(parammap["Zero"], -1.00, 0.0001);
    TS_ASSERT_DELTA(parammap["Sig2"], sqrt(514.546), 0.0001);
    TS_ASSERT_DELTA(parammap["Beta0t"], 85.918922, 0.00001);

    // 4. Clean
    AnalysisDataService::Instance().remove("TestBank1Table");
    Poco::File("Test1Bank.irf").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test import from a 1-bank irf file
    */
  void test_2BankCase()
  {
    // 1. Generate file
    string filename("Test2Bank.irf");
    generate2BankIrfFile(filename);

    // 2. Load
    LoadFullprofResolution alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setPropertyValue("Banks", "3");
    alg.setProperty("OutputWorkspace", "TestBank3Table");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("TestBank3Table"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->columnCount(), 2);
    TS_ASSERT_EQUALS(outws->rowCount(), 27);

    // 3. Verify value
    map<string, double> parammap;
    parseTableWorkspace(outws, parammap);

    TS_ASSERT_DELTA(parammap["Dtt1"], 22586.10156, 0.0001);
    TS_ASSERT_DELTA(parammap["Sig1"], sqrt(10.00), 0.0001);
    TS_ASSERT_DELTA(parammap["Alph0t"], 86.059, 0.00001);

    // 4. Clean
    AnalysisDataService::Instance().remove("TestBank3Table");
    Poco::File("Test2Bank.irf").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test import all banks from a 2-bank irf file
    */
  void test_LoadAllBankCase()
  {
    // Generate file
    string filename("Test2Bank.irf");
    generate2BankIrfFile(filename);

    // Init LoadFullprofResolution
    LoadFullprofResolution alg;
    alg.initialize();

    // Set up
    alg.setProperty("Filename", filename);
    alg.setProperty("OutputWorkspace", "TestBank4Table");

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("TestBank4Table"));
    TS_ASSERT(outws);

    // Check table workspace size
    TS_ASSERT_EQUALS(outws->columnCount(), 3);
    TS_ASSERT_EQUALS(outws->rowCount(), 27);

    // Verify value
    map<string, double> parammap1;
    parseTableWorkspace(outws, parammap1);
    TS_ASSERT_DELTA(parammap1["Dtt1"], 22580.59157, 0.0001);
    TS_ASSERT_DELTA(parammap1["Sig1"], sqrt(0.00044), 0.0001);
    TS_ASSERT_DELTA(parammap1["Alph0t"], 0.010156, 0.00001);

    map<string, double> parammap2;
    parseTableWorkspace2(outws, parammap2);
    TS_ASSERT_DELTA(parammap2["Dtt1"], 22586.10156, 0.0001);
    TS_ASSERT_DELTA(parammap2["Sig1"], sqrt(10.00), 0.0001);
    TS_ASSERT_DELTA(parammap2["Alph0t"], 86.059, 0.00001);



    // Clean
    AnalysisDataService::Instance().remove("TestBank4Table");
    Poco::File("Test2Bank.irf").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test import all banks from a 2-bank irf file
    */
  void test_Load3BankCase()
  {
    // Generate file
    string filename("Test3Bank.irf");
    generate3BankIrfFile(filename);

    // Init LoadFullprofResolution
    LoadFullprofResolution alg;
    alg.initialize();

    // Set up
    alg.setProperty("Filename", filename);
    alg.setProperty("OutputWorkspace", "TestBank5Table");
    alg.setPropertyValue("Banks", "2-4");

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("TestBank5Table"));
    TS_ASSERT(outws);

    // Check table workspace size
    TS_ASSERT_EQUALS(outws->columnCount(), 4);
    TS_ASSERT_EQUALS(outws->rowCount(), 27);

    // Verify value
    map<string, double> parammap1;
    parseTableWorkspace(outws, parammap1);
    TS_ASSERT_DELTA(parammap1["Dtt1"], 22580.59157, 0.0001);
    TS_ASSERT_DELTA(parammap1["Sig1"], sqrt(0.00044), 0.0001);
    TS_ASSERT_DELTA(parammap1["Alph0t"], 0.010156, 0.00001);

    // Clean
    AnalysisDataService::Instance().remove("TestBank5Table");
    Poco::File("Test3Bank.irf").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test Exception
    */
  void test_WrongInputBankCase()
  {
    // 1. Generate file
    string filename("Test2Bank.irf");
    generate2BankIrfFile(filename);

    // 2. Load
    LoadFullprofResolution alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setPropertyValue("Banks", "2");
    alg.setProperty("OutputWorkspace", "TestBank3Table");

    alg.execute();

    // 3. Check if failed
    TS_ASSERT(!alg.isExecuted());

    // 4. Clean
    Poco::File("Test2Bank.irf").remove();

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Parse a TableWorkspace to a map
    */
  void parseTableWorkspace(TableWorkspace_sptr tablews, map<string, double>& parammap)
  {
    parammap.clear();

    size_t numrows = tablews->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      TableRow row = tablews->getRow(i);
      double value;
      string name;
      row >> name >> value;
      parammap.insert(make_pair(name, value));
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Parse a TableWorkspace's 2nd bank to a map
    */
  void parseTableWorkspace2(TableWorkspace_sptr tablews, map<string, double>& parammap)
  {
    parammap.clear();

    size_t numrows = tablews->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      TableRow row = tablews->getRow(i);
      double value1, value2;
      string name;
      row >> name >> value1 >> value2;
      parammap.insert(make_pair(name, value2));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a 1 bank .irf file
    */
  void generate1BankIrfFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "  Instrumental resolution function for POWGEN/SNS  A Huq  2013-12-03  ireso: 6 \n";
      ofile << "! To be used with function NPROF=10 in FullProf  (Res=6)                       \n";
      ofile << "! ----------------------------------------------  Bank 1  CWL =   0.5330A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt          \n";
      ofile << "NPROF 10                                                                       \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                                   \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                                      \n";
      ofile << "!          Zero    Dtt1                                                        \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                                 \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width                        \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957                  \n";
      ofile << "!     TOF-TWOTH of the bank                                                    \n";
      ofile << "TWOTH     90.00                                                                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                                      \n";
      ofile << "SIGMA     514.546      0.00044      0.355                                      \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                                      \n";
      ofile << "GAMMA       0.000       0.000       0.000                                      \n";
      ofile << "!         alph0       beta0       alph1       beta1                            \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000                          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                           \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000                           \n";
      ofile << "END                                                                            \n";

      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a 2 bank .irf file
    */
  void generate2BankIrfFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "  Instrumental resolution function for POWGEN/SNS  A Huq  2013-12-03  ireso: 6 \n";
      ofile << "! To be used with function NPROF=10 in FullProf  (Res=6)                       \n";
      ofile << "! ----------------------------------------------  Bank 1  CWL =   0.5330A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt          \n";
      ofile << "NPROF 10                                                                       \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                                   \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                                      \n";
      ofile << "!          Zero    Dtt1                                                        \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                                 \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width                        \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957                  \n";
      ofile << "!     TOF-TWOTH of the bank                                                    \n";
      ofile << "TWOTH     90.00                                                                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                                      \n";
      ofile << "SIGMA     514.546      0.00044      0.355                                      \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                                      \n";
      ofile << "GAMMA       0.000       0.000       0.000                                      \n";
      ofile << "!         alph0       beta0       alph1       beta1                            \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000                          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                           \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000                           \n";
      ofile << "END                                                                            \n";
      ofile << "! ----------------------------------------------  Bank 3  CWL =   1.3330A\n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt    \n";
      ofile << "NPROF 10                                                                 \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                             \n";
      ofile << "TOFRG   9800.0000      5.0000   86000.0000                               \n";
      ofile << "!       Zero   Dtt1                                                      \n";
      ofile << "ZD2TOF     0.00  22586.10156                                             \n";
      ofile << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width                   \n";
      ofile << "ZD2TOT -42.76068   22622.76953    0.30    0.3560    2.4135               \n";
      ofile << "!     TOF-TWOTH of the bank                                              \n";
      ofile << "TWOTH    90.000                                                          \n";
      ofile << "!       Sig-2     Sig-1     Sig-0                                        \n";
      ofile << "SIGMA  72.366    10.000     0.000                                        \n";
      ofile << "!       Gam-2     Gam-1     Gam-0                                        \n";
      ofile << "GAMMA     0.000     2.742      0.000                                     \n";
      ofile << "!          alph0       beta0       alph1       beta1                     \n";
      ofile << "ALFBE        1.500      3.012      5.502      9.639                      \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                     \n";
      ofile << "ALFBT       86.059     96.487     13.445      3.435                      \n";

      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

  /** Generate a 3 bank .irf file
    */
  void generate3BankIrfFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "  Instrumental resolution function for POWGEN/SNS  A Huq  2013-12-03  ireso: 6 \n";
      ofile << "! To be used with function NPROF=10 in FullProf  (Res=6)                       \n";
      ofile << "! ----------------------------------------------  Bank 2  CWL =   0.5330A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt          \n";
      ofile << "NPROF 10                                                                       \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                                   \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                                      \n";
      ofile << "!          Zero    Dtt1                                                        \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                                 \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width                        \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957                  \n";
      ofile << "!     TOF-TWOTH of the bank                                                    \n";
      ofile << "TWOTH     90.00                                                                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                                      \n";
      ofile << "SIGMA     514.546      0.00044      0.355                                      \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                                      \n";
      ofile << "GAMMA       0.000       0.000       0.000                                      \n";
      ofile << "!         alph0       beta0       alph1       beta1                            \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000                          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                           \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000                           \n";
      ofile << "END                                                                            \n";
      ofile << "! ----------------------------------------------  Bank 3  CWL =   0.5339A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt          \n";
      ofile << "NPROF 10                                                                       \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                                   \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                                      \n";
      ofile << "!          Zero    Dtt1                                                        \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                                 \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width                        \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957                  \n";
      ofile << "!     TOF-TWOTH of the bank                                                    \n";
      ofile << "TWOTH     90.00                                                                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                                      \n";
      ofile << "SIGMA     514.546      0.00044      0.355                                      \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                                      \n";
      ofile << "GAMMA       0.000       0.000       0.000                                      \n";
      ofile << "!         alph0       beta0       alph1       beta1                            \n";
      ofile << "ALFBE    0.000008    6.251096    0.000000    0.000000                          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                           \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000                           \n";
      ofile << "END                                                                            \n";
      ofile << "! ----------------------------------------------  Bank 4  CWL =   1.3330A\n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt    \n";
      ofile << "NPROF 10                                                                 \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                             \n";
      ofile << "TOFRG   9800.0000      5.0000   86000.0000                               \n";
      ofile << "!       Zero   Dtt1                                                      \n";
      ofile << "ZD2TOF     0.00  22586.10156                                             \n";
      ofile << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width                   \n";
      ofile << "ZD2TOT -42.76068   22622.76953    0.30    0.3560    2.4135               \n";
      ofile << "!     TOF-TWOTH of the bank                                              \n";
      ofile << "TWOTH    90.000                                                          \n";
      ofile << "!       Sig-2     Sig-1     Sig-0                                        \n";
      ofile << "SIGMA  72.366    10.000     0.000                                        \n";
      ofile << "!       Gam-2     Gam-1     Gam-0                                        \n";
      ofile << "GAMMA     0.000     2.742      0.000                                     \n";
      ofile << "!          alph0       beta0       alph1       beta1                     \n";
      ofile << "ALFBE        1.500      3.012      5.502      9.639                      \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                     \n";
      ofile << "ALFBT       86.059     96.487     13.445      3.435                      \n";

      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

};


#endif /* MANTID_DATAHANDLING_LOADFULLPROFRESOLUTIONTEST_H_ */
