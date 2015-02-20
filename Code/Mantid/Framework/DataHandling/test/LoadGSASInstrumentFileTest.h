#ifndef MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILETEST_H_
#define MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadGSASInstrumentFile.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include <fstream>
#include <Poco/File.h>

using Mantid::DataHandling::LoadGSASInstrumentFile;

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

class LoadGSASInstrumentFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadGSASInstrumentFileTest *createSuite() { return new LoadGSASInstrumentFileTest(); }
  static void destroySuite( LoadGSASInstrumentFileTest *suite ) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test import from a 1-bank prm file
    */
  void test_1BankCase()
  {
    // 1. Generate file
    string filename("Test1Bank.prm");
    generate1BankPrmFile(filename);

    // 2. Load
    LoadGSASInstrumentFile alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setProperty("OutputTableWorkspace", "Test1BankTable");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
    AnalysisDataService::Instance().retrieve("Test1BankTable"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->columnCount(), 2);
    TS_ASSERT_EQUALS(outws->rowCount(), getExpectedNumberOfRows() );

    // 3. Verify name and value
    map<string, double> parammap;
    parseTableWorkspace(outws, parammap);

    TS_ASSERT_EQUALS(parammap.count("Beta0"),1);
    TS_ASSERT_EQUALS(parammap.count("Sig1"),1);
    TS_ASSERT_EQUALS(parammap.count("Gam1"),1);

    TS_ASSERT_DELTA(parammap["Beta0"], 31.793, 0.001);
    TS_ASSERT_DELTA(parammap["Sig1"], 176.802, 0.001);
    TS_ASSERT_DELTA(parammap["Gam1"], 0.007, 0.0001);

    // 4. Clean
    AnalysisDataService::Instance().remove("Test1BankTable");
    Poco::File("Test1Bank.prm").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test import from a 2-bank prm file
    */
  void test_2BankCase()
  {
    // 1. Generate file
    string filename("Test2Bank.prm");
    generate2BankPrmFile(filename);

    // 2. Load
    LoadGSASInstrumentFile alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setProperty("OutputTableWorkspace", "Test2BankTable");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
    AnalysisDataService::Instance().retrieve("Test2BankTable"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->columnCount(), 3);
    TS_ASSERT_EQUALS(outws->rowCount(), getExpectedNumberOfRows());

    // 3. Verify name and value
    map<string, double> parammap1;
    parseTableWorkspace(outws, parammap1);
    TS_ASSERT_EQUALS(parammap1.count("Alph1"),1);
    TS_ASSERT_EQUALS(parammap1.count("Sig2"),1);
    TS_ASSERT_EQUALS(parammap1.count("Gam1"),1);

    TS_ASSERT_DELTA(parammap1["Alph1"], 0.21, 0.0001);
    TS_ASSERT_DELTA(parammap1["Sig2"], 0.0, 0.0001);
    TS_ASSERT_DELTA(parammap1["Gam1"], 0.007, 0.00001);

    map<string, double> parammap2;
    parseTableWorkspace2(outws, parammap2);
    TS_ASSERT_EQUALS(parammap2.count("Alph1"),1);
    TS_ASSERT_EQUALS(parammap2.count("Sig2"),1);
    TS_ASSERT_EQUALS(parammap2.count("Gam1"),1);

    TS_ASSERT_DELTA(parammap2["Alph1"], 0.22, 0.0001);
    TS_ASSERT_DELTA(parammap2["Sig2"], -1.34662, 0.0001);
    TS_ASSERT_DELTA(parammap2["Gam1"], 3.61229, 0.00001);

    // 4. Clean
    AnalysisDataService::Instance().remove("Test2BankTable");
    Poco::File("Test2Bank.prm").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test import of ALFBE, GAMMA and SIGMA parameters
  *   and check they are given their expected names.
  */
  void test_ags_parameters()
  {
    // 1. Generate file
    string filename("TestAGS.prm");
    generate1BankPrmFile(filename);

    // 2. Load
    LoadGSASInstrumentFile alg;
    alg.initialize();

    alg.setProperty("Filename", filename);
    alg.setProperty("OutputTableWorkspace", "TestAGSTable");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
    AnalysisDataService::Instance().retrieve("TestAGSTable"));
    TS_ASSERT(outws);

    // 3. Verify names 
    map<string, double> parammap;
    parseTableWorkspace(outws, parammap);

    // 3a. ALFBE
    TS_ASSERT_EQUALS(parammap.count("Alph0"),1);
    TS_ASSERT_EQUALS(parammap.count("Beta0"),1);
    TS_ASSERT_EQUALS(parammap.count("Alph1"),1);
    TS_ASSERT_EQUALS(parammap.count("Beta1"),1);
    // 3b. GAMMA
    TS_ASSERT_EQUALS(parammap.count("Gam2"),1);
    TS_ASSERT_EQUALS(parammap.count("Gam1"),1);
    TS_ASSERT_EQUALS(parammap.count("Gam0"),1);
    // 3c. SIGMA
    TS_ASSERT_EQUALS(parammap.count("Sig2"),1);
    TS_ASSERT_EQUALS(parammap.count("Sig1"),1);
    TS_ASSERT_EQUALS(parammap.count("Sig0"),1);

    // 4. Clean
    AnalysisDataService::Instance().remove("TestAGSTable");
    Poco::File("TestAGS.prm").remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test invalid histrogram type
    */
  void test_Invalid_Histogram_Type()
  {
    // Generate file
    string filename("TestBadHistogramType.prm");
    generateBadHistogramTypePrmFile(filename);

    // Initialise and set Properties
    LoadGSASInstrumentFile alg;
    alg.initialize();
    alg.setProperty("Filename", filename);
    alg.setProperty("OutputTableWorkspace", "TestBadHistogramTable");

   // Execute and check that execution failed
    alg.execute();
    TS_ASSERT(!alg.isExecuted());

    // 3. Clean
    Poco::File("TestBadHistogramType.prm").remove();

    return;
  }

  void test_workspace()
  {
    // Generate file with two banks
    string filename("TestWorskpace.irf");
    generate2BankPrmFile(filename);

    // Create workspace group to put parameters into
    // This is a group of two workspaces
    createWorkspaceGroup(2,"loadGSASInstrumentFileWorkspace");

    // Set up algorithm to load into the workspace
    LoadGSASInstrumentFile alg;
    alg.initialize();
    alg.setProperty("Filename", filename);
    alg.setPropertyValue("Banks", "1,2");
    alg.setProperty("Workspace", wsName);

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check parameters in output workspace 
    // The output workspace is a workspace group with each 
    // member corresponding to each of the one banks in the prm file

    // First, check first workspace
    WorkspaceGroup_sptr gws;
    gws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    Mantid::Geometry::ParameterMap& paramMap = ws->instrumentParameters();
    boost::shared_ptr<const Mantid::Geometry::Instrument> instr = ws->getInstrument();

    // To check parameters in workspace
    Mantid::Geometry::FitParameter fitParam;
    Mantid::Geometry::Parameter_sptr param;

    // Check Alpha0 parameter    
    param = paramMap.get(&(*instr), "Alpha0", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 0.00);
    // Check Alpha1 parameter
    param = paramMap.get(&(*instr), "Alpha1", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 0.21);
    // Check Beta0 parameter
    param = paramMap.get(&(*instr), "Beta0", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 31.7927);
    // Check Beta1 parameter
    param = paramMap.get(&(*instr), "Kappa", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 51.4205);
    // Check SigmsSquared parameter
    // This is a formula, so values are not exact
    param = paramMap.get(&(*instr), "SigmaSquared", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_DELTA( fitParam.getValue(0.0), 0.01, 0.000001);
    TS_ASSERT_DELTA( fitParam.getValue(0.5), 7814.7468, 0.000001);
    // Check Gamma parameter
    // Although this is a formula, all coefficients should be zero
    // and so values should be exactly 0 as well
    param = paramMap.get(&(*instr), "Gamma", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( fitParam.getValue( 0.0 ), 0.0);
    TS_ASSERT_EQUALS( fitParam.getValue( 0.0 ), 0.0);


    // Now check second workspace
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(1));
    Mantid::Geometry::ParameterMap& paramMap2 = ws->instrumentParameters();
    instr = ws->getInstrument();

    // Check Alpha0 parameter
    param = paramMap2.get(&(*instr), "Alpha0", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 0.001);
    // Check Alpha1 parameter
    param = paramMap2.get(&(*instr), "Alpha1", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 0.22);
    // Check Beta0 parameter
    param = paramMap2.get(&(*instr), "Beta0", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 32.7927);
    // Check Beta1 parameter
    param = paramMap2.get(&(*instr), "Kappa", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( boost::lexical_cast<double>(fitParam.getFormula()), 52.4205);
    // Check SigmsSquared parameter
    // This is a formula, so values are not exact
    param = paramMap2.get(&(*instr), "SigmaSquared", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_DELTA( fitParam.getValue(0.0), 0.04, 0.000001);
    TS_ASSERT_DELTA( fitParam.getValue(0.5), 21840.741796, 0.000001);
    // Check Gamma parameter
    // Although this is a formula, all coefficients should be zero
    // and so values should be exactly 0 as well
    param = paramMap2.get(&(*instr), "Gamma", "fitting");
    fitParam = param->value<Mantid::Geometry::FitParameter>();
    TS_ASSERT_EQUALS( fitParam.getValue( 0.0 ), 0.0);
    TS_ASSERT_EQUALS( fitParam.getValue( 0.0 ), 0.0);

    // Clean
    Poco::File("TestWorskpace.irf").remove();
    AnalysisDataService::Instance().remove("loadGSASInstrumentFileWorkspace");
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
  /** Generate a 1 bank .prm file
    */
  void generate1BankPrmFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "COMM  Test file with one bank       \n";
      ofile << "INS   BANK  1                                                                  \n";
      ofile << "INS   HTYPE   PNTR      \n";
      ofile << "COMM5678901234567890                                                           \n";
      ofile << "INS  1 ICONS    746.96     -0.24      3.04                                     \n";
      ofile << "INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1       \n";
      ofile << "INS  1I ITYP    0    1.000     25.000       2                                  \n";
      ofile << "INS  1I HEAD   TIC 8983 on HRPD                                                \n";
      ofile << "INS  1PRCF      2   15   0.00100                                               \n";
      ofile << "COMM The next 15 parameters as in wiki page CreateIkedaCarpenterParametersGSAS \n";
      ofile << "INS  1PRCF 1   0.000000E+00   0.210000E+00   0.317927E+02   0.514205E+02       \n";
      ofile << "INS  1PRCF 2   0.100000E+00   0.176802E+03   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 3   0.007000E+00   0.008000E+00   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00                      \n";
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
  void generate2BankPrmFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "COMM  Test file with two banks       \n";
      ofile << "INS   BANK  2                                                                  \n";
      ofile << "INS   HTYPE   PNTR      \n";
      ofile << "COMM5678901234567890                                                           \n";
      ofile << "INS  1 ICONS    746.96     -0.24      3.04                                     \n";
      ofile << "INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1       \n";
      ofile << "INS  1I ITYP    0    1.000     25.000       2                                  \n";
      ofile << "INS  1PRCF      2   15   0.00100                                               \n";
      ofile << "INS  1PRCF 1   0.000000E+00   0.210000E+00   0.317927E+02   0.514205E+02       \n";
      ofile << "INS  1PRCF 2   0.100000E+00   0.176802E+03   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 3   0.007000E+00   0.000000E+00   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00                      \n";
      ofile << "INS  2 ICONS   1482.98      0.98     12.65                                     \n";
      ofile << "INS  2BNKPAR    1.7714     17.98      0.00    .00000     .3000    1    1       \n";
      ofile << "INS  2I ITYP    0    1.000     21.000       2                                  \n";
      ofile << "INS  2PRCF      2   15   0.00100                                               \n";
      ofile << "INS  2PRCF 1   0.001000E+00   0.220000E+00   0.327927E+02   0.524205E+02       \n";
      ofile << "INS  2PRCF 2   0.200000E+00   0.295572E+03  -0.134662E+01   0.000000E+00       \n";
      ofile << "INS  2PRCF 3   0.361229E+01   0.000000E+00   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  2PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00                      \n";
      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

    //----------------------------------------------------------------------------------------------
  /** Generate a 1 bank .prm file
    */
  void generateBadHistogramTypePrmFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "COMM  Test file with one bank       \n";
      ofile << "INS   BANK  1                                                                  \n";
      ofile << "INS   HTYPE   BLOG      \n";
      ofile << "COMM5678901234567890                                                           \n";
      ofile << "INS  1 ICONS    746.96     -0.24      3.04                                     \n";
      ofile << "INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1       \n";
      ofile << "INS  1I ITYP    0    1.000     25.000       2                                  \n";
      ofile << "INS  1PRCF      2   15   0.00100                                               \n";
      ofile << "COMM The next 15 parameters as in wiki page CreateIkedaCarpenterParametersGSAS \n";
      ofile << "INS  1PRCF 1   0.000000E+00   0.210000E+00   0.317927E+02   0.514205E+02       \n";
      ofile << "INS  1PRCF 2   0.100000E+00   0.176802E+03   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 3   0.007000E+00   0.008000E+00   0.000000E+00   0.000000E+00       \n";
      ofile << "INS  1PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00                      \n";
      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

    //----------------------------------------------------------------------------------------------
  /** Create a workspace group with specified number of workspaces.
    */
  void createWorkspaceGroup( size_t numberOfWorkspaces, std::string workspaceName)
  {
    // create a workspace with some sample data
    WorkspaceGroup_sptr gws(new API::WorkspaceGroup);

    for (size_t i=0; i < numberOfWorkspaces; ++i)
    {
      Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
      gws->addWorkspace( ws2D );
    }

    // put this workspace in the analysis data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(workspaceName, gws));

    // save workspace name
    wsName = workspaceName;
  }

  /* Return the number of rows the table must have
  */
  int getExpectedNumberOfRows()
  {
    return 12;  // Change this value if you add or remove any rows from the OutputTableWorkspace
  }

  private:
    std::string wsName;  // For workspace property

};


#endif /* MANTID_DATAHANDLING_LOADGSASINSTRUMENTFILETEST_H_ */
