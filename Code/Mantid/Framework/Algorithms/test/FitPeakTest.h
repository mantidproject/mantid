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

class FitPeakTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeakTest *createSuite()
  {
    API::FrameworkManager::Instance();
    return new FitPeakTest();
  }
  static void destroySuite( FitPeakTest *suite ) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test on init and setup
    */
  void test_Init()
  {
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
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue("InputWorkspace", "PG3_4866Peak5"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue("ParameterTableWorkspace", "Fitted_Peak5_Parameters"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("WorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterNames", bkgdparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterValues", bkgdparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue("FitWindow", "10.0, 20.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setPropertyValue("PeakRange", "11.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("FitBackgroundFirst", true));

    // Clean
    AnalysisDataService::Instance().remove("PG3_4866Peak5");
    AnalysisDataService::Instance().remove("Peak5_Parameters");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Test on fit a peak with significantly high background
    */
  void test_FitPeakWithHighBkgd()
  {
    // Generate input workspace
    MatrixWorkspace_sptr dataws = gen_4866P5Data();
    AnalysisDataService::Instance().addOrReplace("PG3_4866Peak5", dataws);

    // Generate peak and background parameters
    vector<string> peakparnames, bkgdparnames;
    vector<double> peakparvalues, bkgdparvalues;

    gen_BkgdParameters(bkgdparnames, bkgdparvalues);
    gen_PeakParameters(peakparnames, peakparvalues);

#if 0
    const MantidVec& vecx = dataws->readX(0);
    const MantidVec& vecy = dataws->readY(0);
    for (size_t i = 0; i < vecx.size(); ++i)
      cout << vecx[i] << "\t\t" << vecy[i] << "\n";
    TS_ASSERT_EQUALS(1, 100);
    return;
#endif

    // Initialize FitPeak
    FitPeak fitpeak;
    fitpeak.initialize();

    // Set up properties
    fitpeak.setPropertyValue("InputWorkspace", "PG3_4866Peak5");
    fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak");
    fitpeak.setPropertyValue("ParameterTableWorkspace", "Fitted_Peak5_Parameters");
    fitpeak.setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterNames", bkgdparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterValues", bkgdparvalues));
    fitpeak.setPropertyValue("FitWindow", "0.586, 0.604");
    fitpeak.setPropertyValue("PeakRange", "0.591, 0.597");
    fitpeak.setProperty("FitBackgroundFirst", true);
    fitpeak.setProperty("RawParams", true);
    fitpeak.setProperty("MinGuessedPeakWidth", 2);
    fitpeak.setProperty("MaxGuessedPeakWidth", 20);
    fitpeak.setProperty("GuessedPeakWidthStep", 2);

    // Execute
    fitpeak.execute();
    TS_ASSERT(fitpeak.isExecuted());

    // Check
    vector<double> fittedpeakvalues = fitpeak.getProperty("FittedPeakParameterValues");
    TS_ASSERT_EQUALS(fittedpeakvalues.size(), 3);

    double peakheight = fittedpeakvalues[0];
    double peakcentre = fittedpeakvalues[1];
    double sigma = fittedpeakvalues[2];
    TS_ASSERT_DELTA(peakheight, 1170., 50.);
    TS_ASSERT_DELTA(peakcentre, 0.5945, 0.001);
    TS_ASSERT_DELTA(sigma, 0.00057, 0.0002);

    vector<double> fittedbkgdvalues = fitpeak.getProperty("FittedBackgroundParameterValues");
    TS_ASSERT_EQUALS(fittedbkgdvalues.size(), 3);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_PeakParameters(vector<string>& parnames, vector<double>& parvalues)
  {
    parnames.clear();
    parvalues.clear();

    parnames.push_back("Height");
    parvalues.push_back(1.0);

    parnames.push_back("PeakCentre");
    parvalues.push_back(0.5936);

    parnames.push_back("Sigma");
    parvalues.push_back(0.01);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_BkgdParameters(vector<string>& parnames, vector<double>& parvalues)
  {
    parnames.clear();
    parvalues.clear();

    parnames.push_back("A0");
    parvalues.push_back(1000.);

    parnames.push_back("A1");
    parvalues.push_back(-10.);

    parnames.push_back("A2");
    parvalues.push_back(0.01);

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  MatrixWorkspace_sptr gen_4866P5Data()
  {
    vector<double> vecx, vecy, vece;
    vecx.push_back(0.58512); vecy.push_back(15917); vece.push_back(126.163);
    vecx.push_back(0.585354); vecy.push_back(16048); vece.push_back(126.681);
    vecx.push_back(0.585588); vecy.push_back(16098); vece.push_back(126.878);
    vecx.push_back(0.585822); vecy.push_back(15855); vece.push_back(125.917);
    vecx.push_back(0.586057); vecy.push_back(15822); vece.push_back(125.786);
    vecx.push_back(0.586291); vecy.push_back(15891); vece.push_back(126.06);
    vecx.push_back(0.586526); vecy.push_back(15772); vece.push_back(125.587);
    vecx.push_back(0.58676); vecy.push_back(15951); vece.push_back(126.297);
    vecx.push_back(0.586995); vecy.push_back(15860); vece.push_back(125.936);
    vecx.push_back(0.58723); vecy.push_back(15813); vece.push_back(125.75);
    vecx.push_back(0.587465); vecy.push_back(15742); vece.push_back(125.467);
    vecx.push_back(0.5877); vecy.push_back(15733); vece.push_back(125.431);
    vecx.push_back(0.587935); vecy.push_back(15594); vece.push_back(124.876);
    vecx.push_back(0.58817); vecy.push_back(15644); vece.push_back(125.076);
    vecx.push_back(0.588405); vecy.push_back(15850); vece.push_back(125.897);
    vecx.push_back(0.588641); vecy.push_back(15623); vece.push_back(124.992);
    vecx.push_back(0.588876); vecy.push_back(15552); vece.push_back(124.708);
    vecx.push_back(0.589112); vecy.push_back(15586); vece.push_back(124.844);
    vecx.push_back(0.589347); vecy.push_back(15524); vece.push_back(124.595);
    vecx.push_back(0.589583); vecy.push_back(15257); vece.push_back(123.519);
    vecx.push_back(0.589819); vecy.push_back(15718); vece.push_back(125.371);
    vecx.push_back(0.590055); vecy.push_back(15427); vece.push_back(124.205);
    vecx.push_back(0.590291); vecy.push_back(15651); vece.push_back(125.104);
    vecx.push_back(0.590527); vecy.push_back(15500); vece.push_back(124.499);
    vecx.push_back(0.590763); vecy.push_back(15611); vece.push_back(124.944);
    vecx.push_back(0.590999); vecy.push_back(15508); vece.push_back(124.531);
    vecx.push_back(0.591236); vecy.push_back(15230); vece.push_back(123.41);
    vecx.push_back(0.591472); vecy.push_back(15111); vece.push_back(122.927);
    vecx.push_back(0.591709); vecy.push_back(15483); vece.push_back(124.431);
    vecx.push_back(0.591946); vecy.push_back(15316); vece.push_back(123.758);
    vecx.push_back(0.592182); vecy.push_back(15256); vece.push_back(123.515);
    vecx.push_back(0.592419); vecy.push_back(15152); vece.push_back(123.093);
    vecx.push_back(0.592656); vecy.push_back(15212); vece.push_back(123.337);
    vecx.push_back(0.592893); vecy.push_back(15282); vece.push_back(123.62);
    vecx.push_back(0.59313); vecy.push_back(15390); vece.push_back(124.056);
    vecx.push_back(0.593368); vecy.push_back(15176); vece.push_back(123.191);
    vecx.push_back(0.593605); vecy.push_back(15374); vece.push_back(123.992);
    vecx.push_back(0.593842); vecy.push_back(15499); vece.push_back(124.495);
    vecx.push_back(0.59408); vecy.push_back(16064); vece.push_back(126.744);
    vecx.push_back(0.594318); vecy.push_back(16324); vece.push_back(127.765);
    vecx.push_back(0.594555); vecy.push_back(16240); vece.push_back(127.436);
    vecx.push_back(0.594793); vecy.push_back(15972); vece.push_back(126.38);
    vecx.push_back(0.595031); vecy.push_back(15770); vece.push_back(125.579);
    vecx.push_back(0.595269); vecy.push_back(15449); vece.push_back(124.294);
    vecx.push_back(0.595507); vecy.push_back(15644); vece.push_back(125.076);
    vecx.push_back(0.595745); vecy.push_back(14972); vece.push_back(122.36);
    vecx.push_back(0.595984); vecy.push_back(15146); vece.push_back(123.069);
    vecx.push_back(0.596222); vecy.push_back(14799); vece.push_back(121.651);
    vecx.push_back(0.596461); vecy.push_back(15151); vece.push_back(123.089);
    vecx.push_back(0.596699); vecy.push_back(14883); vece.push_back(121.996);
    vecx.push_back(0.596938); vecy.push_back(14878); vece.push_back(121.975);
    vecx.push_back(0.597177); vecy.push_back(14891); vece.push_back(122.029);
    vecx.push_back(0.597415); vecy.push_back(14782); vece.push_back(121.581);
    vecx.push_back(0.597654); vecy.push_back(14746); vece.push_back(121.433);
    vecx.push_back(0.597893); vecy.push_back(15020); vece.push_back(122.556);
    vecx.push_back(0.598133); vecy.push_back(14721); vece.push_back(121.33);
    vecx.push_back(0.598372); vecy.push_back(14813); vece.push_back(121.709);
    vecx.push_back(0.598611); vecy.push_back(14744); vece.push_back(121.425);
    vecx.push_back(0.598851); vecy.push_back(14786); vece.push_back(121.598);
    vecx.push_back(0.59909); vecy.push_back(14783); vece.push_back(121.585);
    vecx.push_back(0.59933); vecy.push_back(14876); vece.push_back(121.967);
    vecx.push_back(0.59957); vecy.push_back(14776); vece.push_back(121.557);
    vecx.push_back(0.599809); vecy.push_back(14729); vece.push_back(121.363);
    vecx.push_back(0.600049); vecy.push_back(14806); vece.push_back(121.68);
    vecx.push_back(0.600289); vecy.push_back(14801); vece.push_back(121.659);
    vecx.push_back(0.600529); vecy.push_back(14344); vece.push_back(119.766);
    vecx.push_back(0.60077); vecy.push_back(14675); vece.push_back(121.14);
    vecx.push_back(0.60101); vecy.push_back(14762); vece.push_back(121.499);
    vecx.push_back(0.60125); vecy.push_back(14589); vece.push_back(120.785);
    vecx.push_back(0.601491); vecy.push_back(14561); vece.push_back(120.669);
    vecx.push_back(0.601731); vecy.push_back(14742); vece.push_back(121.417);
    vecx.push_back(0.601972); vecy.push_back(14682); vece.push_back(121.169);
    vecx.push_back(0.602213); vecy.push_back(14634); vece.push_back(120.971);
    vecx.push_back(0.602454); vecy.push_back(14542); vece.push_back(120.59);
    vecx.push_back(0.602695); vecy.push_back(14758); vece.push_back(121.483);
    vecx.push_back(0.602936); vecy.push_back(14667); vece.push_back(121.107);
    vecx.push_back(0.603177); vecy.push_back(14586); vece.push_back(120.773);
    vecx.push_back(0.603418); vecy.push_back(14729); vece.push_back(121.363);
    vecx.push_back(0.60366); vecy.push_back(14581); vece.push_back(120.752);
    vecx.push_back(0.603901); vecy.push_back(14445); vece.push_back(120.187);
    vecx.push_back(0.604143); vecy.push_back(14408); vece.push_back(120.033);
    vecx.push_back(0.604384); vecy.push_back(14569); vece.push_back(120.702);
    vecx.push_back(0.604626); vecy.push_back(14659); vece.push_back(121.074);
    vecx.push_back(0.604868); vecy.push_back(14500); vece.push_back(120.416);


    size_t NVectors = 1;
    size_t sizex = vecx.size();
    size_t sizey = vecy.size();
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
            WorkspaceFactory::Instance().create("Workspace2D", NVectors, sizex, sizey));

    MantidVec& vecX = ws->dataX(0);
    MantidVec& vecY = ws->dataY(0);
    MantidVec& vecE = ws->dataE(0);

    for (size_t i = 0; i < sizex; ++i)
        vecX[i] = vecx[i];
    for (size_t i = 0; i < sizey; ++i)
    {
        vecY[i] = vecy[i];
        vecE[i] = vece[i];
    }

    return ws;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on fit a peak with 1 step
    */
  void Xtest_FitPeakOneStep()
  {
    // Generate input workspace
    // FIXME - Find a new set of data!
    MatrixWorkspace_sptr dataws = gen_4866P5Data();
    AnalysisDataService::Instance().addOrReplace("PG3_4866Peak5", dataws);

    // Generate peak and background parameters
    vector<string> peakparnames, bkgdparnames;
    vector<double> peakparvalues, bkgdparvalues;

    gen_linearBkgdParameters(bkgdparnames, bkgdparvalues);
    gen_PeakParameters(peakparnames, peakparvalues);

#if 0
    const MantidVec& vecx = dataws->readX(0);
    const MantidVec& vecy = dataws->readY(0);
    for (size_t i = 0; i < vecx.size(); ++i)
      cout << vecx[i] << "\t\t" << vecy[i] << "\n";
    TS_ASSERT_EQUALS(1, 100);
    return;
#endif

    // Initialize FitPeak
    FitPeak fitpeak;
    fitpeak.initialize();

    // Set up properties
    fitpeak.setPropertyValue("InputWorkspace", "PG3_4866Peak5");
    fitpeak.setPropertyValue("OutputWorkspace", "FittedPeak");
    fitpeak.setPropertyValue("ParameterTableWorkspace", "Fitted_Peak5_Parameters");
    fitpeak.setProperty("WorkspaceIndex", 0);
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakFunctionType", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterNames", bkgdparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeak.setProperty("BackgroundParameterValues", bkgdparvalues));
    fitpeak.setPropertyValue("FitWindow", "0.586, 0.604");
    fitpeak.setPropertyValue("PeakRange", "0.591, 0.597");
    fitpeak.setProperty("FitBackgroundFirst", false);
    fitpeak.setProperty("RawParams", true);

    // Execute
    fitpeak.execute();
    TS_ASSERT(fitpeak.isExecuted());

    // Check
    vector<double> fittedpeakvalues = fitpeak.getProperty("FittedPeakParameterValues");
    TS_ASSERT_EQUALS(fittedpeakvalues.size(), 3);

    vector<double> fittedbkgdvalues = fitpeak.getProperty("FittedBackgroundParameterValues");
    TS_ASSERT_EQUALS(fittedbkgdvalues.size(), 2);

    double peakheight = fittedpeakvalues[0];
    TS_ASSERT_DELTA(peakheight, 1200., 200.);


    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  void gen_linearBkgdParameters(vector<string>& parnames, vector<double>& parvalues)
  {
    parnames.clear();
    parvalues.clear();

    parnames.push_back("A0");
    parvalues.push_back(48000.);

    parnames.push_back("A1");
    parvalues.push_back(-60010.);

    return;
  }


};


#endif /* MANTID_ALGORITHMS_FITPEAKTEST_H_ */
