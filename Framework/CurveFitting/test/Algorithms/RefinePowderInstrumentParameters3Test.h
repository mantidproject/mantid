#ifndef MANTID_CURVEFITTING_RefinePowderInstrumentParameters3TEST_H_
#define MANTID_CURVEFITTING_RefinePowderInstrumentParameters3TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters3.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <fstream>

using Mantid::CurveFitting::Algorithms::RefinePowderInstrumentParameters3;

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

class RefinePowderInstrumentParameters3Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RefinePowderInstrumentParameters3Test *createSuite() {
    return new RefinePowderInstrumentParameters3Test();
  }
  static void destroySuite(RefinePowderInstrumentParameters3Test *suite) {
    delete suite;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit with non Monte Carlo method.
    * The parameters to fit include Dtt1, Zero, and Width/Tcross
   */
  void test_FitNonMonteCarlo() {
    // 1. Create workspaces for testing
    int bankid = 1;

    // a) Generate workspaces
    Workspace2D_sptr posWS = generatePeakPositionWorkspace(bankid);
    // TableWorkspace_sptr profWS = generateInstrumentProfileTable(bankid);
    TableWorkspace_sptr profWS = generateInstrumentProfileTableBank1();

    // z) Set to data service
    AnalysisDataService::Instance().addOrReplace("Bank1PeakPositions", posWS);
    AnalysisDataService::Instance().addOrReplace("Bank1ProfileParameters",
                                                 profWS);

    // 2. Initialization
    RefinePowderInstrumentParameters3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // 3. Set parameters
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");

    alg.setProperty("InputInstrumentParameterWorkspace",
                    "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace",
                    "Bank1FittedProfileParameters");

    alg.setProperty("RefinementAlgorithm", "OneStepFit");
    alg.setProperty("StandardError", "UseInputValue");

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Check result
    // a) Profile parameter table
    TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<TableWorkspace>(
            AnalysisDataService::Instance().retrieve(
                "Bank1FittedProfileParameters"));
    TS_ASSERT(newgeomparamws);
    if (newgeomparamws) {
      std::map<std::string, double> fitparamvalues;
      parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
      map<string, double>::iterator mapiter;

      stringstream msgss;
      msgss << "[Unit Test]  Parameters: \n";
      for (mapiter = fitparamvalues.begin(); mapiter != fitparamvalues.end();
           ++mapiter) {
        msgss << "  |  " << mapiter->first << "\t = \t" << mapiter->second
              << "\t\n";
      }
      cout << msgss.str();
    }

    // b) Data
    Workspace2D_sptr outdataws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("Bank1FittedPositions"));
    TS_ASSERT(outdataws);
    if (outdataws) {
      /*
      stringstream outss;
      outss << "Difference: \n";
      for (size_t i = 0; i < outdataws->readX(0).size(); ++i)
        outss << outdataws->readX(0)[i] << "\t\t" << outdataws->readY(2)[i] <<
      '\n';
      cout << outss.str();
      */
    }

    // 4. Clean
    AnalysisDataService::Instance().remove("Bank1PeakPositions");
    AnalysisDataService::Instance().remove("Bank1FittedPositions");
    AnalysisDataService::Instance().remove("Bank1ProfileParameters");
    AnalysisDataService::Instance().remove("Bank1FittedProfileParameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit with Monte Carlo method.
    * The parameters to fit include Dtt1, Zero, and Width/Tcross
   */
  void test_FitMonteCarlo() {
    // 1. Create workspaces for testing
    int bankid = 1;

    // a) Generate workspaces
    Workspace2D_sptr posWS = generatePeakPositionWorkspace(bankid);
    // TableWorkspace_sptr profWS = generateInstrumentProfileTable(bankid);
    TableWorkspace_sptr profWS = generateInstrumentProfileTableBank1();

    // z) Set to data service
    AnalysisDataService::Instance().addOrReplace("Bank1PeakPositions", posWS);
    AnalysisDataService::Instance().addOrReplace("Bank1ProfileParameters",
                                                 profWS);

    // 2. Initialization
    RefinePowderInstrumentParameters3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // 3. Set parameters
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");

    alg.setProperty("InputInstrumentParameterWorkspace",
                    "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace",
                    "Bank1FittedProfileParameters");

    alg.setProperty("RefinementAlgorithm", "MonteCarlo");
    alg.setProperty("StandardError", "UseInputValue");

    alg.setProperty("AnnealingTemperature", 100.0);

    alg.setProperty("MonteCarloIterations", 100);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Check result
    // a) Profile parameter table
    TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<TableWorkspace>(
            AnalysisDataService::Instance().retrieve(
                "Bank1FittedProfileParameters"));
    TS_ASSERT(newgeomparamws);
    if (newgeomparamws) {
      std::map<std::string, double> fitparamvalues;
      parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
      map<string, double>::iterator mapiter;

      for (mapiter = fitparamvalues.begin(); mapiter != fitparamvalues.end();
           ++mapiter) {
        cout << "[P] " << mapiter->first << "\t = \t" << mapiter->second
             << "\t\n";
      }
    }

    // b) Data
    Workspace2D_sptr outdataws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("Bank1FittedPositions"));
    TS_ASSERT(outdataws);
    if (outdataws) {
      /*
      stringstream outss;
      outss << "Difference: \n";
      for (size_t i = 0; i < outdataws->readX(0).size(); ++i)
        outss << outdataws->readX(0)[i] << "\t\t" << outdataws->readY(2)[i] <<
      '\n';
      cout << outss.str();
      */
    }

    // 4. Clean
    AnalysisDataService::Instance().remove("Bank1PeakPositions");
    AnalysisDataService::Instance().remove("Bank1FittedPositions");
    AnalysisDataService::Instance().remove("Bank1ProfileParameters");
    AnalysisDataService::Instance().remove("Bank1FittedProfileParameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a table workspace for holding instrument parameters for POWGEN's
   * bank 1
    */
  TableWorkspace_sptr generateInstrumentProfileTableBank1() {
    DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr geomws =
        DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");

    API::TableRow newrow = geomws->appendRow();
    newrow << "Dtt1" << 22778.3 << "f" << 0.0 << 1.0E20 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Dtt1t" << 22747.4 << "t" << 0.0 << 1.0E20 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Dtt2" << 0.0 << "t" << 0.0 << 1.0E20 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Dtt2t" << 0.3 << "t" << -10000.0 << 100000.0 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Tcross" << 0.356 << "t" << 0.0 << 1000.0 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Width" << 1.1072 << "f" << 0.0 << 1000.0 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Zero" << 0.0 << "f" << -10000.0 << 10000.0 << 1.0;

    newrow = geomws->appendRow();
    newrow << "Zerot" << 90.7 << "t" << -10000.0 << 10000.0 << 1.0;

    return geomws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a table workspace for holding instrument parameters
    */
  TableWorkspace_sptr generateInstrumentProfileTable(int bankid) {
    // 1. Import
    vector<string> colnames;
    vector<vector<string>> strparams;

    if (bankid == 1) {
      string filename(
          "/home/wzz/Mantid/Code/debug/MyTestData/bank1profile.txt");
      importTableTextFile(filename, strparams, colnames, 6);
    } else {
      throw runtime_error("generateInstrumentProfile supports bank 1 only.");
    }

    // 2. Generate workspace
    DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr geomws =
        DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");

    // 3. Set up workspace
    int iname, ivalue, ifit, imin, imax, istep;
    iname = getIndex(colnames, "Name");
    ivalue = getIndex(colnames, "Value");
    ifit = getIndex(colnames, "FitOrTie");
    imin = getIndex(colnames, "Min");
    imax = getIndex(colnames, "Max");
    istep = getIndex(colnames, "StepSize");

    for (size_t ir = 0; ir < strparams.size(); ++ir) {
      // For each row
      API::TableRow newrow = geomws->appendRow();
      vector<string> &strvalues = strparams[ir];

      string parname = strvalues[iname];
      double parvalue = atof(strvalues[ivalue].c_str());
      string fitstr = strvalues[ifit];
      double minvalue = -DBL_MAX;
      if (imin >= 0)
        minvalue = atof(strvalues[imin].c_str());
      double maxvalue = DBL_MAX;
      if (imax >= 0)
        maxvalue = atof(strvalues[imax].c_str());
      double stepsize = 1.0;
      if (istep >= 0)
        stepsize = atof(strvalues[istep].c_str());

      newrow << parname << parvalue << fitstr << minvalue << maxvalue
             << stepsize;

      /*
      cout << "newrow = geomws->appendRow();\n";
      cout << "newrow << \"" <<  parname << "\" << " << parvalue << " << \"" <<
      fitstr << "\" << " << minvalue << " << "
           << maxvalue << " << " << stepsize << "; \n";
      */
    }

    return geomws;
  }

  //----------------------------------------------------------------------------------------------
  /** Get the index of a string in a vector
    */
  int getIndex(vector<string> vecstrs, string value) {
    int index = -1;
    for (int i = 0; i < static_cast<int>(vecstrs.size()); ++i) {
      if (value.compare(vecstrs[i]) == 0) {
        index = i;
        break;
      }
    }

    return index;
  }

  //----------------------------------------------------------------------------------------------
  /** Import a table from a text file (tabulized)
    */
  void importTableTextFile(string filename, vector<vector<string>> &strvectors,
                           vector<string> &columnnames, int numcols) {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open()) {
      std::cout << "File " << filename << " cannot be opened. \n";
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    } else {
      std::cout << "Importing tabulized text file " << filename << '\n';
    }

    if (numcols <= 0) {
      throw runtime_error(
          "If number of columns is set to zero or less, then it requires "
          "auto-column "
          "number determination.  Implement this functionality.");
    }

    // 2. Parse
    strvectors.clear();
    columnnames.clear();

    char line[256];
    while (ins.getline(line, 256)) {
      std::stringstream ss;
      ss.str(line);

      if (line[0] == '#') {
        // Column names
        string term;
        ss >> term;
        for (int i = 0; i < numcols; ++i) {
          ss >> term;
          columnnames.push_back(term);
        }
      } else {
        // Data
        vector<string> vecstr;
        for (int i = 0; i < numcols; ++i) {
          string term;
          ss >> term;
          vecstr.push_back(term);
        }
        strvectors.push_back(vecstr);
      }
    } // ENDWHILE

    ins.close();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse Table Workspace to a map of string, double pair
    */
  void parseParameterTableWorkspace(TableWorkspace_sptr paramws,
                                    map<string, double> &paramvalues) {
    for (size_t irow = 0; irow < paramws->rowCount(); ++irow) {
      Mantid::API::TableRow row = paramws->getRow(irow);
      std::string parname;
      double parvalue;
      row >> parname >> parvalue;

      paramvalues.emplace(parname, parvalue);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate workspace holding peak positions
    */
  Workspace2D_sptr generatePeakPositionWorkspace(int bankid) {

    if (bankid != 1) {
      throw runtime_error(
          "generatePeakPositionWorkspace supports bank 1 only.");
      // string
      // filename("/home/wzz/Mantid/Code/debug/MyTestData/bank1peakpositions.dat");
      // importDataFromColumnFile(filename, vecDsp, vecTof, vecError);
    }

    // 1. Generate vectors, bank 1's peak positions
    const size_t size = 16;
    std::array<double, size> vecDsp = {
        {0.907108, 0.929509, 0.953656, 0.979788, 1.008190, 1.039220, 1.110980,
         1.152910, 1.199990, 1.253350, 1.314520, 1.385630, 1.469680, 1.697040,
         1.859020, 2.078440}};
    std::array<double, size> vecTof = {
        {20487.600000, 20994.700000, 21537.400000, 22128.800000, 22769.200000,
         23469.400000, 25083.600000, 26048.100000, 27097.600000, 28272.200000,
         29684.700000, 31291.500000, 33394.000000, 38326.300000, 41989.800000,
         46921.700000}};
    std::array<double, size> vecError = {
        {0.350582, 0.597347, 0.644844, 0.879349, 0.417830, 0.481466, 0.527287,
         0.554732, 0.363456, 0.614706, 0.468477, 0.785721, 0.555938, 0.728131,
         0.390796, 0.997644}};

    // 2. Generate workspace
    Workspace2D_sptr dataws = boost::dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

    // 3. Put data
    MantidVec &vecX = dataws->dataX(0);
    MantidVec &vecY = dataws->dataY(0);
    MantidVec &vecE = dataws->dataE(0);
    for (size_t i = 0; i < size; ++i) {
      vecX[i] = vecDsp[i];
      vecY[i] = vecTof[i];
      vecE[i] = vecError[i];
    }

    return dataws;
  }

  //----------------------------------------------------------------------------------------------
  /** Import data from a column data file
   */
  void importDataFromColumnFile(string filename, vector<double> &vecX,
                                vector<double> &vecY, vector<double> &vecE) {
    std::ifstream ins;
    ins.open(filename.c_str());

    if (!ins.is_open()) {
      std::cout << "Data file " << filename << " cannot be opened. \n";
      throw std::invalid_argument("Unable to open data fiile. ");
    } else {
      std::cout << "Data file " << filename << " is opened for parsing. \n";
    }

    char line[256];
    while (ins.getline(line, 256)) {
      if (line[0] != '#') {
        double x, y, e;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y >> e;
        vecX.push_back(x);
        vecY.push_back(y);
        if (e < 0.00001) {
          if (y > 1.0)
            e = std::sqrt(y);
          else
            e = 1.0;
        }
        vecE.push_back(e);
      }
    }

    return;
  }
};

#endif /* MANTID_CURVEFITTING_RefinePowderInstrumentParameters3TEST_H_ */
