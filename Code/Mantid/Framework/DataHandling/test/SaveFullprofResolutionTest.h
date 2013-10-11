#ifndef MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_
#define MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveFullprofResolution.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <Poco/File.h>

#include <fstream>
#include <iostream>

using namespace std;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using Mantid::DataHandling::SaveFullprofResolution;

class SaveFullprofResolutionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveFullprofResolutionTest *createSuite() { return new SaveFullprofResolutionTest(); }
  static void destroySuite( SaveFullprofResolutionTest *suite ) { delete suite; }


  void test_Init()
  {
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** Test save profile 10
    */
  void XXXtest_SaveFile()
  {
    // 1. Create input workspace
    throw runtime_error("Need to dig out the data");
    string filename("/home/wzz/Mantid/Code/debug/MyTestData/Bank1InstrumentTable.dat");
    map<std::string, double> parameters, newvalueparameters;
    map<string, vector<double> > parametermcs;
    importInstrumentTxtFile(filename, parameters, parametermcs);
    TableWorkspace_sptr itablews = createInstrumentParameterWorkspace(parameters, newvalueparameters, parametermcs);

    AnalysisDataService::Instance().addOrReplace("Bank1InstrumentParameterTable", itablews);

    // 2. Init the algorithm
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // 3. Set up
    alg.setProperty("InputWorkspace", "Bank1InstrumentParameterTable");
    alg.setProperty("OutputFilename", "bank1.irf");
    alg.setProperty("Bank", 1);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(1, 212);

  }

  //----------------------------------------------------------------------------------------------
  /** Test writing out a single bank in a multiple bank table workspace
    */
  void test_write1BankInMultiBankTableProf9()
  {
    // Generate test table workspace
    string parwsname("HRPD2BankParameterTableWS");
    create2BankProf9Table(parwsname);

    // Create and set up algorithm to test
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("InputWorkspace", parwsname);
    alg.setProperty("OutputFilename", "bank2.irf");
    alg.setProperty("Bank", 2);
    alg.setProperty("ProfileFunction", "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Locate file
    Poco::File irffile("bank2.irf");
    TS_ASSERT(irffile.exists());
    if (!irffile.exists())
    {
      Poco::File(parwsname).remove();
      return;
    }

    // Count number of lines
    int numlines = getFileLines("bank2.irf");
    TS_ASSERT_EQUALS(numlines, 18);

    // Clean
    Poco::File("bank2.irf").remove();

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Test writing out a single bank in a multiple bank table workspace
    */
  void test_appendBankInMultiBankTableProf9()
  {
    // Generate test table workspace
    string parwsname("HRPD2BankParameterTableWS");
    create2BankProf9Table(parwsname);

    // Write out the first bank
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("InputWorkspace", parwsname);
    alg.setProperty("OutputFilename", "bankall.irf");
    alg.setProperty("Bank", 1);
    alg.setProperty("ProfileFunction", "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Append the second bank
    Mantid::DataHandling::SaveFullprofResolution alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());

    alg2.setProperty("InputWorkspace", parwsname);
    alg2.setProperty("OutputFilename", "bankall.irf");
    alg2.setProperty("Bank", 2);
    alg2.setProperty("ProfileFunction", "Back-to-back exponential convoluted with pseudo-voigt (profile 9)");
    alg2.setProperty("Append", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg.isExecuted());

    // Locate file
    Poco::File irffile("bankall.irf");
    TS_ASSERT(irffile.exists());
    if (!irffile.exists())
    {
      Poco::File(parwsname).remove();
      return;
    }

    // Count number of lines
    int numlines = getFileLines("bankall.irf");
    TS_ASSERT_EQUALS(numlines, 34);

    // Clean
    irffile.remove();
  }


  //----------------------------------------------------------------------------------------------
  /** Find out number of lines in a text file
    */
  int getFileLines(std::string filename)
  {
    ifstream infile;
    infile.open(filename.c_str());

    int numlines = 0;
    if (infile.is_open())
    {
      string line;
      while(getline(infile, line))
      {
        if (line.size() > 0)
          ++ numlines;
      }
    }
    else
    {
      numlines = -1;
    }

    return numlines;
  }

  //----------------------------------------------------------------------------------------------
  /** Write out a TableWorkspace contain 2 banks' parameters
    * ISIS HRPD Data
    */
  void create2BankProf9Table(string workspacename)
  {
    TableWorkspace_sptr partablews = boost::make_shared<TableWorkspace>();
    partablews->addColumn("str", "Name");
    partablews->addColumn("double", "Value_1");
    partablews->addColumn("double", "Value_2");

    TableRow row0 = partablews->appendRow();
    row0 << "BANK" << 1. << 2.;
    TableRow row1 = partablews->appendRow();
    row1 << "Alph0" 	<< 	0.		<< 0.         ;
    TableRow row2 = partablews->appendRow();
    row2 << "Alph1" 	<< 	0.081722	<< 0.109024;
    TableRow row3 = partablews->appendRow();
    row3 << "Beta0" 	<< 	0.023271	<< 0.018108;
    TableRow row4 = partablews->appendRow();
    row4 << "Beta1" 	<< 	0.006292	<< 0.015182;
    TableRow row5 = partablews->appendRow();
    row5 << "CWL" 	<< 	-1.		<< -1.     ;
    TableRow row6 = partablews->appendRow();
    row6 << "Dtt1" 	<< 	48303.1		<< 34837.1 ;
    TableRow row7 = partablews->appendRow();
    row7 << "Dtt2" 	<< 	-4.093		<< -0.232  ;
    TableRow row8 = partablews->appendRow();
    row8 << "Gam0" 	<< 	6.611		<< 0.      ;
    TableRow row9 = partablews->appendRow();
    row9 << "Gam1" 	<< 	0.		<< 5.886   ;
    TableRow row10 = partablews->appendRow();
    row10 << "Gam2" 	<< 	0.		<< 0.      ;
    TableRow row11 = partablews->appendRow();
    row11 << "Sig0" 	<< 	0.		<< 0.      ;
    TableRow row12 = partablews->appendRow();
    row12 << "Sig1" 	<< 	10.6313		<< 61.5518 ;
    TableRow row13 = partablews->appendRow();
    row13 << "Sig2" 	<< 	0.		<< 12.1755 ;
    TableRow row14 = partablews->appendRow();
    row14 << "Zero" 	<< 	-4.734		<< 2.461   ;
    TableRow row15 = partablews->appendRow();
    row15 << "step" 	<< 	1.		<< 7.85    ;
    TableRow row16 = partablews->appendRow();
    row16 << "tof-max" << 	105100.		<< 111500. ;
    TableRow row17 = partablews->appendRow();
    row17 << "tof-min" << 	14364.		<< 12680.  ;
    TableRow row18 = partablews->appendRow();
    row18 << "twotheta" << 	168.33		<< 89.58   ;

    AnalysisDataService::Instance().addOrReplace(workspacename, partablews);

    return;
  }


  //----------------  Helpers To Create Input Workspaces --------------------------

  /** Create instrument geometry parameter/LeBail parameter workspaces
   */
  DataObjects::TableWorkspace_sptr createInstrumentParameterWorkspace(std::map<std::string, double> parameters,
                                                                      std::map<std::string, double> newvalueparameters,
                                                                      map<string, vector<double> > mcparameters)
  {
    UNUSED_ARG(mcparameters);

    // 1. Combine 2 inputs
    std::map<std::string, double>::iterator nvit;
    std::stringstream infoss;
    infoss << "Modifying parameters: " << std::endl;
    for (nvit = newvalueparameters.begin(); nvit != newvalueparameters.end(); ++nvit)
    {
      std::map<std::string, double>::iterator fdit;
      fdit = parameters.find(nvit->first);
      if (fdit != parameters.end())
      {
        fdit->second = nvit->second;
        infoss << "Name: " << std::setw(15) << fdit->first << ", Value: " << fdit->second << std::endl;
      }
    }
    std::cout << infoss.str();

    // 2. Crate table workspace
    DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr geomws = DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");
    tablews->addColumn("double", "Chi2");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");

    // 2. Add peak parameters' name and values
    map<string, double>::iterator mit;
    string fitortie("f");
    double minvalue = 0.0;
    double maxvalue = 0.0;
    double stepsize = 0.0;
    for (mit = parameters.begin(); mit != parameters.end(); ++mit)
    {
      string parname = mit->first;
      double parvalue = mit->second;

      API::TableRow newrow = geomws->appendRow();
      newrow << parname << parvalue << fitortie << 1.234 << minvalue << maxvalue << stepsize;
    }

    return geomws;
  }

  /** Import text file containing the instrument parameters
    * Format: name, value, min, max, step-size
    * Input:  a text based file
    * Output: a map for (parameter name, parameter value)
    */
  void importInstrumentTxtFile(std::string filename, std::map<std::string, double>& parameters,
                               std::map<string, vector<double> >& parametermcs)
  {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open())
    {
      std::cout << "File " << filename << " cannot be opened. " << std::endl;
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    }
    else
    {
      std::cout << "Importing instrument parameter file " << filename << std::endl;
    }

    // 2. Parse
    parameters.clear();
    parametermcs.clear();

    char line[256];
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        std::string parname;
        double parvalue, parmin, parmax, parstepsize;

        std::stringstream ss;
        ss.str(line);
        ss >> parname >> parvalue;
        parameters.insert(std::make_pair(parname, parvalue));

        try
        {
          ss >> parmin >> parmax >> parstepsize;
          vector<double> mcpars;
          mcpars.push_back(parmin);
          mcpars.push_back(parmax);
          mcpars.push_back(parstepsize);
          parametermcs.insert(make_pair(parname, mcpars));
        }
        catch (runtime_error err)
        {
          ;
        }
      }
    }

    ins.close();

    return;
  }


};


#endif /* MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_ */
