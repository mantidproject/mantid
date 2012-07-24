#ifndef MANTID_CURVEFITTING_LEBAILFIT2TEST_H_
#define MANTID_CURVEFITTING_LEBAILFIT2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidCurveFitting/LeBailFit2.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

using Mantid::CurveFitting::LeBailFit2;

class LeBailFit2Test : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFit2Test *createSuite() { return new LeBailFit2Test(); }
  static void destroySuite( LeBailFit2Test *suite ) { delete suite; }

  /*
   * Fundamental test to calcualte 2 peak w/o background
   */
  void test_cal2Peaks()
  {
      // 1. Create input workspace
      API::MatrixWorkspace_sptr dataws;
      DataObjects::TableWorkspace_sptr parameterws;
      DataObjects::TableWorkspace_sptr hklws;

      dataws = createInputDataWorkspace();
      parameterws = createPeakParameterWorkspace();
      hklws = createReflectionWorkspace();

      AnalysisDataService::Instance().addOrReplace("Data", dataws);
      AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
      AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

      // 2. Initialize the algorithm
      LeBailFit2 lbfit;

      TS_ASSERT_THROWS_NOTHING(lbfit.initialize());
      TS_ASSERT(lbfit.isInitialized());

      // 3. Set properties
      lbfit.setPropertyValue("InputWorkspace", "Data");
      lbfit.setPropertyValue("ParametersWorkspace", "PeakParameters");
      lbfit.setPropertyValue("ReflectionsWorkspace", "Reflections");
      lbfit.setProperty("WorkspaceIndex", 0);
      lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");

      // 4. Execute
      TS_ASSERT_THROWS_NOTHING(lbfit.execute());

      TS_ASSERT(lbfit.isExecuted());
  }

  /*
   * Create parameter workspace for peak calculation
   */
  DataObjects::TableWorkspace_sptr createPeakParameterWorkspace()
  {
    DataObjects::TableWorkspace *tablews;

    tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr parameterws(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");

    API::TableRow newparam = parameterws->appendRow();
    newparam << "Dtt1"  << 29671.7500 << "t";
    newparam = parameterws->appendRow();
    newparam << "Dtt2"  << 0.0 << "t";
    newparam = parameterws->appendRow();
    newparam << "Dtt1t" << 29671.750 << "t";
    newparam = parameterws->appendRow();
    newparam << "Dtt2t" << 0.30 << "t";
    newparam = parameterws->appendRow();
    newparam << "Zero"  << 0.0 << "f";
    // newparam << "Zero"  << 50.0 << "f";
    newparam = parameterws->appendRow();
    newparam << "Zerot" << 33.70 << "t";
    newparam = parameterws->appendRow();
    newparam << "Alph0" << 4.026 << "t";
    newparam = parameterws->appendRow();
    newparam << "Alph1" << 7.362 << "t";
    newparam = parameterws->appendRow();
    newparam << "Beta0" << 3.489 << "t";
    newparam = parameterws->appendRow();
    newparam << "Beta1" << 19.535 << "t";
    newparam = parameterws->appendRow();
    newparam << "Alph0t"<< 60.683 << "t";
    newparam = parameterws->appendRow();
    newparam << "Alph1t"<< 39.730 << "t";
    newparam = parameterws->appendRow();
    newparam << "Beta0t"<< 96.864 << "t";
    newparam = parameterws->appendRow();
    newparam << "Beta1t"<< 96.864 << "t";
    newparam = parameterws->appendRow();
    newparam << "Sig2"  <<  11.380 << "t";
    newparam = parameterws->appendRow();
    newparam << "Sig1"  <<   9.901 << "t";
    newparam = parameterws->appendRow();
    newparam << "Sig0"  <<  17.370 << "t";
    newparam = parameterws->appendRow();
    newparam << "Width" << 1.0055 << "t";
    newparam = parameterws->appendRow();
    newparam << "Tcross"<< 0.4700 << "t";
    newparam = parameterws->appendRow();
    newparam << "Gam0"  << 0.0 << "t";
    newparam = parameterws->appendRow();
    newparam << "Gam1"  << 0.0 << "t";
    newparam = parameterws->appendRow();
    newparam << "Gam2"  << 0.0 << "t";
    newparam = parameterws->appendRow();
    newparam << "LatticeConstant" << 4.156890 << "t";

    return parameterws;
  }


  /*
   * Create reflection table workspaces
   */
  DataObjects::TableWorkspace_sptr createReflectionWorkspace()
  {
    DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr hklws = DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("int", "H");
    tablews->addColumn("int", "K");
    tablews->addColumn("int", "L");

    // a) Add reflection (111) and (110)
    API::TableRow hkl = hklws->appendRow();
    hkl << 1 << 1 << 0;
    hkl = hklws->appendRow();
    hkl << 1 << 1 << 1;

    return hklws;
  }

  /*
   * Create data workspace without background
   */
  API::MatrixWorkspace_sptr createInputDataWorkspace()
  {
    // 1. Import data
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    /*
    std::string filename("/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat");
    importDataFromColumnFile(filename, vecX, vecY,  vecE);
    */
    generateData(vecX, vecY, vecE);

    // 2. Get workspace
    int64_t nHist = 1;
    int64_t nBins = vecX.size();
    std::set<int64_t> maskedWSIndices;

    API::MatrixWorkspace_sptr dataws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            API::WorkspaceFactory::Instance().create("Workspace2D", nHist, nBins, nBins));


    // Mantid::DataObjects::Workspace2D_sptr  Create2DWorkspace(int nHist, int nBins);
    // API::MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(Create2DWorkspaceWithValues(
    // nHist, nBins, isHist, maskedWSIndices, xvalue, yvalue, evalue));

    // 3. Input data
    for (size_t i = 0; i < vecX.size(); ++i)
    {
      dataws->dataX(0)[i] = vecX[i];
      dataws->dataY(0)[i] = vecY[i];
      dataws->dataE(0)[i] = vecE[i];
    }

    return dataws;
  }

  /*
   * Generate a set of powder diffraction data with 2 peaks w/o background
   */
  void generateData(std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
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


};


#endif /* MANTID_CURVEFITTING_LEBAILFIT2TEST_H_ */
