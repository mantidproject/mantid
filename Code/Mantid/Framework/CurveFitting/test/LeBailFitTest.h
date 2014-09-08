#ifndef MANTID_CURVEFITTING_LEBAILFITTEST_H_
#define MANTID_CURVEFITTING_LEBAILFITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidCurveFitting/LeBailFit.h"
#include "MantidDataHandling/LoadAscii.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <cmath>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

using namespace std;

using Mantid::CurveFitting::LeBailFit;

class LeBailFitTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFitTest *createSuite() { return new LeBailFitTest(); }
  static void destroySuite( LeBailFitTest *suite ) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test calculation mode on calculating 2 peaks
   *  It is same as LeBailFunctionTest.test_CalculatePeakParameters()
   */
  void Ptest_CalculationSimpleMode()
  {
    // Create input workspaces
    API::MatrixWorkspace_sptr dataws;
    dataws = createInputDataWorkspace(1);

    //  Profile parameters from backgroundless setup
    map<string, double> modmap;
    TableWorkspace_sptr parameterws = createPeakParameterWorkspace(modmap, 1);

    //  Add reflection (111) and (110)
    TableWorkspace_sptr hklws;
    double h110 = 660.0/0.0064;
    double h111 = 1370.0/0.008;
    std::vector<double> peakheights;
    peakheights.push_back(h111); peakheights.push_back(h110);
    std::vector<std::vector<int> > hkls;
    std::vector<int> p111;
    p111.push_back(1); p111.push_back(1); p111.push_back(1);
    hkls.push_back(p111);
    std::vector<int> p110;
    p110.push_back(1); p110.push_back(1); p110.push_back(0);
    hkls.push_back(p110);
    hklws = createInputHKLWorkspace(hkls, peakheights);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

    // Initialize the algorithm
    LeBailFit lbfit;

    TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
    TS_ASSERT(lbfit.isInitialized());

    // 3. Set properties
    lbfit.setProperty("InputWorkspace", "Data");
    lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");

    lbfit.setProperty("InputParameterWorkspace", "PeakParameters");
    lbfit.setProperty("OutputParameterWorkspace", "PeakParameters");

    lbfit.setProperty("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("OutputPeaksWorkspace", "PeakParameterWS");

    lbfit.setProperty("WorkspaceIndex", 0);

    lbfit.setProperty("Function", "Calculation");

    lbfit.setProperty("PeakType", "ThermalNeutronBk2BkExpConvPVoigt");
    lbfit.setProperty("BackgroundType", "Polynomial");
    lbfit.setPropertyValue("BackgroundParameters", "0.0, 0.0, 0.0");

    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);

    lbfit.setProperty("PlotIndividualPeaks", true);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());

    TS_ASSERT(lbfit.isExecuted());

    // 5. Get output
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);
    if (!outws)
    {
      return;
    }

    // 9 fixed + 2 individual peaks
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 11);

    /*
    for (size_t i = 0; i < outws->dataY(0).size(); ++i)
      std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << "\t\t" << outws->dataY(1)[i] << std::endl;
    */

    // 4. Calcualte data
    double y25 = 1366.40;
    double y59 = 0.2857;
    double y86 = 649.464;

    TS_ASSERT_DELTA(outws->readY(1)[25], y25, 0.1);
    TS_ASSERT_DELTA(outws->readY(1)[59], y59, 0.0001);
    TS_ASSERT_DELTA(outws->readY(1)[86], y86, 0.001);

    // 5. Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("CalculatedPeaks");
    AnalysisDataService::Instance().remove("PeakParameterWS");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test calculation mode on calculating 1 peak using Fullprof #9 profile
    * It is same as LeBailFunctionTest.test_calculateLeBailFunctionProf9()
    * Task of this test is to make sure the workflow is correct.
   */
  void test_CalculationSimpleModeProfile9()
  {
    // Create input workspaces
    API::MatrixWorkspace_sptr dataws;
    dataws = createInputDataWorkspace(9);

    //  Profile parameters from backgroundless setup
    map<string, double> modmap;
    TableWorkspace_sptr parameterws = createPeakParameterWorkspace(modmap, 9);

    //  Add reflection (220)
    TableWorkspace_sptr hklws;
    double h220 = 660.0/0.0064;
    std::vector<double> peakheights;
    peakheights.push_back(h220);

    std::vector<std::vector<int> > hkls;
    std::vector<int> p220(3, 2);
    p220[2] = 0;
    hkls.push_back(p220);

    hklws = createInputHKLWorkspace(hkls, peakheights);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

    // Initialize the algorithm
    LeBailFit lbfit;

    TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
    TS_ASSERT(lbfit.isInitialized());

    // 3. Set properties
    lbfit.setProperty("InputWorkspace", "Data");
    lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");

    lbfit.setProperty("InputParameterWorkspace", "PeakParameters");
    lbfit.setProperty("OutputParameterWorkspace", "PeakParameters");

    lbfit.setProperty("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("OutputPeaksWorkspace", "PeakParameterWS");

    lbfit.setProperty("WorkspaceIndex", 0);

    lbfit.setProperty("Function", "Calculation");

    lbfit.setProperty("PeakType", "NeutronBk2BkExpConvPVoigt");
    lbfit.setProperty("BackgroundType", "Polynomial");
    lbfit.setPropertyValue("BackgroundParameters", "0.0, 0.0, 0.0");

    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);

    lbfit.setProperty("PlotIndividualPeaks", true);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());

    TS_ASSERT(lbfit.isExecuted());

    // 5. Get output
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);
    if (!outws)
    {
      return;
    }

    // 9 fixed + 2 individual peaks
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 10);

    return;

    /*
    for (size_t i = 0; i < outws->dataY(0).size(); ++i)
      std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << "\t\t" << outws->dataY(1)[i] << std::endl;
    */

    // 4. Calcualte data
    double y25 = 1366.40;
    double y59 = 0.2857;
    double y86 = 649.464;

    TS_ASSERT_DELTA(outws->readY(1)[25], y25, 0.1);
    TS_ASSERT_DELTA(outws->readY(1)[59], y59, 0.0001);
    TS_ASSERT_DELTA(outws->readY(1)[86], y86, 0.001);

    // 5. Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("CalculatedPeaks");
    AnalysisDataService::Instance().remove("PeakParameterWS");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Test on peak calcualtion with non-trivial background
   */
  void test_CalculationModeFull()
  {
    // 1. Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;

    dataws = createInputDataWorkspace(1);
    map<string, double> emptymap;
    parameterws = createPeakParameterWorkspace(emptymap, 1);

    // a) Add reflection (111) and (110)
    double h110 = 660.0/0.0064;
    double h111 = 1370.0/0.008;
    std::vector<double> peakheights;
    peakheights.push_back(h111); peakheights.push_back(h110);
    std::vector<std::vector<int> > hkls;
    std::vector<int> p111;
    p111.push_back(1); p111.push_back(1); p111.push_back(1);
    hkls.push_back(p111);
    std::vector<int> p110;
    p110.push_back(1); p110.push_back(1); p110.push_back(0);
    hkls.push_back(p110);
    hklws = createInputHKLWorkspace(hkls, peakheights);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

    // 2. Initialize the algorithm
    LeBailFit lbfit;

    TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
    TS_ASSERT(lbfit.isInitialized());

    // 3. Set properties
    lbfit.setPropertyValue("InputWorkspace", "Data");
    lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");
    lbfit.setPropertyValue("InputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("OutputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("OutputPeaksWorkspace", "PeakParameterWS");
    lbfit.setProperty("WorkspaceIndex", 0);
    lbfit.setProperty("BackgroundType", "Polynomial");    
    lbfit.setPropertyValue("BackgroundParameters", "101.0, 0.001"); // a second order polynomial background
    lbfit.setProperty("Function", "Calculation");
    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());
    TS_ASSERT(lbfit.isExecuted());

    // 5. Get output & Test
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);

    // Check background (last point)
    double bkgdx = outws->readX(1).back()*0.001 + 101.0;
    TS_ASSERT_DELTA(outws->readY(1).back(), bkgdx, 1.0);

    // Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("CalculatedPeaks");
    AnalysisDataService::Instance().remove("PeakParameterWS");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit 1 parameter value in a 2 peak pattern
    * Due to the strongly correlated peak parameters, only 1 parameter
    * has its value shifted from true value for unit test purpose
   */
  void test_fit1Parameter()
  {
    std::string testplan("zero");

    // Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;

    // Data.  Option 1
    dataws = createInputDataWorkspace(1);
    // Profile parameter
    std::map<std::string, double> parammodifymap;
    if (testplan.compare("zero") == 0)
    {
      parammodifymap.insert(std::make_pair("Zero", 2.0));
    }
    else if (testplan.compare("alpha") == 0)
    {
      double alph0 = 4.026;
      double newalph0 = alph0*0.05;
      parammodifymap.insert(std::make_pair("Alph0", newalph0));
    }
    else if (testplan.compare("sigma") == 0)
    {
      double sig1 = 9.901;
      double newsig1 = sig1*0.1;
      double sig0 = 127.37;
      double newsig0 = sig0*0.1;
      parammodifymap.insert(std::make_pair("Sig0", newsig0));
      parammodifymap.insert(std::make_pair("Sig1", newsig1));
    }
    parameterws = createPeakParameterWorkspace(parammodifymap, 1);
    // c) Reflection (111) and (110)
    double h110 = 1.0;
    double h111 = 1.0;
    std::vector<double> peakheights;
    peakheights.push_back(h111); peakheights.push_back(h110);
    std::vector<std::vector<int> > hkls;
    std::vector<int> p111;
    p111.push_back(1); p111.push_back(1); p111.push_back(1);
    hkls.push_back(p111);
    std::vector<int> p110;
    p110.push_back(1); p110.push_back(1); p110.push_back(0);
    hkls.push_back(p110);
    hklws = createInputHKLWorkspace(hkls, peakheights);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

    // Initialize LeBaiFit
    LeBailFit lbfit;
    TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
    TS_ASSERT(lbfit.isInitialized());

    // Set properties
    lbfit.setPropertyValue("InputWorkspace", "Data");
    lbfit.setPropertyValue("InputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("OutputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("WorkspaceIndex", 0);
    lbfit.setProperty("Function", "LeBailFit");
    lbfit.setProperty("OutputWorkspace", "FitResultWS");
    lbfit.setProperty("OutputPeaksWorkspace", "PeakInfoWS");
    lbfit.setProperty("PeakRadius", 8);
    lbfit.setPropertyValue("BackgroundType", "Polynomial");
    lbfit.setPropertyValue("BackgroundParameters", "0.01, 0.0, 0.0, 0.0");

    lbfit.setProperty("NumberMinimizeSteps", 1000);

    lbfit.execute();

    // 4. Get output
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (AnalysisDataService::Instance().retrieve("FitResultWS"));
    TS_ASSERT(outws);
    if (!outws)
    {
      return;
    }

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);
    if (outws->getNumberHistograms() != 9)
    {
      return;
    }

    // 5. Check fit result
    DataObjects::TableWorkspace_sptr paramws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>
        (AnalysisDataService::Instance().retrieve("PeakParameters"));
    TS_ASSERT(paramws);
    if (!paramws)
    {
      return;
    }

    TS_ASSERT_EQUALS(paramws->columnCount(), 9);

    std::map<std::string, double> paramvalues;
    std::map<std::string, char> paramfitstatus;
    parseParameterTableWorkspace(paramws, paramvalues, paramfitstatus);

    if (testplan.compare("zero") == 0)
    {
      double zero = paramvalues["Zero"];
      cout << "Zero = " << zero << ".\n";
      TS_ASSERT_DELTA(zero, 0.0, 0.5);
    }
    else if (testplan.compare("alpha") == 0)
    {
      double alph0 = paramvalues["Alph0"];
      TS_ASSERT_DELTA(alph0, 4.026, 1.00);
    }
    else if (testplan.compare("sigma") == 0)
    {
      double sig0 = paramvalues["Sig0"];
      TS_ASSERT_DELTA(sig0, sqrt(17.37), 0.01);
      double sig1 = paramvalues["Sig1"];
      TS_ASSERT_DELTA(sig1, sqrt(9.901), 0.01);
    }

    // 6. Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("FitResultWS");
    AnalysisDataService::Instance().remove("PeakInfoWS");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test a complete LeBail Fit process with background by Monte Carlo algorithm
   *  Using Run 4862 Bank 7 as the testing data
   */
  void Disabled_test_monteCarloLeBailFit_PG3Bank7()
  {
    // 1. Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;
    DataObjects::TableWorkspace_sptr bkgdws;

    // a)  Reflections
    std::vector<std::vector<int> > hkls;
    // importReflectionTxtFile("PG3_Bank7_Peaks.hkl", hkls);
    // (222)
    vector<int> r222(3, 2);
    hkls.push_back(r222);
    // (311)
    vector<int> r311(3, 1); r311[0] = 3;
    hkls.push_back(r311);
    // (220)
    vector<int> r220(3, 2); r220[2] = 0;
    hkls.push_back(r220);
    // (200)
    vector<int> r200(3, 0); r200[0] = 2;
    hkls.push_back(r200);
    // (111)
    vector<int> r111(3, 1);
    hkls.push_back(r111);

    size_t numpeaks = hkls.size();
    std::cout << "[TESTx349] Nmber of (file imported) peaks = " << hkls.size() << std::endl;

    // b) data
    dataws = createInputDataWorkspace(4);
    std::cout << "[TESTx349] Data Workspace Range: " << dataws->readX(0)[0] << ", " << dataws->readX(0).back() << std::endl;

    // c) Generate TableWorkspaces
    std::vector<double> pkheights(numpeaks, 1.0);
    map<string, double> modmap;
    modmap.insert(make_pair("Alph0", 5.0));
    modmap.insert(make_pair("Beta0", 5.0));
    parameterws = createPeakParameterWorkspace(modmap, 2);
    hklws = createInputHKLWorkspace(hkls, pkheights);
    bkgdws = createBackgroundParameterWorksapce(1);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);
    AnalysisDataService::Instance().addOrReplace("BackgroundParameters", bkgdws);

    // 2. Other properties
    std::vector<double> fitregion;
    fitregion.push_back(56198.0);
    fitregion.push_back(151239.0);

    // 3. Genearte LeBailFit algorithm and set it up
    LeBailFit lbfit;
    lbfit.initialize();

    lbfit.setPropertyValue("InputWorkspace", "Data");
    lbfit.setPropertyValue("InputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("WorkspaceIndex", 0);
    lbfit.setProperty("FitRegion", fitregion);
    lbfit.setProperty("Function", "MonteCarlo");
    lbfit.setProperty("BackgroundType", "Polynomial");
    lbfit.setPropertyValue("BackgroundParametersWorkspace", "BackgroundParameters");
    lbfit.setProperty("OutputWorkspace", "FittedData");
    lbfit.setProperty("OutputPeaksWorkspace", "FittedPeaks");
    lbfit.setProperty("OutputParameterWorkspace", "FittedParameters");
    lbfit.setProperty("PeakRadius", 8);
    lbfit.setProperty("Damping", 0.4);
    lbfit.setProperty("NumberMinimizeSteps", 100);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());
    TS_ASSERT(lbfit.isExecuted());
    if (!lbfit.isExecuted())
    {
      return;
    }

    // 5. Exam
    // Take the output data:
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (AnalysisDataService::Instance().retrieve("FittedData"));
    TS_ASSERT(outws);
    if (!outws)
      return;
    else
    {
      TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);
    }

    // Peaks table
    DataObjects::TableWorkspace_sptr peakparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>
        (AnalysisDataService::Instance().retrieve("FittedPeaks"));
    TS_ASSERT(peakparamws);
    if (!peakparamws)
    {
      return;
    }

    else
    {
      TS_ASSERT_EQUALS(peakparamws->rowCount(), 5);
    }

    // Parameters table
    DataObjects::TableWorkspace_sptr instrparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>
        (AnalysisDataService::Instance().retrieve("FittedParameters"));
    TS_ASSERT(instrparamws);
    if (!instrparamws)
      return;

    std::map<std::string, double> paramvalues;
    std::map<std::string, char> paramfitstatus;
    parseParameterTableWorkspace(instrparamws, paramvalues, paramfitstatus);

    double zero = paramvalues["Zero"];
    TS_ASSERT_DELTA(zero, 0.0, 0.5);

    double alph0 = paramvalues["Alph0"];
    TS_ASSERT_DELTA(alph0, 4.026, 1.00);

    double beta0 = paramvalues["Beta0"];
    TS_ASSERT_DELTA(beta0, 4.026, 1.00);

    // Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("FittedData");
    AnalysisDataService::Instance().remove("FittedPeaks");
    AnalysisDataService::Instance().remove("FittedParameters");
    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test refining background.  The data to test against is from NOM 11848-4
    */
  void Xtest_refineBackground()
  {
    // 1. Create input workspace
    // a) Data workspace
    API::MatrixWorkspace_sptr dataws;
    dataws = createInputDataWorkspace(3);
    AnalysisDataService::Instance().addOrReplace("DataB", dataws);

    // b) Parameter table workspace
    DataObjects::TableWorkspace_sptr parameterws;
    map<string, double> modmap;
    parameterws = createPeakParameterWorkspace(modmap, 3);
    AnalysisDataService::Instance().addOrReplace("NOMADBank4", parameterws);

    // c) Reflection (peak 211 @ TOF = 16100)
    vector<vector<int> > peakhkls;
    vector<double> peakheights;
    vector<int> p211(3, 0);
    p211[0] = 2; p211[1] = 1; p211[2] = 1;
    peakhkls.push_back(p211);
    peakheights.push_back(1.0);

    DataObjects::TableWorkspace_sptr hklws;
    hklws = createInputHKLWorkspace(peakhkls, peakheights);
    AnalysisDataService::Instance().addOrReplace("LaB6Reflections", hklws);

    // d) Background
    TableWorkspace_sptr bkgdws = createBackgroundParameterWorksapce(2);
    AnalysisDataService::Instance().addOrReplace("NomB4BackgroundParameters", bkgdws);

    // 2. Initialize the algorithm
    LeBailFit lbfit;

    TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
    TS_ASSERT(lbfit.isInitialized());

    // 3. Set properties
    lbfit.setPropertyValue("InputWorkspace", "DataB");
    lbfit.setProperty("OutputWorkspace", "RefinedBackground");
    lbfit.setPropertyValue("InputParameterWorkspace", "NOMADBank4");
    lbfit.setPropertyValue("OutputParameterWorkspace", "Dummy1");
    lbfit.setPropertyValue("InputHKLWorkspace", "LaB6Reflections");
    lbfit.setProperty("OutputPeaksWorkspace", "Dummy2");
    lbfit.setProperty("WorkspaceIndex", 0);
    lbfit.setProperty("Function", "RefineBackground");
    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);
    lbfit.setProperty("Damping", 0.4);
    lbfit.setProperty("NumberMinimizeSteps", 100);
    lbfit.setProperty("BackgroundParametersWorkspace", "NomB4BackgroundParameters");

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());
    TS_ASSERT(lbfit.isExecuted());

    // 5. Get output
    DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          AnalysisDataService::Instance().retrieve("RefinedBackground"));
    TS_ASSERT(outws);
    if (!outws)
    {
      return;
    }

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);

#if 0
    /*
      for (size_t i = 0; i < outws->dataY(0).size(); ++i)
        std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << "\t\t" << outws->dataY(1)[i] << std::endl;
      */

    // 4. Calcualte data
    double y25 = 1360.20;
    double y59 = 0.285529;
    double y86 = 648.998;

    TS_ASSERT_DELTA(outws->readY(1)[25], y25, 0.1);
    TS_ASSERT_DELTA(outws->readY(1)[59], y59, 0.0001);
    TS_ASSERT_DELTA(outws->readY(1)[86], y86, 0.001);
#endif

    // 5. Clean
    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("RefinedBackground");
    AnalysisDataService::Instance().remove("NOMADBank4");
    AnalysisDataService::Instance().remove("Dummy1");
    AnalysisDataService::Instance().remove("LaB6Reflections");
    AnalysisDataService::Instance().remove("Dummy2");
    AnalysisDataService::Instance().remove("NomB4BackgroundParameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create parameter workspace for peak calculation.
    * If a parameter is to be modifed by absolute value, then this parameter will be fit.
    * @param parammodifymap :: map containing parameter and its value to update from original
    * @param option :: choice to select parameter values.
    */
  TableWorkspace_sptr createPeakParameterWorkspace(map<std::string, double> parammodifymap,
                                                   int option)
  {
    // 1. Set the parameter and fit map
    std::map<std::string, double> paramvaluemap;
    std::map<std::string, std::string> paramfitmap;

    // 2. Get parameters (map) according to option
    switch (option)
    {
      case 1:
        // The backgroundless data
        genPeakParametersBackgroundLessData(paramvaluemap);
        break;

      case 2:
        // Bank 7 w/ background
        genPeakParameterBank7(paramvaluemap);
        break;

      case 3:
        // NOMAD Bank 4
        genPeakParameterNomBank4(paramvaluemap);
        break;

      case 9:
        generateGPPDBank1(paramvaluemap);
        break;

      default:
        // Unsupported
        stringstream errmsg;
        errmsg << "Peak parameters option = " << option << " is not supported." << ".\n"
               << "Supported options are (1) Backgroundless, (2) Background Bank 7, (3) NOMAD Bank4.";
        throw std::invalid_argument(errmsg.str());
        break;
    }
    cout << "Parameter Value Map Size = " << paramvaluemap.size() << std::endl;

    // 3. Fix all peak parameters
    std::map<std::string, double>::iterator mit;
    for (mit = paramvaluemap.begin(); mit != paramvaluemap.end(); ++mit)
      {
          std::string parname = mit->first;
          paramfitmap.insert(std::make_pair(parname, "t"));
      }

    std::cout << "Parameter Fit Map Size = " << paramfitmap.size() << std::endl;

    // 4. Parpare the table workspace
    DataObjects::TableWorkspace *tablews;

    tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr parameterws(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");

    // 5. Add value
    std::map<std::string, double>::iterator paramiter;
    for (paramiter = paramvaluemap.begin(); paramiter != paramvaluemap.end(); ++paramiter)
    {
      // a) Access value from internal parameter maps and parameter to modify map
      std::string parname = paramiter->first;
      double parvalue;
      std::string fit_tie;

      // a) Whether is a parameter w/ value to be modified
      std::map<std::string, double>::iterator moditer;
      moditer = parammodifymap.find(parname);
      if (moditer != parammodifymap.end())
          {
              // Modify
              parvalue = moditer->second;
              fit_tie = "f";
          }
      else
          {
              // Use original
              parvalue = paramiter->second;
              fit_tie = paramfitmap[parname];
          }

      // c) Append to table
      std::cout << parname << ": " << parvalue << "  " << fit_tie << std::endl;

      API::TableRow newparam = parameterws->appendRow();
      newparam << parname << parvalue << fit_tie;
    }

    std::cout << "ParameterWorkspace: Size = " << parameterws->rowCount() << std::endl;

    return parameterws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate peak parameters for the data without background
   */
  void genPeakParametersBackgroundLessData(std::map<std::string, double>& paramvaluemap)
  {
    // a) Value
    paramvaluemap.insert(std::make_pair("Dtt1", 29671.7500));
    paramvaluemap.insert(std::make_pair("Dtt2" ,  0.0 ));
    paramvaluemap.insert(std::make_pair("Dtt1t",  29671.750 ));
    paramvaluemap.insert(std::make_pair("Dtt2t",  0.30 ));
    paramvaluemap.insert(std::make_pair("Zero" ,  0.0  ));
    paramvaluemap.insert(std::make_pair("Zerot",  33.70 ));
    paramvaluemap.insert(std::make_pair("Alph0",  4.026 ));
    paramvaluemap.insert(std::make_pair("Alph1",  7.362 ));
    paramvaluemap.insert(std::make_pair("Beta0",  3.489 ));
    paramvaluemap.insert(std::make_pair("Beta1",  19.535 ));
    paramvaluemap.insert(std::make_pair("Alph0t", 60.683 ));
    paramvaluemap.insert(std::make_pair("Alph1t", 39.730 ));
    paramvaluemap.insert(std::make_pair("Beta0t", 96.864 ));
    paramvaluemap.insert(std::make_pair("Beta1t", 96.864 ));
    paramvaluemap.insert(std::make_pair("Sig2" ,   sqrt(11.380) ));
    paramvaluemap.insert(std::make_pair("Sig1" ,   sqrt(9.901)  ));
    paramvaluemap.insert(std::make_pair("Sig0" ,   sqrt(17.370) ));
    paramvaluemap.insert(std::make_pair("Width",  1.0055 ));
    paramvaluemap.insert(std::make_pair("Tcross", 0.4700 ));
    paramvaluemap.insert(std::make_pair("Gam0" ,  0.0 ));
    paramvaluemap.insert(std::make_pair("Gam1" ,  0.0 ));
    paramvaluemap.insert(std::make_pair("Gam2" ,  0.0 ));
    paramvaluemap.insert(std::make_pair("LatticeConstant", 4.156890));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Genearte peak parameters for data with background.  Bank 7
   */
  void genPeakParameterBank7(std::map<std::string, double>& paramvaluemap)
  {
    paramvaluemap.clear();

    paramvaluemap.insert(std::make_pair("Alph0",  0.5     ));
    paramvaluemap.insert(std::make_pair("Alph0t", 128.96  ));
    paramvaluemap.insert(std::make_pair("Alph1",  0.      ));
    paramvaluemap.insert(std::make_pair("Alph1t", 15.702  ));
    paramvaluemap.insert(std::make_pair("Beta0",  2.0     ));
    paramvaluemap.insert(std::make_pair("Beta0t", 202.28  ));
    paramvaluemap.insert(std::make_pair("Beta1",  0.      ));
    paramvaluemap.insert(std::make_pair("Beta1t", 0.      ));
    paramvaluemap.insert(std::make_pair("CWL",	  4.797   ));
    paramvaluemap.insert(std::make_pair("Dtt1",	  22777.1 ));
    paramvaluemap.insert(std::make_pair("Dtt1t",  22785.4 ));
    paramvaluemap.insert(std::make_pair("Dtt2",   0.0));
    paramvaluemap.insert(std::make_pair("Dtt2t",  0.3     ));
    paramvaluemap.insert(std::make_pair("Gam0",	  0       ));
    paramvaluemap.insert(std::make_pair("Gam1",	  0       ));
    paramvaluemap.insert(std::make_pair("Gam2",	  0       ));
    paramvaluemap.insert(std::make_pair("Profile",	    10      ));
    paramvaluemap.insert(std::make_pair("Sig0",	    0       ));
    paramvaluemap.insert(std::make_pair("Sig1",	    sqrt(10.0)    ));
    paramvaluemap.insert(std::make_pair("Sig2",	    sqrt(15.48) ));
    paramvaluemap.insert(std::make_pair("Tcross",	    0.25    ));
    paramvaluemap.insert(std::make_pair("Width",	    5.8675  ));
    paramvaluemap.insert(std::make_pair("Zero",	    0       ));
    paramvaluemap.insert(std::make_pair("Zerot",	    62.5    ));
    paramvaluemap.insert(std::make_pair("step",	    0.005   ));
    paramvaluemap.insert(std::make_pair("tof-max",	    233.8   ));
    paramvaluemap.insert(std::make_pair("tof-min",	    50.2919 ));
    paramvaluemap.insert(std::make_pair("twotheta",	    90.807  ));
    paramvaluemap.insert(std::make_pair("LatticeConstant",	 9.438));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate peak parameters for NOMAD Bank4
    */
  void genPeakParameterNomBank4(map<std::string, double>& paramvaluemap)
  {
    paramvaluemap.clear();

    paramvaluemap.insert(make_pair("Alph0",	0.886733  ));
    paramvaluemap.insert(make_pair("Alph0t",	114.12    ));
    paramvaluemap.insert(make_pair("Alph1",	8.38073   ));
    paramvaluemap.insert(make_pair("Alph1t",	75.8038   ));
    paramvaluemap.insert(make_pair("Beta0",	3.34888   ));
    paramvaluemap.insert(make_pair("Beta0t",	88.292    ));
    paramvaluemap.insert(make_pair("Beta1",	10.5768   ));
    paramvaluemap.insert(make_pair("Beta1t",	-0.0346847));
    paramvaluemap.insert(make_pair("Dtt1",	9491.56));
    paramvaluemap.insert(make_pair("Dtt1t",	9423.85));
    paramvaluemap.insert(make_pair("Dtt2",	0));
    paramvaluemap.insert(make_pair("Dtt2t",	0.3));
    paramvaluemap.insert(make_pair("Gam0",	0));
    paramvaluemap.insert(make_pair("Gam1",	0));
    paramvaluemap.insert(make_pair("Gam2",	0));
    paramvaluemap.insert(make_pair("LatticeConstant",	4.15689));
    paramvaluemap.insert(make_pair("Sig0",	0));
    paramvaluemap.insert(make_pair("Sig1",	18.3863));
    paramvaluemap.insert(make_pair("Sig2",	0.671019));
    paramvaluemap.insert(make_pair("Tcross",	0.4373));
    paramvaluemap.insert(make_pair("Width",	2.9654));
    paramvaluemap.insert(make_pair("Zero",	0));
    paramvaluemap.insert(make_pair("Zerot",	101.618));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate peak parameters for GPPD bank 1 from arg_si.pcr (Fullprof example)
    */
  void generateGPPDBank1(map<std::string, double>& parammap)
  {
    parammap.insert(make_pair("Dtt1", 16370.650));
    parammap.insert(make_pair("Dtt2", 0.10));
    parammap.insert(make_pair("Zero", 0.0));

    parammap.insert(make_pair("Alph0", 1.0));
    parammap.insert(make_pair("Alph1", 0.0));
    parammap.insert(make_pair("Beta0", 0.109036));
    parammap.insert(make_pair("Beta1", 0.009834));

    parammap.insert(make_pair("Sig2",  sqrt(91.127)));
    parammap.insert(make_pair("Sig1",  sqrt(1119.230)));
    parammap.insert(make_pair("Sig0",  sqrt(0.0)));

    parammap.insert(make_pair("Gam0", 0.0));
    parammap.insert(make_pair("Gam1", 7.688));
    parammap.insert(make_pair("Gam2", 0.0));

    parammap.insert(make_pair("LatticeConstant", 5.431363));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create reflection table workspaces
   */
  DataObjects::TableWorkspace_sptr createInputHKLWorkspace(std::vector<std::vector<int> > hkls, std::vector<double> heights)
  {
      // 0. Check
      if (hkls.size() != heights.size())
      {
          std::cout << "createInputHKLWorkspace: input two vectors have different sizes.  It is not supported." << std::endl;
          throw std::invalid_argument("Vectors for HKL and heights are of different sizes.");
      }

      // 1. Crate table workspace
      DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
      DataObjects::TableWorkspace_sptr hklws = DataObjects::TableWorkspace_sptr(tablews);

      tablews->addColumn("int", "H");
      tablews->addColumn("int", "K");
      tablews->addColumn("int", "L");
      tablews->addColumn("double", "PeakHeight");

      // 2. Add reflections and heights
      for (size_t ipk = 0; ipk < hkls.size(); ++ipk)
      {
          API::TableRow hkl = hklws->appendRow();
          for (size_t i = 0; i < 3; ++i)
          {
              hkl << hkls[ipk][i];
          }
          hkl << heights[ipk];
      }

      return hklws;
  }

  //----------------------------------------------------------------------------------------------
  /** Create data workspace without background
   */
  API::MatrixWorkspace_sptr createInputDataWorkspace(int option)
  {    
    API::MatrixWorkspace_sptr dataws;

    if (option != 4)
    {
      // Generate data

      std::vector<double> vecX;
      std::vector<double> vecY;
      std::vector<double> vecE;

      // a) Generate data
      switch (option)
      {
        case 1:
          generateSeparateTwoPeaksData2(vecX, vecY, vecE);
          break;

        case 2:
          generateTwinPeakData(vecX, vecY, vecE);
          break;

        case 3:
          generate1PeakDataPlusBackground(vecX, vecY, vecE);
          break;

        case 9:
          generateArgSiPeak220(vecX, vecY, vecE);
          break;

        default:
          stringstream errmsg;
          errmsg << "Option " << option << " to generate a data workspace is  not supported.";
          throw runtime_error(errmsg.str());
          break;
      }

      // b) Get workspace
      size_t nHist = 1;
      size_t nBins = vecX.size();

      dataws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            API::WorkspaceFactory::Instance().create("Workspace2D", nHist, nBins, nBins));

      // c) Input data
      for (size_t i = 0; i < vecX.size(); ++i)
      {
        dataws->dataX(0)[i] = vecX[i];
        dataws->dataY(0)[i] = vecY[i];
        dataws->dataE(0)[i] = vecE[i];
      }

    }
    else if (option == 4)
    {
      // Load from column file
      throw runtime_error("Using .dat file is not allowed for committing. ");
      string datafilename("PG3_4862_Bank7.dat");
      string wsname("Data");
      importDataFromColumnFile(datafilename, wsname);
      dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(wsname));
    }
    else
    {
      // not supported
      throw std::invalid_argument("Logic error. ");
    }

    return dataws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a set of powder diffraction data with 2 peaks w/o background
   */
  void generateSeparateTwoPeaksData2(vector<double>& vecX, vector<double>& vecY, vector<double>& vecE)
  {
    vecX.push_back(70931.750);    vecY.push_back(    0.0000000    );
    vecX.push_back(70943.609);    vecY.push_back(    0.0000000    );
    vecX.push_back(70955.477);    vecY.push_back(   0.69562334    );
    vecX.push_back(70967.336);    vecY.push_back(   0.99016321    );
    vecX.push_back(70979.203);    vecY.push_back(    1.4097446    );
    vecX.push_back(70991.063);    vecY.push_back(    2.0066566    );
    vecX.push_back(71002.930);    vecY.push_back(    2.8569770    );
    vecX.push_back(71014.789);    vecY.push_back(    4.0666742    );
    vecX.push_back(71026.656);    vecY.push_back(    5.7899261    );
    vecX.push_back(71038.516);    vecY.push_back(    8.2414885    );
    vecX.push_back(71050.383);    vecY.push_back(    11.733817    );
    vecX.push_back(71062.242);    vecY.push_back(    16.702133    );
    vecX.push_back(71074.109);    vecY.push_back(    23.779659    );
    vecX.push_back(71085.969);    vecY.push_back(    33.848408    );
    vecX.push_back(71097.836);    vecY.push_back(    48.191662    );
    vecX.push_back(71109.695);    vecY.push_back(    68.596909    );
    vecX.push_back(71121.563);    vecY.push_back(    97.664757    );
    vecX.push_back(71133.430);    vecY.push_back(    139.04889    );
    vecX.push_back(71145.289);    vecY.push_back(    197.90808    );
    vecX.push_back(71157.156);    vecY.push_back(    281.60803    );
    vecX.push_back(71169.016);    vecY.push_back(    399.65021    );
    vecX.push_back(71180.883);    vecY.push_back(    562.42670    );
    vecX.push_back(71192.742);    vecY.push_back(    773.34192    );
    vecX.push_back(71204.609);    vecY.push_back(    1015.2813    );
    vecX.push_back(71216.469);    vecY.push_back(    1238.3613    );
    vecX.push_back(71228.336);    vecY.push_back(    1374.9380    );
    vecX.push_back(71240.195);    vecY.push_back(    1380.5173    );
    vecX.push_back(71252.063);    vecY.push_back(    1266.3978    );
    vecX.push_back(71263.922);    vecY.push_back(    1086.2141    );
    vecX.push_back(71275.789);    vecY.push_back(    894.75891    );
    vecX.push_back(71287.648);    vecY.push_back(    723.46112    );
    vecX.push_back(71299.516);    vecY.push_back(    581.04535    );
    vecX.push_back(71311.375);    vecY.push_back(    465.93588    );
    vecX.push_back(71323.242);    vecY.push_back(    373.45383    );
    vecX.push_back(71335.102);    vecY.push_back(    299.35800    );
    vecX.push_back(71346.969);    vecY.push_back(    239.92720    );
    vecX.push_back(71358.836);    vecY.push_back(    192.29497    );
    vecX.push_back(71370.695);    vecY.push_back(    154.14153    );
    vecX.push_back(71382.563);    vecY.push_back(    123.54013    );
    vecX.push_back(71394.422);    vecY.push_back(    99.028404    );
    vecX.push_back(71406.289);    vecY.push_back(    79.368507    );
    vecX.push_back(71418.148);    vecY.push_back(    63.620914    );
    vecX.push_back(71430.016);    vecY.push_back(    50.990391    );
    vecX.push_back(71441.875);    vecY.push_back(    40.873333    );
    vecX.push_back(71453.742);    vecY.push_back(    32.758839    );
    vecX.push_back(71465.602);    vecY.push_back(    26.259121    );
    vecX.push_back(71477.469);    vecY.push_back(    21.045954    );
    vecX.push_back(71489.328);    vecY.push_back(    16.870203    );
    vecX.push_back(71501.195);    vecY.push_back(    13.520998    );
    vecX.push_back(71513.055);    vecY.push_back(    10.838282    );
    vecX.push_back(71524.922);    vecY.push_back(    8.6865807    );
    vecX.push_back(71536.781);    vecY.push_back(    6.9630671    );
    vecX.push_back(71548.648);    vecY.push_back(    5.5807042    );
    vecX.push_back(71560.508);    vecY.push_back(    4.4734306    );
    vecX.push_back(71572.375);    vecY.push_back(    3.5853302    );
    vecX.push_back(71584.242);    vecY.push_back(    2.8735423    );
    vecX.push_back(71596.102);    vecY.push_back(    2.3033996    );
    vecX.push_back(71607.969);    vecY.push_back(    1.8461106    );
    vecX.push_back(71619.828);    vecY.push_back(    0.0000000    );
    vecX.push_back(86911.852);    vecY.push_back(   0.28651541    );
    vecX.push_back(86923.719);    vecY.push_back(   0.39156997    );
    vecX.push_back(86935.578);    vecY.push_back(   0.53503412    );
    vecX.push_back(86947.445);    vecY.push_back(   0.73121130    );
    vecX.push_back(86959.305);    vecY.push_back(   0.99911392    );
    vecX.push_back(86971.172);    vecY.push_back(    1.3654519    );
    vecX.push_back(86983.039);    vecY.push_back(    1.8661126    );
    vecX.push_back(86994.898);    vecY.push_back(    2.5498226    );
    vecX.push_back(87006.766);    vecY.push_back(    3.4847479    );
    vecX.push_back(87018.625);    vecY.push_back(    4.7614965    );
    vecX.push_back(87030.492);    vecY.push_back(    6.5073609    );
    vecX.push_back(87042.352);    vecY.push_back(    8.8915405    );
    vecX.push_back(87054.219);    vecY.push_back(    12.151738    );
    vecX.push_back(87066.078);    vecY.push_back(    16.603910    );
    vecX.push_back(87077.945);    vecY.push_back(    22.691912    );
    vecX.push_back(87089.805);    vecY.push_back(    31.005537    );
    vecX.push_back(87101.672);    vecY.push_back(    42.372311    );
    vecX.push_back(87113.531);    vecY.push_back(    57.886639    );
    vecX.push_back(87125.398);    vecY.push_back(    79.062233    );
    vecX.push_back(87137.258);    vecY.push_back(    107.82082    );
    vecX.push_back(87149.125);    vecY.push_back(    146.58661    );
    vecX.push_back(87160.984);    vecY.push_back(    197.83006    );
    vecX.push_back(87172.852);    vecY.push_back(    263.46185    );
    vecX.push_back(87184.711);    vecY.push_back(    343.08966    );
    vecX.push_back(87196.578);    vecY.push_back(    432.57846    );
    vecX.push_back(87208.445);    vecY.push_back(    522.64124    );
    vecX.push_back(87220.305);    vecY.push_back(    600.01373    );
    vecX.push_back(87232.172);    vecY.push_back(    651.22260    );
    vecX.push_back(87244.031);    vecY.push_back(    667.17743    );
    vecX.push_back(87255.898);    vecY.push_back(    646.90039    );
    vecX.push_back(87267.758);    vecY.push_back(    597.38873    );
    vecX.push_back(87279.625);    vecY.push_back(    530.12573    );
    vecX.push_back(87291.484);    vecY.push_back(    456.83890    );
    vecX.push_back(87303.352);    vecY.push_back(    386.05295    );
    vecX.push_back(87315.211);    vecY.push_back(    322.58456    );
    vecX.push_back(87327.078);    vecY.push_back(    267.96231    );
    vecX.push_back(87338.938);    vecY.push_back(    222.04863    );
    vecX.push_back(87350.805);    vecY.push_back(    183.80043    );
    vecX.push_back(87362.664);    vecY.push_back(    152.11101    );
    vecX.push_back(87374.531);    vecY.push_back(    125.85820    );
    vecX.push_back(87386.391);    vecY.push_back(    104.14707    );
    vecX.push_back(87398.258);    vecY.push_back(    86.170067    );
    vecX.push_back(87410.117);    vecY.push_back(    71.304932    );
    vecX.push_back(87421.984);    vecY.push_back(    58.996807    );
    vecX.push_back(87433.844);    vecY.push_back(    48.819309    );
    vecX.push_back(87445.711);    vecY.push_back(    40.392483    );
    vecX.push_back(87457.578);    vecY.push_back(    33.420235    );
    vecX.push_back(87469.438);    vecY.push_back(    27.654932    );
    vecX.push_back(87481.305);    vecY.push_back(    22.881344    );
    vecX.push_back(87493.164);    vecY.push_back(    18.934097    );
    vecX.push_back(87505.031);    vecY.push_back(    15.665835    );
    vecX.push_back(87516.891);    vecY.push_back(    12.963332    );
    vecX.push_back(87528.758);    vecY.push_back(    10.725698    );
    vecX.push_back(87540.617);    vecY.push_back(    8.8754158    );
    vecX.push_back(87552.484);    vecY.push_back(    7.3434072    );
    vecX.push_back(87564.344);    vecY.push_back(    6.0766010    );
    vecX.push_back(87576.211);    vecY.push_back(    5.0277033    );
    vecX.push_back(87588.070);    vecY.push_back(    4.1603775    );
    vecX.push_back(87599.938);    vecY.push_back(    3.4422443    );
    vecX.push_back(87611.797);    vecY.push_back(    2.8484249    );
    vecX.push_back(87623.664);    vecY.push_back(    2.3567512    );
    vecX.push_back(87635.523);    vecY.push_back(    1.9501896    );
    vecX.push_back(87647.391);    vecY.push_back(    1.6135623    );
    vecX.push_back(87659.250);    vecY.push_back(    1.3352078    );
    vecX.push_back(87671.117);    vecY.push_back(    1.1047342    );
    vecX.push_back(87682.984);    vecY.push_back(   0.91404319    );
    vecX.push_back(87694.844);    vecY.push_back(   0.75636220    );
    vecX.push_back(87706.711);    vecY.push_back(    0.0000000    );

    for (size_t i = 0; i < vecY.size(); ++i)
    {
      double e = 1.0;
      if (vecY[i] > 1.0)
        e = sqrt(vecY[i]);
      vecE.push_back(e);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate data (vectors) containg twin peak w/o background
   */
  void generateTwinPeakData(std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    // These data of reflection (932) and (852)
    vecX.push_back(12646.470);    vecY.push_back(  0.56916749     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12658.333);    vecY.push_back(  0.35570398     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12670.196);    vecY.push_back(  0.85166878     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12682.061);    vecY.push_back(   4.6110063     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12693.924);    vecY.push_back(   24.960907     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12705.787);    vecY.push_back(   135.08231     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12717.650);    vecY.push_back(   613.15887     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12729.514);    vecY.push_back(   587.66174     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12741.378);    vecY.push_back(   213.99724     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12753.241);    vecY.push_back(   85.320320     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12765.104);    vecY.push_back(   86.317253     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12776.968);    vecY.push_back(   334.30905     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12788.831);    vecY.push_back(   1171.0187     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12800.695);    vecY.push_back(   732.47943     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12812.559);    vecY.push_back(   258.37717     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12824.422);    vecY.push_back(   90.549515     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12836.285);    vecY.push_back(   31.733501     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12848.148);    vecY.push_back(   11.121155     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12860.013);    vecY.push_back(   3.9048645     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12871.876);    vecY.push_back(  4.15836312E-02 );  vecE.push_back(  1000.0000 );
    vecX.push_back(12883.739);    vecY.push_back(  0.22341134     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12895.603);    vecY.push_back(   1.2002950     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12907.466);    vecY.push_back(   6.4486742     );  vecE.push_back(  1000.0000 );

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate data with background
    * The data comes from NOMAD 11848-4 (bank 4)
    */
  void generate1PeakDataPlusBackground(vector<double>& vecX, vector<double>& vecY, vector<double>& vecE)
  {
    vecX.push_back(15804.51508); vecY.push_back(  0.00093899);      vecE.push_back( 0.00182963);
    vecX.push_back(15819.15517); vecY.push_back(  0.00345301);      vecE.push_back( 0.00182634);
    vecX.push_back(15833.80882); vecY.push_back( -0.00091186);      vecE.push_back( 0.00183490);
    vecX.push_back(15848.47604); vecY.push_back(  0.00188472);      vecE.push_back( 0.00182437);
    vecX.push_back(15863.15685); vecY.push_back(  0.00332765);      vecE.push_back( 0.00185097);
    vecX.push_back(15877.85126); vecY.push_back(  0.00364515);      vecE.push_back( 0.00183573);
    vecX.push_back(15892.55929); vecY.push_back(  0.00218618);      vecE.push_back( 0.00184518);
    vecX.push_back(15907.28093); vecY.push_back(  0.00181782);      vecE.push_back( 0.00186918);
    vecX.push_back(15922.01622); vecY.push_back(  0.00183030);      vecE.push_back( 0.00188213);
    vecX.push_back(15936.76515); vecY.push_back(  0.00261025);      vecE.push_back( 0.00189781);
    vecX.push_back(15951.52774); vecY.push_back(  0.00775414);      vecE.push_back( 0.00191501);
    vecX.push_back(15966.30401); vecY.push_back(  0.01119628);      vecE.push_back( 0.00193190);
    vecX.push_back(15981.09397); vecY.push_back(  0.02129512);      vecE.push_back( 0.00196919);
    vecX.push_back(15995.89763); vecY.push_back(  0.03490967);      vecE.push_back( 0.00205366);
    vecX.push_back(16010.71500); vecY.push_back(  0.06945186);      vecE.push_back( 0.00222871);
    vecX.push_back(16025.54610); vecY.push_back(  0.11997786);      vecE.push_back( 0.00246872);
    vecX.push_back(16040.39093); vecY.push_back(  0.21313078);      vecE.push_back( 0.00283099);
    vecX.push_back(16055.24952); vecY.push_back(  0.32872762);      vecE.push_back( 0.00323105);
    vecX.push_back(16070.12187); vecY.push_back(  0.46376577);      vecE.push_back( 0.00366236);
    vecX.push_back(16085.00799); vecY.push_back(  0.60672834);      vecE.push_back( 0.00406101);
    vecX.push_back(16099.90791); vecY.push_back(  0.70995429);      vecE.push_back( 0.00433328);
    vecX.push_back(16114.82163); vecY.push_back(  0.72737104);      vecE.push_back( 0.00439982);
    vecX.push_back(16129.74916); vecY.push_back(  0.68092272);      vecE.push_back( 0.00430344);
    vecX.push_back(16144.69052); vecY.push_back(  0.56167618);      vecE.push_back( 0.00401318);
    vecX.push_back(16159.64572); vecY.push_back(  0.42685691);      vecE.push_back( 0.00363757);
    vecX.push_back(16174.61478); vecY.push_back(  0.30260402);      vecE.push_back( 0.00325554);
    vecX.push_back(16189.59770); vecY.push_back(  0.20770640);      vecE.push_back( 0.00292711);
    vecX.push_back(16204.59450); vecY.push_back(  0.14654898);      vecE.push_back( 0.00268130);
    vecX.push_back(16219.60519); vecY.push_back(  0.09628758);      vecE.push_back( 0.00247655);
    vecX.push_back(16234.62979); vecY.push_back(  0.06952267);      vecE.push_back( 0.00234315);
    vecX.push_back(16249.66830); vecY.push_back(  0.04493752);      vecE.push_back( 0.00227152);
    vecX.push_back(16264.72074); vecY.push_back(  0.03126838);      vecE.push_back( 0.00219436);
    vecX.push_back(16279.78713); vecY.push_back(  0.02455495);      vecE.push_back( 0.00216714);
    vecX.push_back(16294.86748); vecY.push_back(  0.02071602);      vecE.push_back( 0.00213767);
    vecX.push_back(16309.96179); vecY.push_back(  0.01423849);      vecE.push_back( 0.00210673);
    vecX.push_back(16325.07009); vecY.push_back(  0.01083945);      vecE.push_back( 0.00210373);
    vecX.push_back(16340.19238); vecY.push_back(  0.00952175);      vecE.push_back( 0.00209212);
    vecX.push_back(16355.32868); vecY.push_back(  0.00666464);      vecE.push_back( 0.00210106);
    vecX.push_back(16370.47900); vecY.push_back(  0.00483277);      vecE.push_back( 0.00210164);
    vecX.push_back(16385.64335); vecY.push_back(  0.00606602);      vecE.push_back( 0.00208481);
    vecX.push_back(16400.82175); vecY.push_back(  0.00797912);      vecE.push_back( 0.00211046);
    vecX.push_back(16416.01421); vecY.push_back(  0.00337981);      vecE.push_back( 0.00209148);
    vecX.push_back(16431.22075); vecY.push_back(  0.00695986);      vecE.push_back( 0.00209749);
    vecX.push_back(16446.44137); vecY.push_back(  0.00076425);      vecE.push_back( 0.00212240);
    vecX.push_back(16461.67609); vecY.push_back( -0.00174803);      vecE.push_back( 0.00212156);
    vecX.push_back(16476.92492); vecY.push_back(  0.00311692);      vecE.push_back( 0.00211736);
    vecX.push_back(16492.18788); vecY.push_back(  0.00267084);      vecE.push_back( 0.00212599);
    vecX.push_back(16507.46497); vecY.push_back(  0.00073160);      vecE.push_back( 0.00217523);
    vecX.push_back(16522.75622); vecY.push_back(  0.00181373);      vecE.push_back( 0.00215910);
    vecX.push_back(16538.06163); vecY.push_back( -0.00060530);      vecE.push_back( 0.00217643);
    vecX.push_back(16553.38122); vecY.push_back( -0.00347549);      vecE.push_back( 0.00217984);
    vecX.push_back(16568.71501); vecY.push_back(  0.00351226);      vecE.push_back( 0.00218813);
    vecX.push_back(16584.06299); vecY.push_back( -0.00079566);      vecE.push_back( 0.00220368);
    vecX.push_back(16599.42519); vecY.push_back(  0.00651456);      vecE.push_back( 0.00224274);
    vecX.push_back(16614.80163); vecY.push_back(  0.01027626);      vecE.push_back( 0.00222865);
    vecX.push_back(16630.19230); vecY.push_back(  0.00498366);      vecE.push_back( 0.00224692);
    vecX.push_back(16645.59723); vecY.push_back(  0.00692367);      vecE.push_back( 0.00223901);
    vecX.push_back(16661.01644); vecY.push_back(  0.00772229);      vecE.push_back( 0.00223212);
    vecX.push_back(16676.44992); vecY.push_back(  0.00603627);      vecE.push_back( 0.00228530);
    vecX.push_back(16691.89770); vecY.push_back(  0.00332977);      vecE.push_back( 0.00225513);
    vecX.push_back(16707.35980); vecY.push_back(  0.00292870);      vecE.push_back( 0.00231030);
    vecX.push_back(16722.83621); vecY.push_back(  0.00736778);      vecE.push_back( 0.00228117);
    vecX.push_back(16738.32696); vecY.push_back(  0.00150402);      vecE.push_back( 0.00232609);
    vecX.push_back(16753.83206); vecY.push_back(  0.00240275);      vecE.push_back( 0.00227347);
    vecX.push_back(16769.35153); vecY.push_back(  0.00426276);      vecE.push_back( 0.00231366);
    vecX.push_back(16784.88537); vecY.push_back(  0.00186002);      vecE.push_back( 0.00231086);
    vecX.push_back(16800.43359); vecY.push_back(  0.00271200);      vecE.push_back( 0.00231613);
    vecX.push_back(16815.99622); vecY.push_back(  0.00157441);      vecE.push_back( 0.00233310);
    vecX.push_back(16831.57327); vecY.push_back( -0.00180279);      vecE.push_back( 0.00234767);
    vecX.push_back(16847.16475); vecY.push_back(  0.00082487);      vecE.push_back( 0.00233778);
    vecX.push_back(16862.77067); vecY.push_back( -0.00336791);      vecE.push_back( 0.00234414);
    vecX.push_back(16878.39104); vecY.push_back( -0.00327705);      vecE.push_back( 0.00234013);
    vecX.push_back(16894.02589); vecY.push_back( -0.00199679);      vecE.push_back( 0.00234771);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate backgroundless peak 220 from arg_si.dat (Fullprof example)
    */
  void generateArgSiPeak220(std::vector<double>& vecx, std::vector<double>& vecy, std::vector<double>& vece)
  {
    vecx.push_back(31019.30000);        vecy.push_back(0.02624178);         vece.push_back(0.00092672);
    vecx.push_back(31050.40000);        vecy.push_back(0.02646138);         vece.push_back(0.00093232);
    vecx.push_back(31081.40000);        vecy.push_back(0.02809566);         vece.push_back(0.00096305);
    vecx.push_back(31112.50000);        vecy.push_back(0.02896440);         vece.push_back(0.00097980);
    vecx.push_back(31143.60000);        vecy.push_back(0.02861105);         vece.push_back(0.00097545);
    vecx.push_back(31174.80000);        vecy.push_back(0.03432836);         vece.push_back(0.00107344);
    vecx.push_back(31205.90000);        vecy.push_back(0.03941826);         vece.push_back(0.00115486);
    vecx.push_back(31237.10000);        vecy.push_back(0.05355697);         vece.push_back(0.00135755);
    vecx.push_back(31268.40000);        vecy.push_back(0.09889440);         vece.push_back(0.00188719);
    vecx.push_back(31299.60000);        vecy.push_back(0.20556772);         vece.push_back(0.00285447);
    vecx.push_back(31330.90000);        vecy.push_back(0.43901506);         vece.push_back(0.00456425);
    vecx.push_back(31362.30000);        vecy.push_back(0.81941730);         vece.push_back(0.00702201);
    vecx.push_back(31393.60000);        vecy.push_back(1.33883897);         vece.push_back(0.01019324);
    vecx.push_back(31425.00000);        vecy.push_back(1.74451085);         vece.push_back(0.01262540);
    vecx.push_back(31456.50000);        vecy.push_back(1.83429503);         vece.push_back(0.01317582);
    vecx.push_back(31487.90000);        vecy.push_back(1.53455479);         vece.push_back(0.01141480);
    vecx.push_back(31519.40000);        vecy.push_back(1.03117425);         vece.push_back(0.00839135);
    vecx.push_back(31550.90000);        vecy.push_back(0.52893114);         vece.push_back(0.00522327);
    vecx.push_back(31582.50000);        vecy.push_back(0.23198354);         vece.push_back(0.00311024);
    vecx.push_back(31614.10000);        vecy.push_back(0.10961397);         vece.push_back(0.00203244);
    vecx.push_back(31645.70000);        vecy.push_back(0.06396058);         vece.push_back(0.00152266);
    vecx.push_back(31677.30000);        vecy.push_back(0.04880334);         vece.push_back(0.00132322);
    vecx.push_back(31709.00000);        vecy.push_back(0.03836045);         vece.push_back(0.00116918);
    vecx.push_back(31740.70000);        vecy.push_back(0.03639256);         vece.push_back(0.00113951);
    vecx.push_back(31772.50000);        vecy.push_back(0.03248324);         vece.push_back(0.00107658);
    vecx.push_back(31804.20000);        vecy.push_back(0.03096179);         vece.push_back(0.00105191);

    for (size_t i = 0; i < vecy.size(); ++i)
      vecy[i] -= 0.02295189;

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Import text file containing reflections (HKL)
   */
  void importReflectionTxtFile(std::string filename, std::vector<std::vector<int> >& hkls)
  {
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open())
    {
      stringstream errss;
      errss << "File " << filename << " cannot be opened. ";
      std::cout << errss.str() << std::endl;
      throw std::invalid_argument(errss.str());
    }
    else
    {
      std::cout << "[TEST] Import file " << filename << " for reflections (HKL)." << std::endl;
    }

    char line[256];
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        int h, k, l;
        std::vector<int> hkl;
        std::stringstream ss;
        ss.str(line);
        ss >> h >> k >> l;
        hkl.push_back(h);
        hkl.push_back(k);
        hkl.push_back(l);
        hkls.push_back(hkl);
      }
    }

    ins.close();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Import data from a column data file
   */
  void importDataFromColumnFile(std::string filename, std::string wsname)
  {
    DataHandling::LoadAscii load;
    load.initialize();

    load.setProperty("FileName", filename);
    load.setProperty("OutputWorkspace", wsname);
    load.setProperty("Separator", "Automatic");
    load.setProperty("Unit", "TOF");

    load.execute();
    if (!load.isExecuted())
    {
      stringstream errss;
      errss << "Data file " << filename << " cannot be opened. ";
      std::cout << errss.str() << std::endl;
      throw std::runtime_error(errss.str());
    }

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve(wsname));
    if (!ws)
    {
      stringstream errss;
      errss << "LoadAscii failed to generate workspace";
      std::cout << errss.str() << std::endl;
      throw std::runtime_error(errss.str());
    }

    // Set error
    const MantidVec& vecY = ws->readY(0);
    MantidVec& vecE = ws->dataE(0);
    size_t numpts = vecY.size();
    for (size_t i = 0; i < numpts; ++i)
    {
      double y = vecY[i];
      double e = 1.0;
      if (y > 1.0)
        e = sqrt(y);
      vecE[i] = e;
    }

    return;
  }

  /// ===============================  Check Results =================================== ///

  /*
   * Parse parameter table workspace to 2 map
   */
  void parseParameterTableWorkspace(DataObjects::TableWorkspace_sptr paramws,
                                    std::map<std::string, double>& paramvalues,
                                    std::map<std::string, char>& paramfitstatus)
  {

      for (size_t irow = 0; irow < paramws->rowCount(); ++irow)
      {
          API::TableRow row = paramws->getRow(irow);
          std::string parname;
          double parvalue;
          std::string fitstatus;
          row >> parname >> parvalue >> fitstatus;

          char fitortie = 't';
          if (fitstatus.size() > 0)
          {
              fitortie = fitstatus[0];
          }
          else
          {
              std::cout << "ParameterWorkspace:  parameter " << parname << " has am empty field for fit/tie. " << std::endl;
          }

          paramvalues.insert(std::make_pair(parname, parvalue));
          paramfitstatus.insert(std::make_pair(parname, fitortie));

      }

      return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a table worskpace for background parameters
    * Note: It is just desired for bank 7 run 4862
   */
  DataObjects::TableWorkspace_sptr createBackgroundParameterWorksapce(int option)
  {
    // 1. Create map
    map<string, double> bkgdparmap;
    switch (option)
    {
      case 1:
        bkgdparmap.insert(make_pair("A0",  -197456));
        bkgdparmap.insert(make_pair("A1",  15.5819));
        bkgdparmap.insert(make_pair("A2",  -0.000467362));
        bkgdparmap.insert(make_pair("A3",  5.59069e-09));
        bkgdparmap.insert(make_pair("A4",  2.81875e-14));
        bkgdparmap.insert(make_pair("A5",  -1.88986e-18));
        bkgdparmap.insert(make_pair("A6",  2.9137e-23));
        bkgdparmap.insert(make_pair("A7",  -2.50121e-28));
        bkgdparmap.insert(make_pair("A8",  1.3279e-33));
        bkgdparmap.insert(make_pair("A9",  -4.33776e-39));
        bkgdparmap.insert(make_pair("A10", 8.01018e-45));
        bkgdparmap.insert(make_pair("A11", -6.40846e-51));

        break;

      case 2:
        // NOMAD Bank4
        bkgdparmap.insert(make_pair("A0",  0.73));
        bkgdparmap.insert(make_pair("A1",  -8.0E-5));
        bkgdparmap.insert(make_pair("A2",  0.0));
        bkgdparmap.insert(make_pair("A3",  0.0));
        bkgdparmap.insert(make_pair("A4",  0.0));
        bkgdparmap.insert(make_pair("A5",  0.0));

        break;

      default:
        stringstream errss;
        errss << "Option " << option << " is not supported.";
        throw runtime_error(errss.str());
        break;
    }

    // 2. Build table workspace
    DataObjects::TableWorkspace* tablewsptr = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr tablews(tablewsptr);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");

    map<string, double>::iterator mapiter;
    for (mapiter = bkgdparmap.begin(); mapiter != bkgdparmap.end(); ++mapiter)
    {
      string parname = mapiter->first;
      double parvalue = mapiter->second;

      API::TableRow newrow = tablews->appendRow();
      newrow << parname << parvalue;
    }

    return tablews;
  }



};


#endif /* MANTID_CURVEFITTING_LEBAILFITTEST_H_ */
