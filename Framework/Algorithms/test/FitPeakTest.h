#ifndef MANTID_ALGORITHMS_FITPEAKTEST_H_
#define MANTID_ALGORITHMS_FITPEAKTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FitPeak.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/TableRow.h"

using Mantid::Algorithms::FitPeak;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

class FitPeakTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeakTest *createSuite() {
    API::FrameworkManager::Instance();
    return new FitPeakTest();
  }
  static void destroySuite(FitPeakTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test on init and setup
    */
  void test_Init() {
    // Generate input workspace
    MatrixWorkspace_sptr dataws = gen_4866P5Data();
    AnalysisDataService::Instance().addOrReplace("PG3_4866Peak5", dataws);

    // Generate peak and background parameters
    vector<string> peakparnames, bkgdparnames;
    vector<double> peakparvalues, bkgdparvalues;

    gen_BkgdParameters(bkgdparnames, bkgdparvalues);
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeak fitpeak;

    fitpeak.initialize();
    TS_ASSERT(fitpeak.isInitialized());

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setPropertyValue("InputWorkspace", "PG3_4866Peak5"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue(
        "ParameterTableWorkspace", "Fitted_Peak5_Parameters"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("WorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterNames", bkgdparnames));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterValues", bkgdparvalues));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setPropertyValue("FitWindow", "10.0, 20.0"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setPropertyValue("PeakRange", "11.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("FitBackgroundFirst", true));

    // Clean
    AnalysisDataService::Instance().remove("PG3_4866Peak5");
    AnalysisDataService::Instance().remove("Peak5_Parameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on fit a peak with significantly high background
    */
  void test_FitPeakWithHighBkgd() {
    // Generate input workspace
    MatrixWorkspace_sptr dataws = gen_4866P5Data();
    AnalysisDataService::Instance().addOrReplace("PG3_4866Peak5", dataws);

    // Generate peak and background parameters
    vector<string> peakparnames, bkgdparnames;
    vector<double> peakparvalues, bkgdparvalues;

    gen_BkgdParameters(bkgdparnames, bkgdparvalues);
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeak fitpeak;
    fitpeak.initialize();

    // Set up properties
    fitpeak.setPropertyValue("InputWorkspace", "PG3_4866Peak5");
    fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak");
    fitpeak.setPropertyValue("ParameterTableWorkspace",
                             "Fitted_Peak5_Parameters");
    fitpeak.setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterNames", bkgdparnames));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterValues", bkgdparvalues));
    fitpeak.setPropertyValue("FitWindow", "0.586, 0.604");
    fitpeak.setPropertyValue("PeakRange", "0.591, 0.597");
    fitpeak.setProperty("FitBackgroundFirst", true);
    fitpeak.setProperty("RawParams", true);
    fitpeak.setProperty("MinGuessedPeakWidth", 2);
    fitpeak.setProperty("MaxGuessedPeakWidth", 20);
    fitpeak.setProperty("GuessedPeakWidthStep", 2);
    // fitpeak.setProperty("OutputFitFunctionOnly", false);

    // Execute
    fitpeak.execute();
    TS_ASSERT(fitpeak.isExecuted());

    // Check
    vector<double> fittedpeakvalues =
        fitpeak.getProperty("FittedPeakParameterValues");
    TS_ASSERT_EQUALS(fittedpeakvalues.size(), 3);

    double peakheight = fittedpeakvalues[0];
    double peakcentre = fittedpeakvalues[1];
    double sigma = fittedpeakvalues[2];
    TS_ASSERT_DELTA(peakheight, 1170., 50.);
    TS_ASSERT_DELTA(peakcentre, 0.5945, 0.001);
    TS_ASSERT_DELTA(sigma, 0.00057, 0.0002);

    vector<double> fittedbkgdvalues =
        fitpeak.getProperty("FittedBackgroundParameterValues");
    TS_ASSERT_EQUALS(fittedbkgdvalues.size(), 3);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_PeakParameters(vector<string> &parnames, vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("Height");
    parvalues.push_back(1.0);

    parnames.emplace_back("PeakCentre");
    parvalues.push_back(0.5936);

    parnames.emplace_back("Sigma");
    parvalues.push_back(0.01);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_BkgdParameters(vector<string> &parnames, vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("A0");
    parvalues.push_back(1000.);

    parnames.emplace_back("A1");
    parvalues.push_back(-10.);

    parnames.emplace_back("A2");
    parvalues.push_back(0.01);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  MatrixWorkspace_sptr gen_4866P5Data() {

    const size_t size = 84;

    std::array<double, size> vecx = {
        {0.585120, 0.585354, 0.585588, 0.585822, 0.586057, 0.586291, 0.586526,
         0.586760, 0.586995, 0.587230, 0.587465, 0.587700, 0.587935, 0.588170,
         0.588405, 0.588641, 0.588876, 0.589112, 0.589347, 0.589583, 0.589819,
         0.590055, 0.590291, 0.590527, 0.590763, 0.590999, 0.591236, 0.591472,
         0.591709, 0.591946, 0.592182, 0.592419, 0.592656, 0.592893, 0.593130,
         0.593368, 0.593605, 0.593842, 0.594080, 0.594318, 0.594555, 0.594793,
         0.595031, 0.595269, 0.595507, 0.595745, 0.595984, 0.596222, 0.596461,
         0.596699, 0.596938, 0.597177, 0.597415, 0.597654, 0.597893, 0.598133,
         0.598372, 0.598611, 0.598851, 0.599090, 0.599330, 0.599570, 0.599809,
         0.600049, 0.600289, 0.600529, 0.600770, 0.601010, 0.601250, 0.601491,
         0.601731, 0.601972, 0.602213, 0.602454, 0.602695, 0.602936, 0.603177,
         0.603418, 0.603660, 0.603901, 0.604143, 0.604384, 0.604626, 0.604868}};
    std::array<double, size> vecy = {
        {15917.0, 16048.0, 16098.0, 15855.0, 15822.0, 15891.0, 15772.0, 15951.0,
         15860.0, 15813.0, 15742.0, 15733.0, 15594.0, 15644.0, 15850.0, 15623.0,
         15552.0, 15586.0, 15524.0, 15257.0, 15718.0, 15427.0, 15651.0, 15500.0,
         15611.0, 15508.0, 15230.0, 15111.0, 15483.0, 15316.0, 15256.0, 15152.0,
         15212.0, 15282.0, 15390.0, 15176.0, 15374.0, 15499.0, 16064.0, 16324.0,
         16240.0, 15972.0, 15770.0, 15449.0, 15644.0, 14972.0, 15146.0, 14799.0,
         15151.0, 14883.0, 14878.0, 14891.0, 14782.0, 14746.0, 15020.0, 14721.0,
         14813.0, 14744.0, 14786.0, 14783.0, 14876.0, 14776.0, 14729.0, 14806.0,
         14801.0, 14344.0, 14675.0, 14762.0, 14589.0, 14561.0, 14742.0, 14682.0,
         14634.0, 14542.0, 14758.0, 14667.0, 14586.0, 14729.0, 14581.0, 14445.0,
         14408.0, 14569.0, 14659.0, 14500.0}};
    std::array<double, size> vece = {
        {126.163000, 126.681000, 126.878000, 125.917000, 125.786000, 126.060000,
         125.587000, 126.297000, 125.936000, 125.750000, 125.467000, 125.431000,
         124.876000, 125.076000, 125.897000, 124.992000, 124.708000, 124.844000,
         124.595000, 123.519000, 125.371000, 124.205000, 125.104000, 124.499000,
         124.944000, 124.531000, 123.410000, 122.927000, 124.431000, 123.758000,
         123.515000, 123.093000, 123.337000, 123.620000, 124.056000, 123.191000,
         123.992000, 124.495000, 126.744000, 127.765000, 127.436000, 126.380000,
         125.579000, 124.294000, 125.076000, 122.360000, 123.069000, 121.651000,
         123.089000, 121.996000, 121.975000, 122.029000, 121.581000, 121.433000,
         122.556000, 121.330000, 121.709000, 121.425000, 121.598000, 121.585000,
         121.967000, 121.557000, 121.363000, 121.680000, 121.659000, 119.766000,
         121.140000, 121.499000, 120.785000, 120.669000, 121.417000, 121.169000,
         120.971000, 120.590000, 121.483000, 121.107000, 120.773000, 121.363000,
         120.752000, 120.187000, 120.033000, 120.702000, 121.074000,
         120.416000}};

    const size_t NVectors = 1;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", NVectors, size,
                                            size));

    MantidVec &vecX = ws->dataX(0);
    MantidVec &vecY = ws->dataY(0);
    MantidVec &vecE = ws->dataE(0);

    for (size_t i = 0; i < size; ++i) {
      vecX[i] = vecx[i];
      vecY[i] = vecy[i];
      vecE[i] = vece[i];
    }

    return ws;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on fit a peak with 1 step
    */
  void test_FitPeakOneStep() {
    // Generate input workspace
    MatrixWorkspace_sptr dataws = gen_PG3DiamondData();
    AnalysisDataService::Instance().addOrReplace("PG3_Si_Peak", dataws);

    // Initialize FitPeak
    FitPeak fitpeak;
    fitpeak.initialize();

    // Set up properties
    fitpeak.setPropertyValue("InputWorkspace", "PG3_Si_Peak");
    fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak2");
    fitpeak.setPropertyValue("ParameterTableWorkspace", "Fitted_Si_Parameters");
    fitpeak.setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterNames", "Height, PeakCentre, Sigma"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterValues", "40.0, 2.0658, 0.001"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterNames", "A0, A1"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterValues", "0.5, 0.0"));
    fitpeak.setPropertyValue("FitWindow", "2.051, 2.077");
    fitpeak.setPropertyValue("PeakRange", "2.055, 2.08");
    fitpeak.setProperty("FitBackgroundFirst", false);
    fitpeak.setProperty("RawParams", true);

    // Execute
    fitpeak.execute();
    TS_ASSERT(fitpeak.isExecuted());

    // Check
    vector<double> fittedpeakvalues =
        fitpeak.getProperty("FittedPeakParameterValues");
    TS_ASSERT_EQUALS(fittedpeakvalues.size(), 3);

    vector<double> fittedbkgdvalues =
        fitpeak.getProperty("FittedBackgroundParameterValues");
    TS_ASSERT_EQUALS(fittedbkgdvalues.size(), 2);

    double peakheight = fittedpeakvalues[0];
    TS_ASSERT_DELTA(peakheight, 26., 1.);

    // Clean
    AnalysisDataService::Instance().remove("PG3_Si_Peak");
    AnalysisDataService::Instance().remove("FittedPeak2");
    AnalysisDataService::Instance().remove("Fitted_Si_Parameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on fit a peak with 1 step
    */
  void test_FitPeakOneStepFullPeakName() {
    // Generate input workspace
    MatrixWorkspace_sptr dataws = gen_PG3DiamondData();
    AnalysisDataService::Instance().addOrReplace("PG3_Si_Peak", dataws);

    // Initialize FitPeak
    FitPeak fitpeak;
    fitpeak.initialize();

    // Set up properties
    fitpeak.setPropertyValue("InputWorkspace", "PG3_Si_Peak");
    fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak2");
    fitpeak.setPropertyValue("ParameterTableWorkspace", "Fitted_Si_Parameters");
    fitpeak.setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty(
        "PeakFunctionType", "Gaussian (Height, PeakCentre, Sigma)"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("PeakParameterValues", "40.0, 2.0658, 0.001"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundType", "Linear (A0, A1)"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeak.setProperty("BackgroundParameterValues", "0.5, 0.0"));
    fitpeak.setPropertyValue("FitWindow", "2.051, 2.077");
    fitpeak.setPropertyValue("PeakRange", "2.055, 2.08");
    fitpeak.setProperty("FitBackgroundFirst", false);
    fitpeak.setProperty("RawParams", true);

    // Execute
    fitpeak.execute();
    TS_ASSERT(fitpeak.isExecuted());

    // Check
    vector<double> fittedpeakvalues =
        fitpeak.getProperty("FittedPeakParameterValues");
    TS_ASSERT_EQUALS(fittedpeakvalues.size(), 3);

    vector<double> fittedbkgdvalues =
        fitpeak.getProperty("FittedBackgroundParameterValues");
    TS_ASSERT_EQUALS(fittedbkgdvalues.size(), 2);

    double peakheight = fittedpeakvalues[0];
    TS_ASSERT_DELTA(peakheight, 26., 1.);

    // Clean
    AnalysisDataService::Instance().remove("PG3_Si_Peak");
    AnalysisDataService::Instance().remove("FittedPeak2");
    AnalysisDataService::Instance().remove("Fitted_Si_Parameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  MatrixWorkspace_sptr gen_PG3DiamondData() {
    vector<double> vecx, vecy, vece;

    vecx.push_back(2.050678);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.051498);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.052319);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.053140);
    vecy.push_back(2.000000);
    vece.push_back(1.41421E+00);
    vecx.push_back(2.053961);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.054783);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.055605);
    vecy.push_back(2.000000);
    vece.push_back(1.41421E+00);
    vecx.push_back(2.056427);
    vecy.push_back(2.000000);
    vece.push_back(1.41421E+00);
    vecx.push_back(2.057250);
    vecy.push_back(3.000000);
    vece.push_back(1.73205E+00);
    vecx.push_back(2.058072);
    vecy.push_back(4.000000);
    vece.push_back(2.00000E+00);
    vecx.push_back(2.058896);
    vecy.push_back(5.000000);
    vece.push_back(2.23607E+00);
    vecx.push_back(2.059719);
    vecy.push_back(16.000000);
    vece.push_back(4.00000E+00);
    vecx.push_back(2.060543);
    vecy.push_back(20.000000);
    vece.push_back(4.47214E+00);
    vecx.push_back(2.061367);
    vecy.push_back(31.000000);
    vece.push_back(5.56776E+00);
    vecx.push_back(2.062192);
    vecy.push_back(26.000000);
    vece.push_back(5.09902E+00);
    vecx.push_back(2.063017);
    vecy.push_back(28.000000);
    vece.push_back(5.29150E+00);
    vecx.push_back(2.063842);
    vecy.push_back(29.000000);
    vece.push_back(5.38516E+00);
    vecx.push_back(2.064668);
    vecy.push_back(41.000000);
    vece.push_back(6.40312E+00);
    vecx.push_back(2.065493);
    vecy.push_back(40.000000);
    vece.push_back(6.32456E+00);
    vecx.push_back(2.066320);
    vecy.push_back(38.000000);
    vece.push_back(6.16441E+00);
    vecx.push_back(2.067146);
    vecy.push_back(40.000000);
    vece.push_back(6.32456E+00);
    vecx.push_back(2.067973);
    vecy.push_back(34.000000);
    vece.push_back(5.83095E+00);
    vecx.push_back(2.068800);
    vecy.push_back(35.000000);
    vece.push_back(5.91608E+00);
    vecx.push_back(2.069628);
    vecy.push_back(18.000000);
    vece.push_back(4.24264E+00);
    vecx.push_back(2.070456);
    vecy.push_back(21.000000);
    vece.push_back(4.58258E+00);
    vecx.push_back(2.071284);
    vecy.push_back(9.000000);
    vece.push_back(3.00000E+00);
    vecx.push_back(2.072112);
    vecy.push_back(6.000000);
    vece.push_back(2.44949E+00);
    vecx.push_back(2.072941);
    vecy.push_back(6.000000);
    vece.push_back(2.44949E+00);
    vecx.push_back(2.073770);
    vecy.push_back(11.000000);
    vece.push_back(3.31662E+00);
    vecx.push_back(2.074600);
    vecy.push_back(10.000000);
    vece.push_back(3.16228E+00);
    vecx.push_back(2.075430);
    vecy.push_back(4.000000);
    vece.push_back(2.00000E+00);
    vecx.push_back(2.076260);
    vecy.push_back(7.000000);
    vece.push_back(2.64575E+00);
    vecx.push_back(2.077090);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.077921);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.078752);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.079584);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.080416);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.081248);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.082080);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.082913);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.083746);
    vecy.push_back(2.000000);
    vece.push_back(1.41421E+00);
    vecx.push_back(2.084580);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.085414);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.086248);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.087082);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.087917);
    vecy.push_back(1.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.088752);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.089588);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.090424);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.091260);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.092096);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.092933);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);
    vecx.push_back(2.093770);
    vecy.push_back(0.000000);
    vece.push_back(1.00000E+00);

    size_t NVectors = 1;
    size_t sizex = vecx.size();
    size_t sizey = vecy.size();
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", NVectors, sizex,
                                            sizey));

    MantidVec &vecX = ws->dataX(0);
    MantidVec &vecY = ws->dataY(0);
    MantidVec &vecE = ws->dataE(0);

    for (size_t i = 0; i < sizex; ++i)
      vecX[i] = vecx[i];
    for (size_t i = 0; i < sizey; ++i) {
      vecY[i] = vecy[i];
      vecE[i] = vece[i];
    }

    return ws;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_linearBkgdParameters(vector<string> &parnames,
                                vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("A0");
    parvalues.push_back(48000.);

    parnames.emplace_back("A1");
    parvalues.push_back(-60010.);

    return;
  }
};

#endif /* MANTID_ALGORITHMS_FITPEAKTEST_H_ */
