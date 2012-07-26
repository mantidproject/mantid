#ifndef MANTID_CURVEFITTING_LEBAILFIT2TEST_H_
#define MANTID_CURVEFITTING_LEBAILFIT2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>

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
   * Fundamental test to calcualte 2 peak w/o background.
   * It is migrated from LeBailFunctionTest.test_CalculatePeakParameters
   */
  void test_cal2Peaks()
  {
      // 1. Create input workspace
      API::MatrixWorkspace_sptr dataws;
      DataObjects::TableWorkspace_sptr parameterws;
      DataObjects::TableWorkspace_sptr hklws;

      dataws = createInputDataWorkspace(1);
      parameterws = createPeakParameterWorkspace();
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
      hklws = createReflectionWorkspace(hkls, peakheights);

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
      lbfit.setProperty("Function", "Calculation");
      lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");

      // 4. Execute
      TS_ASSERT_THROWS_NOTHING(lbfit.execute());

      TS_ASSERT(lbfit.isExecuted());

      // 5. Get output
      DataObjects::Workspace2D_sptr outws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
                  AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
      TS_ASSERT(outws);

      /*
      for (size_t i = 0; i < outws->dataY(0).size(); ++i)
          std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << std::endl;
      */

      // 4. Calcualte data
      double y25 = 1360.27;
      double y59 = 0.285529;
      double y86 = 648.998;

      TS_ASSERT_DELTA(outws->readY(0)[25], y25, 0.01);
      TS_ASSERT_DELTA(outws->readY(0)[59], y59, 0.0001);
      TS_ASSERT_DELTA(outws->readY(0)[86], y86, 0.001);

      // 5. Clean
      AnalysisDataService::Instance().remove("Data");
      AnalysisDataService::Instance().remove("PeakParameters");
      AnalysisDataService::Instance().remove("Reflections");
      AnalysisDataService::Instance().remove("CalculatedPeaks");

  }

  /*
   * Unit test on figure out peak height
   * The test data are of reflection (932) and (852) @ TOF = 12721.91 and 12790.13
   */
  void test_calOverlappedPeakHeights()
  {
      // 1. Geneate data and Create input workspace
      API::MatrixWorkspace_sptr dataws;
      DataObjects::TableWorkspace_sptr parameterws;
      DataObjects::TableWorkspace_sptr hklws;

      //   Reflections
      int xp932[] = {9, 3, 2};
      std::vector<int> p932(xp932, xp932+sizeof(xp932)/sizeof(int));
      int xp852[] = {8, 5, 2};
      std::vector<int> p852(xp852, xp852+sizeof(xp852)/sizeof(int));
      std::vector<std::vector<int> > hkls;
      hkls.push_back(p932);
      hkls.push_back(p852);
      std::vector<double> pkheights(2, 1.0);

      dataws = createInputDataWorkspace(2);
      parameterws = createPeakParameterWorkspace();
      hklws = createReflectionWorkspace(hkls, pkheights);

      AnalysisDataService::Instance().addOrReplace("Data", dataws);
      AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
      AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

      // 2. Create LeBailFit and do the calculation
      LeBailFit2 lbfit;
      lbfit.initialize();

      // 3. Computation
      // 3. Set properties
      lbfit.setPropertyValue("InputWorkspace", "Data");
      lbfit.setPropertyValue("ParametersWorkspace", "PeakParameters");
      lbfit.setPropertyValue("ReflectionsWorkspace", "Reflections");
      lbfit.setProperty("WorkspaceIndex", 0);
      lbfit.setProperty("Function", "Calculation");
      lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");
      lbfit.setProperty("UseInputPeakHeights", false);

      TS_ASSERT_THROWS_NOTHING(lbfit.execute());
      TS_ASSERT(lbfit.isExecuted());

      // 4. Get result
      DataObjects::Workspace2D_sptr outputws =
              boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
      TS_ASSERT(outputws);
      if (!outputws)
      {
          return;
      }

      TS_ASSERT_EQUALS(outputws->getNumberHistograms(), 3);

      /*
      for (size_t ih = 0; ih < outputws->getNumberHistograms(); ++ih)
      {
          std::stringstream namestream;
          namestream << "calculated_pattern_";
          if (ih == 0)
          {
              namestream << "complete";
          }
          else
          {
              namestream << "peak_" << (ih-1);
          }
          namestream << ".dat";
          std::string filename = namestream.str();

          std::ofstream ofile;
          ofile.open(filename.c_str());
          std::cout << "Write to file " << filename << std::endl;
          for (size_t i = 0; i < outputws->dataY(ih).size(); ++i)
              ofile << outputws->dataX(ih)[i] << "\t\t" << outputws->dataY(ih)[i] << std::endl;
          ofile.close();
      }
      */


      /*
      std::cout << "Calculate Peak Heights. " << std::endl;
      std::vector<std::pair<int, double> > peakheights;
      lbfit.calcualtePeakHeights(peakheights, 0);


      std::cout << "Calculate Pattern. " << std::endl;
      int xp932[] = {9, 3, 2};
      std::vector<int> p932(xp932, xp932+sizeof(xp932)/sizeof(int));
      int xp852[] = {8, 5, 2};
      std::vector<int> p852(xp852, xp852+sizeof(xp852)/sizeof(int));
      std::vector<std::vector<int> > hkls;
      hkls.push_back(p932);
      hkls.push_back(p852);

      std::sort(peakheights.begin(), peakheights.end());
      std::vector<double> pkheights;
      for (size_t i = peakheights.size()-1; i >= 0; --i)
      {
          pkheights.push_back(peakheights[i].second);
      }
      hklws = createReflectionWorkspace(hkls, pkheights);
      */

      // 3. Do the calcualtion ...

      // 4. Check
      // a) peak position (very small deviation)

      // b) peak height (can be some percent off)

      // c) integrated value

      // -1. Clean
      AnalysisDataService::Instance().remove("Data");
      AnalysisDataService::Instance().remove("PeakParameters");
      AnalysisDataService::Instance().remove("Reflections");

      return;
  }

  /*
   * This is an advanced test based on test_calOverlappedPeakHeights
   * It will be REMOVED from unit test before commit
   */
  void Disabled_test_calOverlappedPeakHeights2()
  {
      // FIXME : Disable this unit test before committing

      // 1. Geneate data and Create input workspace
      API::MatrixWorkspace_sptr dataws;
      DataObjects::TableWorkspace_sptr parameterws;
      DataObjects::TableWorkspace_sptr hklws;

      // a)  Reflections
      std::vector<std::vector<int> > hkls;
      importReflectionTxtFile("/home/wzz/Mantid/Code/debug/unittest_multigroups_reflection.txt", hkls);
      size_t numpeaks = hkls.size();
      std::cout << "TestXXX Number of peaks = " << hkls.size() << std::endl;

      // b) data
      dataws = createInputDataWorkspace(3);

      // c) Workspaces
      std::vector<double> pkheights(numpeaks, 1.0);
      parameterws = createPeakParameterWorkspace();
      hklws = createReflectionWorkspace(hkls, pkheights);

      AnalysisDataService::Instance().addOrReplace("Data", dataws);
      AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
      AnalysisDataService::Instance().addOrReplace("Reflections", hklws);

      // 2. Create LeBailFit and do the calculation
      LeBailFit2 lbfit;
      lbfit.initialize();

      // 3. Computation
      // 3. Set properties
      lbfit.setPropertyValue("InputWorkspace", "Data");
      lbfit.setPropertyValue("ParametersWorkspace", "PeakParameters");
      lbfit.setPropertyValue("ReflectionsWorkspace", "Reflections");
      lbfit.setProperty("WorkspaceIndex", 0);
      lbfit.setProperty("Function", "Calculation");
      lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");
      lbfit.setProperty("UseInputPeakHeights", false);

      TS_ASSERT_THROWS_NOTHING(lbfit.execute());
      TS_ASSERT(lbfit.isExecuted());

      // 4. Get result
      DataObjects::Workspace2D_sptr outputws =
              boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
      TS_ASSERT(outputws);
      if (!outputws)
      {
          return;
      }

      TS_ASSERT_EQUALS(outputws->getNumberHistograms(), 3);

      for (size_t ih = 0; ih < outputws->getNumberHistograms(); ++ih)
      {
          std::stringstream namestream;
          namestream << "calculated_pattern_";
          if (ih == 0)
          {
              namestream << "complete";
          }
          else
          {
              namestream << "peak_" << (ih-1);
          }
          namestream << ".dat";
          std::string filename = namestream.str();

          std::ofstream ofile;
          ofile.open(filename.c_str());
          std::cout << "Write to file " << filename << std::endl;
          for (size_t i = 0; i < outputws->dataY(ih).size(); ++i)
              ofile << outputws->dataX(ih)[i] << "\t\t" << outputws->dataY(ih)[i] << std::endl;
          ofile.close();
      }


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
  DataObjects::TableWorkspace_sptr createReflectionWorkspace(std::vector<std::vector<int> > hkls, std::vector<double> heights)
  {
      // 0. Check
      if (hkls.size() != heights.size())
      {
          std::cout << "createReflectionWorkspace: input two vectors have different sizes.  It is not supported." << std::endl;
          throw std::invalid_argument("Vectors for HKL and heights are of different sizes.");
      }

      // 1. Crate table workspace
      DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
      DataObjects::TableWorkspace_sptr hklws = DataObjects::TableWorkspace_sptr(tablews);

      tablews->addColumn("int", "H");
      tablews->addColumn("int", "K");
      tablews->addColumn("int", "L");
      tablews->addColumn("double", "height");

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

  /*
   * Create data workspace without background
   */
  API::MatrixWorkspace_sptr createInputDataWorkspace(int option)
  {
    // 1. Import data
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    /*
    std::string filename("/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat");
    importDataFromColumnFile(filename, vecX, vecY,  vecE);
    */

    switch (option)
    {
    case 1:
        generateData(vecX, vecY, vecE);
        break;

    case 2:
        generateTwinPeakData(vecX, vecY, vecE);
        break;

    case 3:
        importDataFromColumnFile("/home/wzz/Mantid/Code/debug/unittest_multigroups.dat", vecX, vecY, vecE);
        break;

    default:
        // not supported
        std::cout << "LeBailFitTest.createInputDataWorkspace() Option " << option << "is not supported. " << std::endl;
        throw std::invalid_argument("Unsupported option. ");
    }


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

  /*
   * Generate data (vectors) containg twin peak w/o background
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

  /*
   * Import text file containing reflection (HKL)
   */
  void importReflectionTxtFile(std::string filename, std::vector<std::vector<int> >& hkls)
  {
      std::ifstream ins;
      ins.open(filename.c_str());
      if (!ins.is_open())
      {
          std::cout << "File " << filename << " cannot be opened. " << std::endl;
          throw std::invalid_argument("Cannot open Reflection-Text-File.");
      }
      else
      {
          std::cout << "File " << filename << " is opened." << std::endl;
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

      return;
  }

  /*
   * Import data from a column data file
   */
  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    std::ifstream ins;
    ins.open(filename.c_str());
    char line[256];
    // std::cout << "File " << filename << " isOpen = " << ins.is_open() << std::endl;
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        double x, y;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y;
        vecX.push_back(x);
        vecY.push_back(y);
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vecE.push_back(e);
      }
    }

    return;
  }
};


#endif /* MANTID_CURVEFITTING_LEBAILFIT2TEST_H_ */
