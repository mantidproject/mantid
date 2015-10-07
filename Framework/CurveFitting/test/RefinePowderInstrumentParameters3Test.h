#ifndef MANTID_CURVEFITTING_RefinePowderInstrumentParameters3TEST_H_
#define MANTID_CURVEFITTING_RefinePowderInstrumentParameters3TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/RefinePowderInstrumentParameters3.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <fstream>

using Mantid::CurveFitting::RefinePowderInstrumentParameters3;

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
      msgss << "[Unit Test]  Parameters: " << endl;
      for (mapiter = fitparamvalues.begin(); mapiter != fitparamvalues.end();
           ++mapiter) {
        msgss << "  |  " << mapiter->first << "\t = \t" << mapiter->second
              << "\t" << endl;
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
      outss << "Difference: " << endl;
      for (size_t i = 0; i < outdataws->readX(0).size(); ++i)
        outss << outdataws->readX(0)[i] << "\t\t" << outdataws->readY(2)[i] <<
      endl;
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
        cout << "[P] " << mapiter->first << "\t = \t" << mapiter->second << "\t"
             << endl;
      }
    }

    // b) Data
    Workspace2D_sptr outdataws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("Bank1FittedPositions"));
    TS_ASSERT(outdataws);
    if (outdataws) {
      /*
      stringstream outss;
      outss << "Difference: " << endl;
      for (size_t i = 0; i < outdataws->readX(0).size(); ++i)
        outss << outdataws->readX(0)[i] << "\t\t" << outdataws->readY(2)[i] <<
      endl;
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
      cout << "newrow = geomws->appendRow();" << endl;
      cout << "newrow << \"" <<  parname << "\" << " << parvalue << " << \"" <<
      fitstr << "\" << " << minvalue << " << "
           << maxvalue << " << " << stepsize << "; " << endl;
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
      std::cout << "File " << filename << " cannot be opened. " << std::endl;
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    } else {
      std::cout << "Importing tabulized text file " << filename << std::endl;
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

      paramvalues.insert(std::make_pair(parname, parvalue));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate workspace holding peak positions
    */
  Workspace2D_sptr generatePeakPositionWorkspace(int bankid) {
    // 1. Generate vectors
    vector<double> vecDsp, vecTof, vecError;

    if (bankid == 1) {
      generateBank1PeakPositions(vecDsp, vecTof, vecError);
      // string
      // filename("/home/wzz/Mantid/Code/debug/MyTestData/bank1peakpositions.dat");
      // importDataFromColumnFile(filename, vecDsp, vecTof, vecError);
    } else {
      throw runtime_error(
          "generatePeakPositionWorkspace supports bank 1 only.");
    }

    // 2. Generate workspace
    Workspace2D_sptr dataws = boost::dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, vecDsp.size(),
                                            vecTof.size()));

    // 3. Put data
    MantidVec &vecX = dataws->dataX(0);
    MantidVec &vecY = dataws->dataY(0);
    MantidVec &vecE = dataws->dataE(0);
    for (size_t i = 0; i < vecDsp.size(); ++i) {
      vecX[i] = vecDsp[i];
      vecY[i] = vecTof[i];
      vecE[i] = vecError[i];
    }

    return dataws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate bank 1's peak positions
    */
  void generateBank1PeakPositions(vector<double> &vecX, vector<double> &vecY,
                                  vector<double> &vecE) {
    vecX.push_back(0.907108);
    vecY.push_back(20487.6);
    vecX.push_back(0.929509);
    vecY.push_back(20994.7);
    vecX.push_back(0.953656);
    vecY.push_back(21537.4);
    vecX.push_back(0.979788);
    vecY.push_back(22128.8);
    vecX.push_back(1.00819);
    vecY.push_back(22769.2);
    vecX.push_back(1.03922);
    vecY.push_back(23469.4);
    vecX.push_back(1.11098);
    vecY.push_back(25083.6);
    vecX.push_back(1.15291);
    vecY.push_back(26048.1);
    vecX.push_back(1.19999);
    vecY.push_back(27097.6);
    vecX.push_back(1.25335);
    vecY.push_back(28272.2);
    vecX.push_back(1.31452);
    vecY.push_back(29684.7);
    vecX.push_back(1.38563);
    vecY.push_back(31291.5);
    vecX.push_back(1.46968);
    vecY.push_back(33394.0);
    vecX.push_back(1.69704);
    vecY.push_back(38326.3);
    vecX.push_back(1.85902);
    vecY.push_back(41989.8);
    vecX.push_back(2.07844);
    vecY.push_back(46921.7);

    vecE.push_back(0.350582);
    vecE.push_back(0.597347);
    vecE.push_back(0.644844);
    vecE.push_back(0.879349);
    vecE.push_back(0.41783);
    vecE.push_back(0.481466);
    vecE.push_back(0.527287);
    vecE.push_back(0.554732);
    vecE.push_back(0.363456);
    vecE.push_back(0.614706);
    vecE.push_back(0.468477);
    vecE.push_back(0.785721);
    vecE.push_back(0.555938);
    vecE.push_back(0.728131);
    vecE.push_back(0.390796);
    vecE.push_back(0.997644);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Import data from a column data file
   */
  void importDataFromColumnFile(string filename, vector<double> &vecX,
                                vector<double> &vecY, vector<double> &vecE) {
    std::ifstream ins;
    ins.open(filename.c_str());

    if (!ins.is_open()) {
      std::cout << "Data file " << filename << " cannot be opened. "
                << std::endl;
      throw std::invalid_argument("Unable to open data fiile. ");
    } else {
      std::cout << "Data file " << filename << " is opened for parsing. "
                << std::endl;
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
