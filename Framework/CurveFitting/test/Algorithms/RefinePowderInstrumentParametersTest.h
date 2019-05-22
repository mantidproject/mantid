// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERSTEST_H_
#define MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <fstream>
#include <iomanip>

using Mantid::CurveFitting::Algorithms::RefinePowderInstrumentParameters;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

class RefinePowderInstrumentParametersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RefinePowderInstrumentParametersTest *createSuite() {
    return new RefinePowderInstrumentParametersTest();
  }
  static void destroySuite(RefinePowderInstrumentParametersTest *suite) {
    delete suite;
  }

  /** Test algorithm initialization
   */
  void test_init() {
    RefinePowderInstrumentParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    return;
  }

  /** Fit with one shifted parmeter 'Zero' of old bank 7 data
   */
  void Passed_test_FitZero() {
    // 1. Generate testing workspace
    std::map<std::string, double> newparamvalues{{"Tcross", 0.5}};

    // This is the output from FitPowderDiffPeaks()
    std::string peakfilename("/home/wzz/Mantid/Code/debug/MyTestData/"
                             "Bank7FittedPeaksParameters.txt");
    std::vector<std::vector<int>> hkls;
    std::vector<std::vector<double>> peakparameters;
    importPeakParametersFile(peakfilename, hkls, peakparameters);
    DataObjects::TableWorkspace_sptr peakparamws =
        createReflectionWorkspace(hkls, peakparameters);

    std::string insfilename(
        "/home/wzz/Mantid/Code/debug/MyTestData/Bank7InstrumentParameters.txt");
    std::map<std::string, double> instrparameters;
    map<string, vector<double>> mcparameters;
    importInstrumentTxtFile(insfilename, instrparameters, mcparameters);
    DataObjects::TableWorkspace_sptr geomparamws =
        createInstrumentParameterWorkspace(instrparameters, newparamvalues,
                                           mcparameters);

    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters",
                                                 geomparamws);

    // 2. [No] Fit
    RefinePowderInstrumentParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputWorkspace", "FittedCurve");
    alg.setProperty("OutputInstrumentParameterWorkspace",
                    "InstrumentParameterTable");
    alg.setProperty("MinNumberFittedPeaks", 3);
    alg.setProperty("ParametersToFit", "");
    alg.setProperty("RefinementAlgorithm", "DirectFit");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Fit
    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputWorkspace", "FittedCurve");
    alg.setProperty("OutputInstrumentParameterWorkspace",
                    "InstrumentParameterTable");
    alg.setProperty("MinNumberFittedPeaks", 3);
    alg.setProperty("ParametersToFit", "Tcross");
    alg.setProperty("RefinementAlgorithm", "DirectFit");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check result
    DataObjects::TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
            AnalysisDataService::Instance().retrieve(
                "InstrumentParameterTable"));

    std::map<std::string, double> fitparamvalues;
    parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
    double zero = fitparamvalues["Zero"];

    TS_ASSERT_DELTA(zero, 0.0, 1.0);

    // TS_ASSERT_EQUALS(1, 231);

    // 4. Clean
    AnalysisDataService::Instance().remove("DataWorkspace");
    AnalysisDataService::Instance().remove("FittedCurve");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("InstrumentParameters");
    AnalysisDataService::Instance().remove("FittedData");
    AnalysisDataService::Instance().remove("PeaksParameterTable");

    return;
  }

  /** Test fit by Monte Carlo random walk
   * Using the data from calibration of PG3 in August 2012 for bank 1
   */
  void Passed_test_MonteCarloRandomWalk() {
    // 0. Init
    map<string, double> newparamvalues;

    // 1. Generate testing workspace
    //    This is the output from FitPowderDiffPeaks()
    std::string peakfilename("/home/wzz/Mantid/Code/debug/MyTestData/"
                             "Bank1FittedPeaksParameters.txt");
    std::vector<std::vector<int>> hkls;
    std::vector<std::vector<double>> peakparameters;
    importPeakParametersFile(peakfilename, hkls, peakparameters);
    DataObjects::TableWorkspace_sptr peakparamws =
        createReflectionWorkspace(hkls, peakparameters);

    std::string insfilename(
        "/home/wzz/Mantid/Code/debug/MyTestData/Bank1InstrumentParameters.txt");
    std::map<std::string, double> instrparameters;
    map<string, vector<double>> mcparameters;
    importInstrumentTxtFile(insfilename, instrparameters, mcparameters);
    DataObjects::TableWorkspace_sptr geomparamws =
        createInstrumentParameterWorkspace(instrparameters, newparamvalues,
                                           mcparameters);

    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters",
                                                 geomparamws);

    // 2. Set up algorithm parameters
    RefinePowderInstrumentParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputWorkspace", "FittedPeakPositions");
    alg.setProperty("OutputInstrumentParameterWorkspace", "FittedParameters");
    alg.setProperty("OutputBestResultsWorkspace", "BestMCResults");
    alg.setProperty("RefinementAlgorithm", "MonteCarlo");
    alg.setProperty("RandomWalkSteps", 2000);
    alg.setProperty("MinSigma", 1.0);
    alg.setProperty("StandardError", "ConstantValue");
    alg.setProperty("ParametersToFit", "Dtt1, Dtt1t, Dtt2t, Zerot, Width");
    alg.setProperty("NumberBestFitRecorded", 10);

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check result
    DataObjects::TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
            AnalysisDataService::Instance().retrieve("FittedParameters"));

    DataObjects::Workspace2D_sptr dataws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            AnalysisDataService::Instance().retrieve("FittedPeakPositions"));
    TS_ASSERT(dataws);
    TS_ASSERT_EQUALS(dataws->getNumberHistograms(), 21);
    /*
    cout << "Number of peak positions = " << dataws->readX(0).size() << '\n';
    for (size_t i = 0; i < dataws->readX(0).size(); ++i)
      cout << i << "\t\t" << dataws->readX(0)[i] << '\n';
      */

    DataObjects::TableWorkspace_sptr mcresultws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
            AnalysisDataService::Instance().retrieve("BestMCResults"));
    TS_ASSERT_EQUALS(mcresultws->rowCount(), 10);

    std::map<std::string, double> fitparamvalues;
    parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
    double zero = fitparamvalues["Zero"];

    TS_ASSERT_DELTA(zero, 0.0, 1.0);

    TS_ASSERT_EQUALS(123, 345);

    return;
  }

  /** Test fit by Simplex
   * Using the data from calibration of PG3 in August 2012 for bank 1
   */
  void Passed_test_FitSimplex() {
    // 0. Init
    map<string, double> newparamvalues;

    // 1. Generate testing workspace
    //    This is the output from FitPowderDiffPeaks()
    std::string peakfilename("/home/wzz/Mantid/Code/debug/MyTestData/"
                             "Bank1FittedPeaksParameters.txt");
    std::vector<std::vector<int>> hkls;
    std::vector<std::vector<double>> peakparameters;
    importPeakParametersFile(peakfilename, hkls, peakparameters);
    DataObjects::TableWorkspace_sptr peakparamws =
        createReflectionWorkspace(hkls, peakparameters);

    std::string insfilename(
        "/home/wzz/Mantid/Code/debug/MyTestData/Bank1InstrumentParameters.txt");
    std::map<std::string, double> instrparameters;
    map<string, vector<double>> mcparameters;
    importInstrumentTxtFile(insfilename, instrparameters, mcparameters);
    DataObjects::TableWorkspace_sptr geomparamws =
        createInstrumentParameterWorkspace(instrparameters, newparamvalues,
                                           mcparameters);

    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters",
                                                 geomparamws);

    // 2. Set up algorithm parameters
    RefinePowderInstrumentParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputWorkspace", "FittedPeakPositions");
    alg.setProperty("OutputInstrumentParameterWorkspace", "FittedParameters");
    alg.setProperty("RefinementAlgorithm", "DirectFit");
    alg.setProperty("MinSigma", 1.0);
    alg.setProperty("StandardError", "ConstantValue");
    alg.setProperty("ParametersToFit", "Dtt1, Dtt1t, Dtt2t, Zerot, Width");
    alg.setProperty("NumberBestFitRecorded", 10);

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check result
    DataObjects::TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
            AnalysisDataService::Instance().retrieve("FittedParameters"));

    DataObjects::Workspace2D_sptr dataws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            AnalysisDataService::Instance().retrieve("FittedPeakPositions"));
    TS_ASSERT(dataws);
    TS_ASSERT_EQUALS(dataws->getNumberHistograms(), 3);

    std::map<std::string, double> fitparamvalues;
    parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
    double zero = fitparamvalues["Zero"];

    TS_ASSERT_DELTA(zero, 0.0, 1.0);

    TS_ASSERT_EQUALS(123, 345);

    return;
  }

  // ==========================  Methods To Create Input Workspaces
  // ======================== //

  /** Create reflection table workspaces
   */
  DataObjects::TableWorkspace_sptr
  createReflectionWorkspace(std::vector<std::vector<int>> hkls,
                            std::vector<std::vector<double>> peakparams) {
    // 1. Crate table workspace
    DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr hklws =
        DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("int", "H");
    tablews->addColumn("int", "K");
    tablews->addColumn("int", "L");
    tablews->addColumn("double", "d_h");
    tablews->addColumn("double", "TOF_h");
    tablews->addColumn("double", "PeakHeight");
    tablews->addColumn("double", "Alpha");
    tablews->addColumn("double", "Beta");
    tablews->addColumn("double", "Sigma2");
    tablews->addColumn("double", "Chi2");

    // 2. Add reflections and heights
    for (size_t ipk = 0; ipk < hkls.size(); ++ipk) {
      API::TableRow hkl = hklws->appendRow();
      for (size_t i = 0; i < 3; ++i) {
        hkl << hkls[ipk][i];
        cout << hkls[ipk][i] << ", ";
      }
      for (double ipm : peakparams[ipk]) {
        hkl << ipm;
        cout << ipm;
      }
      cout << '\n';
    }

    std::cout << "Created Table Workspace with " << hkls.size()
              << " entries of peaks.\n";

    return hklws;
  }

  /** Import text file containing reflection (HKL) and peak parameters
   * Input:  a text based file
   * Output: a vector for (H, K, L) and a vector for (Height, TOF_H, ALPHA,
   * BETA, ...
   */
  void
  importPeakParametersFile(std::string filename,
                           std::vector<std::vector<int>> &hkls,
                           std::vector<std::vector<double>> &peakparameters) {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open()) {
      std::cout << "File " << filename << " cannot be opened. \n";
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    } else {
      std::cout << "Parsing peak parameters file " << filename << '\n';
    }

    // 2. Parse
    hkls.clear();
    peakparameters.clear();

    char line[256];
    while (ins.getline(line, 256)) {
      if (line[0] != '#') {
        int h, k, l;
        std::vector<int> hkl;
        std::stringstream ss;
        ss.str(line);
        ss >> h >> k >> l;
        hkl.push_back(h);
        hkl.push_back(k);
        hkl.push_back(l);
        hkls.push_back(hkl);

        double d_h, tof_h, height, alpha, beta, sigma2, chi2;
        std::vector<double> params;
        ss >> d_h >> tof_h >> height >> alpha >> beta >> sigma2 >> chi2;
        params.push_back(d_h);
        params.push_back(tof_h);
        params.push_back(height);
        params.push_back(alpha);
        params.push_back(beta);
        params.push_back(sigma2);
        params.push_back(chi2);
        peakparameters.push_back(params);
      }
    }

    // 3. Close up
    ins.close();

    return;
  }

  /** Create instrument geometry parameter/LeBail parameter workspaces
   */
  DataObjects::TableWorkspace_sptr createInstrumentParameterWorkspace(
      std::map<std::string, double> parameters,
      std::map<std::string, double> newvalueparameters,
      map<string, vector<double>> mcparameters) {
    // 1. Combine 2 inputs
    std::map<std::string, double>::iterator nvit;
    std::stringstream infoss;
    infoss << "Importing instrument related parameters: \n";
    for (nvit = newvalueparameters.begin(); nvit != newvalueparameters.end();
         ++nvit) {
      std::map<std::string, double>::iterator fdit;
      fdit = parameters.find(nvit->first);
      if (fdit != parameters.end()) {
        fdit->second = nvit->second;
        infoss << "Name: " << std::setw(15) << fdit->first
               << ", Value: " << fdit->second << '\n';
      }
    }
    std::cout << infoss.str();

    // 2. Crate table workspace
    DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr geomws =
        DataObjects::TableWorkspace_sptr(tablews);

    std::vector<std::string> paramnames{"Zero",  "Zerot",          "Dtt1",
                                        "Dtt1t", "Dtt2t",          "Tcross",
                                        "Width", "LatticeConstant"};

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");

    // 2. Add peak parameters' name and values
    map<string, vector<double>>::iterator finditer;
    for (const auto &paramname : paramnames) {
      API::TableRow newrow = geomws->appendRow();
      std::string parname = paramname;
      double parvalue = parameters[paramname];
      newrow << parname << parvalue;
      double parmin = -DBL_MAX;
      double parmax = DBL_MAX;
      double stepsize = 1.0;
      finditer = mcparameters.find(parname);
      if (finditer != mcparameters.end()) {
        parmin = finditer->second[0];
        parmax = finditer->second[1];
        stepsize = finditer->second[2];
      }
      newrow << parmin << parmax << stepsize;
    }

    return geomws;
  }

  /** Import text file containing the instrument parameters
   * Format: name, value, min, max, step-size
   * Input:  a text based file
   * Output: a map for (parameter name, parameter value)
   */
  void importInstrumentTxtFile(std::string filename,
                               std::map<std::string, double> &parameters,
                               std::map<string, vector<double>> &parametermcs) {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open()) {
      std::cout << "File " << filename << " cannot be opened. \n";
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    } else {
      std::cout << "Importing instrument parameter file " << filename << '\n';
    }

    // 2. Parse
    parameters.clear();
    parametermcs.clear();

    char line[256];
    while (ins.getline(line, 256)) {
      if (line[0] != '#') {
        std::string parname;
        double parvalue, parmin, parmax, parstepsize;

        std::stringstream ss;
        ss.str(line);
        ss >> parname >> parvalue;
        parameters.emplace(parname, parvalue);

        try {
          ss >> parmin >> parmax >> parstepsize;
          vector<double> mcpars;
          mcpars.push_back(parmin);
          mcpars.push_back(parmax);
          mcpars.push_back(parstepsize);
          parametermcs.emplace(parname, mcpars);
        } catch (const std::runtime_error &err) {
          ;
        }
      }
    }

    ins.close();

    return;
  }

  /// =================  Check Output ================ ///
  void
  parseParameterTableWorkspace(Mantid::DataObjects::TableWorkspace_sptr paramws,
                               std::map<std::string, double> &paramvalues) {

    for (size_t irow = 0; irow < paramws->rowCount(); ++irow) {
      Mantid::API::TableRow row = paramws->getRow(irow);
      std::string parname;
      double parvalue;
      row >> parname >> parvalue;

      paramvalues.emplace(parname, parvalue);
    }

    return;
  }
};

#endif /* MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERSTEST_H_ */
