// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/LeBailFit.h"
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Points.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

using namespace std;

using Mantid::CurveFitting::Algorithms::LeBailFit;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;

namespace {
//----------------------------------------------------------------------------------------------
/** Generate peak parameters for the data without background
 */
void genPeakParametersBackgroundLessData(std::map<std::string, double> &paramvaluemap) {
  // a) Value
  paramvaluemap.emplace("Dtt1", 29671.7500);
  paramvaluemap.emplace("Dtt2", 0.0);
  paramvaluemap.emplace("Dtt1t", 29671.750);
  paramvaluemap.emplace("Dtt2t", 0.30);
  paramvaluemap.emplace("Zero", 0.0);
  paramvaluemap.emplace("Zerot", 33.70);
  paramvaluemap.emplace("Alph0", 4.026);
  paramvaluemap.emplace("Alph1", 7.362);
  paramvaluemap.emplace("Beta0", 3.489);
  paramvaluemap.emplace("Beta1", 19.535);
  paramvaluemap.emplace("Alph0t", 60.683);
  paramvaluemap.emplace("Alph1t", 39.730);
  paramvaluemap.emplace("Beta0t", 96.864);
  paramvaluemap.emplace("Beta1t", 96.864);
  paramvaluemap.emplace("Sig2", sqrt(11.380));
  paramvaluemap.emplace("Sig1", sqrt(9.901));
  paramvaluemap.emplace("Sig0", sqrt(17.370));
  paramvaluemap.emplace("Width", 1.0055);
  paramvaluemap.emplace("Tcross", 0.4700);
  paramvaluemap.emplace("Gam0", 0.0);
  paramvaluemap.emplace("Gam1", 0.0);
  paramvaluemap.emplace("Gam2", 0.0);
  paramvaluemap.emplace("LatticeConstant", 4.156890);
  return;
}

//----------------------------------------------------------------------------------------------
/** Genearte peak parameters for data with background.  Bank 7
 */
void genPeakParameterBank7(std::map<std::string, double> &paramvaluemap) {
  paramvaluemap.clear();

  paramvaluemap.emplace("Alph0", 0.5);
  paramvaluemap.emplace("Alph0t", 128.96);
  paramvaluemap.emplace("Alph1", 0.);
  paramvaluemap.emplace("Alph1t", 15.702);
  paramvaluemap.emplace("Beta0", 2.0);
  paramvaluemap.emplace("Beta0t", 202.28);
  paramvaluemap.emplace("Beta1", 0.);
  paramvaluemap.emplace("Beta1t", 0.);
  paramvaluemap.emplace("CWL", 4.797);
  paramvaluemap.emplace("Dtt1", 22777.1);
  paramvaluemap.emplace("Dtt1t", 22785.4);
  paramvaluemap.emplace("Dtt2", 0.0);
  paramvaluemap.emplace("Dtt2t", 0.3);
  paramvaluemap.emplace("Gam0", 0);
  paramvaluemap.emplace("Gam1", 0);
  paramvaluemap.emplace("Gam2", 0);
  paramvaluemap.emplace("Profile", 10);
  paramvaluemap.emplace("Sig0", 0);
  paramvaluemap.emplace("Sig1", sqrt(10.0));
  paramvaluemap.emplace("Sig2", sqrt(15.48));
  paramvaluemap.emplace("Tcross", 0.25);
  paramvaluemap.emplace("Width", 5.8675);
  paramvaluemap.emplace("Zero", 0);
  paramvaluemap.emplace("Zerot", 62.5);
  paramvaluemap.emplace("step", 0.005);
  paramvaluemap.emplace("tof-max", 233.8);
  paramvaluemap.emplace("tof-min", 50.2919);
  paramvaluemap.emplace("twotheta", 90.807);
  paramvaluemap.emplace("LatticeConstant", 9.438);

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate peak parameters for NOMAD Bank4
 */
void genPeakParameterNomBank4(map<std::string, double> &paramvaluemap) {
  paramvaluemap.clear();

  paramvaluemap.emplace("Alph0", 0.886733);
  paramvaluemap.emplace("Alph0t", 114.12);
  paramvaluemap.emplace("Alph1", 8.38073);
  paramvaluemap.emplace("Alph1t", 75.8038);
  paramvaluemap.emplace("Beta0", 3.34888);
  paramvaluemap.emplace("Beta0t", 88.292);
  paramvaluemap.emplace("Beta1", 10.5768);
  paramvaluemap.emplace("Beta1t", -0.0346847);
  paramvaluemap.emplace("Dtt1", 9491.56);
  paramvaluemap.emplace("Dtt1t", 9423.85);
  paramvaluemap.emplace("Dtt2", 0);
  paramvaluemap.emplace("Dtt2t", 0.3);
  paramvaluemap.emplace("Gam0", 0);
  paramvaluemap.emplace("Gam1", 0);
  paramvaluemap.emplace("Gam2", 0);
  paramvaluemap.emplace("LatticeConstant", 4.15689);
  paramvaluemap.emplace("Sig0", 0);
  paramvaluemap.emplace("Sig1", 18.3863);
  paramvaluemap.emplace("Sig2", 0.671019);
  paramvaluemap.emplace("Tcross", 0.4373);
  paramvaluemap.emplace("Width", 2.9654);
  paramvaluemap.emplace("Zero", 0);
  paramvaluemap.emplace("Zerot", 101.618);

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate peak parameters for GPPD bank 1 from arg_si.pcr (Fullprof
 * example)
 */
void generateGPPDBank1(map<std::string, double> &parammap) {
  parammap.emplace("Dtt1", 16370.650);
  parammap.emplace("Dtt2", 0.10);
  parammap.emplace("Zero", 0.0);

  parammap.emplace("Alph0", 1.0);
  parammap.emplace("Alph1", 0.0);
  parammap.emplace("Beta0", 0.109036);
  parammap.emplace("Beta1", 0.009834);

  parammap.emplace("Sig2", sqrt(91.127));
  parammap.emplace("Sig1", sqrt(1119.230));
  parammap.emplace("Sig0", sqrt(0.0));

  parammap.emplace("Gam0", 0.0);
  parammap.emplace("Gam1", 7.688);
  parammap.emplace("Gam2", 0.0);

  parammap.emplace("LatticeConstant", 5.431363);

  return;
}

//----------------------------------------------------------------------------------------------
/** Create reflection table workspaces
 */
DataObjects::TableWorkspace_sptr createInputHKLWorkspace(std::vector<std::vector<int>> hkls,
                                                         std::vector<double> heights) {
  // 0. Check
  if (hkls.size() != heights.size()) {
    std::cout << "createInputHKLWorkspace: input two vectors have different "
                 "sizes.  It is not supported.\n";
    throw std::invalid_argument("Vectors for HKL and heights are of different sizes.");
  }

  // 1. Crate table workspace
  DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
  DataObjects::TableWorkspace_sptr hklws = DataObjects::TableWorkspace_sptr(tablews);

  tablews->addColumn("int", "H");
  tablews->addColumn("int", "K");
  tablews->addColumn("int", "L");
  tablews->addColumn("double", "PeakHeight");

  // 2. Add reflections and heights
  for (size_t ipk = 0; ipk < hkls.size(); ++ipk) {
    API::TableRow hkl = hklws->appendRow();
    for (size_t i = 0; i < 3; ++i) {
      hkl << hkls[ipk][i];
    }
    hkl << heights[ipk];
  }

  return hklws;
}

//----------------------------------------------------------------------------------------------
/** Generate a set of powder diffraction data with 2 peaks w/o background
 */
API::MatrixWorkspace_sptr generateSeparateTwoPeaksData2() {

  // a) Generate data
  Points vecX = {
      70931.750000, 70943.609000, 70955.477000, 70967.336000, 70979.203000, 70991.063000, 71002.930000, 71014.789000,
      71026.656000, 71038.516000, 71050.383000, 71062.242000, 71074.109000, 71085.969000, 71097.836000, 71109.695000,
      71121.563000, 71133.430000, 71145.289000, 71157.156000, 71169.016000, 71180.883000, 71192.742000, 71204.609000,
      71216.469000, 71228.336000, 71240.195000, 71252.063000, 71263.922000, 71275.789000, 71287.648000, 71299.516000,
      71311.375000, 71323.242000, 71335.102000, 71346.969000, 71358.836000, 71370.695000, 71382.563000, 71394.422000,
      71406.289000, 71418.148000, 71430.016000, 71441.875000, 71453.742000, 71465.602000, 71477.469000, 71489.328000,
      71501.195000, 71513.055000, 71524.922000, 71536.781000, 71548.648000, 71560.508000, 71572.375000, 71584.242000,
      71596.102000, 71607.969000, 71619.828000, 86911.852000, 86923.719000, 86935.578000, 86947.445000, 86959.305000,
      86971.172000, 86983.039000, 86994.898000, 87006.766000, 87018.625000, 87030.492000, 87042.352000, 87054.219000,
      87066.078000, 87077.945000, 87089.805000, 87101.672000, 87113.531000, 87125.398000, 87137.258000, 87149.125000,
      87160.984000, 87172.852000, 87184.711000, 87196.578000, 87208.445000, 87220.305000, 87232.172000, 87244.031000,
      87255.898000, 87267.758000, 87279.625000, 87291.484000, 87303.352000, 87315.211000, 87327.078000, 87338.938000,
      87350.805000, 87362.664000, 87374.531000, 87386.391000, 87398.258000, 87410.117000, 87421.984000, 87433.844000,
      87445.711000, 87457.578000, 87469.438000, 87481.305000, 87493.164000, 87505.031000, 87516.891000, 87528.758000,
      87540.617000, 87552.484000, 87564.344000, 87576.211000, 87588.070000, 87599.938000, 87611.797000, 87623.664000,
      87635.523000, 87647.391000, 87659.250000, 87671.117000, 87682.984000, 87694.844000, 87706.711000};
  Counts vecY = {0.000000,    0.000000,    0.695623,    0.990163,    1.409745,    2.006657,   2.856977,   4.066674,
                 5.789926,    8.241489,    11.733817,   16.702133,   23.779659,   33.848408,  48.191662,  68.596909,
                 97.664757,   139.048890,  197.908080,  281.608030,  399.650210,  562.426700, 773.341920, 1015.281300,
                 1238.361300, 1374.938000, 1380.517300, 1266.397800, 1086.214100, 894.758910, 723.461120, 581.045350,
                 465.935880,  373.453830,  299.358000,  239.927200,  192.294970,  154.141530, 123.540130, 99.028404,
                 79.368507,   63.620914,   50.990391,   40.873333,   32.758839,   26.259121,  21.045954,  16.870203,
                 13.520998,   10.838282,   8.686581,    6.963067,    5.580704,    4.473431,   3.585330,   2.873542,
                 2.303400,    1.846111,    0.000000,    0.286515,    0.391570,    0.535034,   0.731211,   0.999114,
                 1.365452,    1.866113,    2.549823,    3.484748,    4.761496,    6.507361,   8.891540,   12.151738,
                 16.603910,   22.691912,   31.005537,   42.372311,   57.886639,   79.062233,  107.820820, 146.586610,
                 197.830060,  263.461850,  343.089660,  432.578460,  522.641240,  600.013730, 651.222600, 667.177430,
                 646.900390,  597.388730,  530.125730,  456.838900,  386.052950,  322.584560, 267.962310, 222.048630,
                 183.800430,  152.111010,  125.858200,  104.147070,  86.170067,   71.304932,  58.996807,  48.819309,
                 40.392483,   33.420235,   27.654932,   22.881344,   18.934097,   15.665835,  12.963332,  10.725698,
                 8.875416,    7.343407,    6.076601,    5.027703,    4.160378,    3.442244,   2.848425,   2.356751,
                 1.950190,    1.613562,    1.335208,    1.104734,    0.914043,    0.756362,   0.000000};
  CountStandardDeviations vecE = {
      1.000000,  1.000000,  1.000000,  1.000000,  1.187330,  1.416570,  1.690260,  2.016600,  2.406230,  2.870800,
      3.425470,  4.086820,  4.876440,  5.817940,  6.942020,  8.282330,  9.882550,  11.791900, 14.068000, 16.781200,
      19.991300, 23.715500, 27.809000, 31.863500, 35.190400, 37.080200, 37.155300, 35.586500, 32.957800, 29.912500,
      26.897200, 24.104900, 21.585500, 19.325000, 17.302000, 15.489600, 13.867000, 12.415400, 11.114900, 9.951300,
      8.908900,  7.976270,  7.140760,  6.393230,  5.723530,  5.124370,  4.587590,  4.107340,  3.677090,  3.292150,
      2.947300,  2.638760,  2.362350,  2.115050,  1.893500,  1.695150,  1.517700,  1.358720,  1.000000,  1.000000,
      1.000000,  1.000000,  1.000000,  1.000000,  1.168530,  1.366060,  1.596820,  1.866750,  2.182090,  2.550950,
      2.981870,  3.485930,  4.074790,  4.763600,  5.568260,  6.509400,  7.608330,  8.891690,  10.383700, 12.107300,
      14.065200, 16.231500, 18.522700, 20.798500, 22.861300, 24.495200, 25.519100, 25.829800, 25.434200, 24.441500,
      23.024500, 21.373800, 19.648200, 17.960600, 16.369600, 14.901300, 13.557300, 12.333300, 11.218700, 10.205200,
      9.282780,  8.444220,  7.680940,  6.987080,  6.355510,  5.781020,  5.258800,  4.783440,  4.351330,  3.958010,
      3.600460,  3.275010,  2.979160,  2.709870,  2.465080,  2.242250,  2.039700,  1.855330,  1.687730,  1.535170,
      1.396490,  1.270260,  1.155510,  1.051060,  1.000000,  1.000000,  1.000000};
  const size_t size = 127;

  // b) Get workspace
  auto dataws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

  // c) Input data
  dataws->setHistogram(0, vecX, vecY, vecE);

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Generate data (vectors) containg twin peak w/o background
 */
API::MatrixWorkspace_sptr generateTwinPeakData() {

  // These data of reflection (932) and (852)
  Points vecX = {12646.470000, 12658.333000, 12670.196000, 12682.061000, 12693.924000, 12705.787000,
                 12717.650000, 12729.514000, 12741.378000, 12753.241000, 12765.104000, 12776.968000,
                 12788.831000, 12800.695000, 12812.559000, 12824.422000, 12836.285000, 12848.148000,
                 12860.013000, 12871.876000, 12883.739000, 12895.603000, 12907.466000};
  Counts vecY = {0.569167,   0.355704,  0.851669,  4.611006,   24.960907,   135.082310, 613.158870, 587.661740,
                 213.997240, 85.320320, 86.317253, 334.309050, 1171.018700, 732.479430, 258.377170, 90.549515,
                 31.733501,  11.121155, 3.904864,  0.041584,   0.223411,    1.200295,   6.448674};
  CountStandardDeviations vecE = {1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000,
                                  1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000,
                                  1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000,
                                  1000.000000, 1000.000000, 1000.000000, 1000.000000, 1000.000000};

  const size_t size = 23;

  // b) Get workspace
  auto dataws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

  // c) Input data
  dataws->setHistogram(0, vecX, vecY, vecE);

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Generate data with background
 * The data comes from NOMAD 11848-4 (bank 4)
 */
API::MatrixWorkspace_sptr generate1PeakDataPlusBackground() {

  Points vecX = {15804.515080, 15819.155170, 15833.808820, 15848.476040, 15863.156850, 15877.851260, 15892.559290,
                 15907.280930, 15922.016220, 15936.765150, 15951.527740, 15966.304010, 15981.093970, 15995.897630,
                 16010.715000, 16025.546100, 16040.390930, 16055.249520, 16070.121870, 16085.007990, 16099.907910,
                 16114.821630, 16129.749160, 16144.690520, 16159.645720, 16174.614780, 16189.597700, 16204.594500,
                 16219.605190, 16234.629790, 16249.668300, 16264.720740, 16279.787130, 16294.867480, 16309.961790,
                 16325.070090, 16340.192380, 16355.328680, 16370.479000, 16385.643350, 16400.821750, 16416.014210,
                 16431.220750, 16446.441370, 16461.676090, 16476.924920, 16492.187880, 16507.464970, 16522.756220,
                 16538.061630, 16553.381220, 16568.715010, 16584.062990, 16599.425190, 16614.801630, 16630.192300,
                 16645.597230, 16661.016440, 16676.449920, 16691.897700, 16707.359800, 16722.836210, 16738.326960,
                 16753.832060, 16769.351530, 16784.885370, 16800.433590, 16815.996220, 16831.573270, 16847.164750,
                 16862.770670, 16878.391040, 16894.025890};
  Counts vecY = {0.000939, 0.003453, -0.000912, 0.001885, 0.003328,  0.003645,  0.002186, 0.001818,  0.001830,
                 0.002610, 0.007754, 0.011196,  0.021295, 0.034910,  0.069452,  0.119978, 0.213131,  0.328728,
                 0.463766, 0.606728, 0.709954,  0.727371, 0.680923,  0.561676,  0.426857, 0.302604,  0.207706,
                 0.146549, 0.096288, 0.069523,  0.044938, 0.031268,  0.024555,  0.020716, 0.014238,  0.010839,
                 0.009522, 0.006665, 0.004833,  0.006066, 0.007979,  0.003380,  0.006960, 0.000764,  -0.001748,
                 0.003117, 0.002671, 0.000732,  0.001814, -0.000605, -0.003475, 0.003512, -0.000796, 0.006515,
                 0.010276, 0.004984, 0.006924,  0.007722, 0.006036,  0.003330,  0.002929, 0.007368,  0.001504,
                 0.002403, 0.004263, 0.001860,  0.002712, 0.001574,  -0.001803, 0.000825, -0.003368, -0.003277,
                 -0.001997};
  CountStandardDeviations vecE = {
      0.001830, 0.001826, 0.001835, 0.001824, 0.001851, 0.001836, 0.001845, 0.001869, 0.001882, 0.001898, 0.001915,
      0.001932, 0.001969, 0.002054, 0.002229, 0.002469, 0.002831, 0.003231, 0.003662, 0.004061, 0.004333, 0.004400,
      0.004303, 0.004013, 0.003638, 0.003256, 0.002927, 0.002681, 0.002477, 0.002343, 0.002272, 0.002194, 0.002167,
      0.002138, 0.002107, 0.002104, 0.002092, 0.002101, 0.002102, 0.002085, 0.002110, 0.002091, 0.002097, 0.002122,
      0.002122, 0.002117, 0.002126, 0.002175, 0.002159, 0.002176, 0.002180, 0.002188, 0.002204, 0.002243, 0.002229,
      0.002247, 0.002239, 0.002232, 0.002285, 0.002255, 0.002310, 0.002281, 0.002326, 0.002273, 0.002314, 0.002311,
      0.002316, 0.002333, 0.002348, 0.002338, 0.002344, 0.002340, 0.002348};

  // b) Get workspace
  const size_t size = 73;
  auto dataws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

  // c) Input data
  dataws->setHistogram(0, vecX, vecY, vecE);

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Generate backgroundless peak 220 from arg_si.dat (Fullprof example)
 */
API::MatrixWorkspace_sptr generateArgSiPeak220() {

  Points vecx = {31019.300000, 31050.400000, 31081.400000, 31112.500000, 31143.600000, 31174.800000, 31205.900000,
                 31237.100000, 31268.400000, 31299.600000, 31330.900000, 31362.300000, 31393.600000, 31425.000000,
                 31456.500000, 31487.900000, 31519.400000, 31550.900000, 31582.500000, 31614.100000, 31645.700000,
                 31677.300000, 31709.000000, 31740.700000, 31772.500000, 31804.200000};
  Counts vecy = {0.026242, 0.026461, 0.028096, 0.028964, 0.028611, 0.034328, 0.039418, 0.053557, 0.098894,
                 0.205568, 0.439015, 0.819417, 1.338839, 1.744511, 1.834295, 1.534555, 1.031174, 0.528931,
                 0.231984, 0.109614, 0.063961, 0.048803, 0.038360, 0.036393, 0.032483, 0.030962};
  vecy -= 0.02295189;
  CountStandardDeviations vece = {0.000927, 0.000932, 0.000963, 0.000980, 0.000975, 0.001073, 0.001155,
                                  0.001358, 0.001887, 0.002854, 0.004564, 0.007022, 0.010193, 0.012625,
                                  0.013176, 0.011415, 0.008391, 0.005223, 0.003110, 0.002032, 0.001523,
                                  0.001323, 0.001169, 0.001140, 0.001077, 0.001052};

  // b) Get workspace
  size_t size = 26;
  auto dataws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

  // c) Input data
  dataws->setHistogram(0, vecx, vecy, vece);

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Create data workspace without background
 */
API::MatrixWorkspace_sptr createInputDataWorkspace(int option) {
  API::MatrixWorkspace_sptr dataws;

  if (option != 4) {
    // Generate data

    // a) Generate data
    switch (option) {
    case 1:
      dataws = generateSeparateTwoPeaksData2();
      break;

    case 2:
      dataws = generateTwinPeakData();
      break;

    case 3:
      dataws = generate1PeakDataPlusBackground();
      break;

    case 9:
      dataws = generateArgSiPeak220();
      break;

    default:
      stringstream errmsg;
      errmsg << "Option " << option << " to generate a data workspace is  not supported.";
      throw runtime_error(errmsg.str());
      break;
    }

  } else {
    // Load from column file
    throw runtime_error("Using .dat file is not allowed for committing. ");
  }

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Create parameter workspace for peak calculation.
 * If a parameter is to be modifed by absolute value, then this parameter
 * will be fit.
 * @param parammodifymap :: map containing parameter and its value to update
 * from original
 * @param option :: choice to select parameter values.
 */
TableWorkspace_sptr createPeakParameterWorkspace(map<std::string, double> parammodifymap, int option) {
  // 1. Set the parameter and fit map
  std::map<std::string, double> paramvaluemap;
  std::map<std::string, std::string> paramfitmap;

  // 2. Get parameters (map) according to option
  switch (option) {
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
    errmsg << "Peak parameters option = " << option << " is not supported."
           << ".\n"
           << "Supported options are (1) Backgroundless, (2) Background Bank "
              "7, (3) NOMAD Bank4.";
    throw std::invalid_argument(errmsg.str());
    break;
  }
  cout << "Parameter Value Map Size = " << paramvaluemap.size() << '\n';

  // 3. Fix all peak parameters
  std::map<std::string, double>::iterator mit;
  for (mit = paramvaluemap.begin(); mit != paramvaluemap.end(); ++mit) {
    std::string parname = mit->first;
    paramfitmap.emplace(parname, "t");
  }

  std::cout << "Parameter Fit Map Size = " << paramfitmap.size() << '\n';

  // 4. Parpare the table workspace
  DataObjects::TableWorkspace *tablews;

  tablews = new DataObjects::TableWorkspace();
  DataObjects::TableWorkspace_sptr parameterws(tablews);

  tablews->addColumn("str", "Name");
  tablews->addColumn("double", "Value");
  tablews->addColumn("str", "FitOrTie");

  // 5. Add value
  std::map<std::string, double>::iterator paramiter;
  for (paramiter = paramvaluemap.begin(); paramiter != paramvaluemap.end(); ++paramiter) {
    // a) Access value from internal parameter maps and parameter to modify
    // map
    std::string parname = paramiter->first;
    double parvalue;
    std::string fit_tie;

    // a) Whether is a parameter w/ value to be modified
    std::map<std::string, double>::iterator moditer;
    moditer = parammodifymap.find(parname);
    if (moditer != parammodifymap.end()) {
      // Modify
      parvalue = moditer->second;
      fit_tie = "f";
    } else {
      // Use original
      parvalue = paramiter->second;
      fit_tie = paramfitmap[parname];
    }

    // c) Append to table
    std::cout << parname << ": " << parvalue << "  " << fit_tie << '\n';

    API::TableRow newparam = parameterws->appendRow();
    newparam << parname << parvalue << fit_tie;
  }

  std::cout << "ParameterWorkspace: Size = " << parameterws->rowCount() << '\n';

  return parameterws;
}
} // namespace

class LeBailFitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFitTest *createSuite() { return new LeBailFitTest(); }
  static void destroySuite(LeBailFitTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test calculation mode on calculating 2 peaks
   *  It is same as LeBailFunctionTest.test_CalculatePeakParameters()
   */
  void Ptest_CalculationSimpleMode() {
    // Create input workspaces
    API::MatrixWorkspace_sptr dataws;
    dataws = createInputDataWorkspace(1);

    //  Profile parameters from backgroundless setup
    map<string, double> modmap;
    TableWorkspace_sptr parameterws = createPeakParameterWorkspace(modmap, 1);

    //  Add reflection (111) and (110)
    TableWorkspace_sptr hklws;
    double h110 = 660.0 / 0.0064;
    double h111 = 1370.0 / 0.008;
    std::vector<double> peakheights;
    peakheights.emplace_back(h111);
    peakheights.emplace_back(h110);
    std::vector<std::vector<int>> hkls;
    std::vector<int> p111;
    p111.emplace_back(1);
    p111.emplace_back(1);
    p111.emplace_back(1);
    hkls.emplace_back(p111);
    std::vector<int> p110;
    p110.emplace_back(1);
    p110.emplace_back(1);
    p110.emplace_back(0);
    hkls.emplace_back(p110);
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
    DataObjects::Workspace2D_sptr outws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
        AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);
    if (!outws) {
      return;
    }

    // 9 fixed + 2 individual peaks
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 11);

    /*
    for (size_t i = 0; i < outws->dataY(0).size(); ++i)
      std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << "\t\t"
    << outws->dataY(1)[i] << '\n';
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
  void test_CalculationSimpleModeProfile9() {
    // Create input workspaces
    API::MatrixWorkspace_sptr dataws;
    dataws = createInputDataWorkspace(9);

    //  Profile parameters from backgroundless setup
    map<string, double> modmap;
    TableWorkspace_sptr parameterws = createPeakParameterWorkspace(modmap, 9);

    //  Add reflection (220)
    TableWorkspace_sptr hklws;
    double h220 = 660.0 / 0.0064;
    std::vector<double> peakheights;
    peakheights.emplace_back(h220);

    std::vector<std::vector<int>> hkls;
    std::vector<int> p220(3, 2);
    p220[2] = 0;
    hkls.emplace_back(p220);

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
    DataObjects::Workspace2D_sptr outws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
        AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);
    if (!outws) {
      return;
    }

    // 9 fixed + 2 individual peaks
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 10);

    return;

    /*
    for (size_t i = 0; i < outws->dataY(0).size(); ++i)
      std::cout << outws->dataX(0)[i] << "\t\t" << outws->dataY(0)[i] << "\t\t"
    << outws->dataY(1)[i] << '\n';
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
  void test_CalculationModeFull() {
    // 1. Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;

    dataws = createInputDataWorkspace(1);
    map<string, double> emptymap;
    parameterws = createPeakParameterWorkspace(emptymap, 1);

    // a) Add reflection (111) and (110)
    double h110 = 660.0 / 0.0064;
    double h111 = 1370.0 / 0.008;
    std::vector<double> peakheights;
    peakheights.emplace_back(h111);
    peakheights.emplace_back(h110);
    std::vector<std::vector<int>> hkls;
    std::vector<int> p111;
    p111.emplace_back(1);
    p111.emplace_back(1);
    p111.emplace_back(1);
    hkls.emplace_back(p111);
    std::vector<int> p110;
    p110.emplace_back(1);
    p110.emplace_back(1);
    p110.emplace_back(0);
    hkls.emplace_back(p110);
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
    lbfit.setPropertyValue("BackgroundParameters",
                           "101.0, 0.001"); // a second order polynomial background
    lbfit.setProperty("Function", "Calculation");
    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(lbfit.execute());
    TS_ASSERT(lbfit.isExecuted());

    // 5. Get output & Test
    DataObjects::Workspace2D_sptr outws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
        AnalysisDataService::Instance().retrieve("CalculatedPeaks"));
    TS_ASSERT(outws);

    // Check background (last point)
    double bkgdx = outws->x(1).back() * 0.001 + 101.0;
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
  void test_fit1Parameter() {
    std::string testplan("zero");

    // Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;

    // Data.  Option 1
    dataws = createInputDataWorkspace(1);
    // Profile parameter
    std::map<std::string, double> parammodifymap;
    if (testplan.compare("zero") == 0) {
      parammodifymap.emplace("Zero", 2.0);
    } else if (testplan.compare("alpha") == 0) {
      double alph0 = 4.026;
      double newalph0 = alph0 * 0.05;
      parammodifymap.emplace("Alph0", newalph0);
    } else if (testplan.compare("sigma") == 0) {
      double sig1 = 9.901;
      double newsig1 = sig1 * 0.1;
      double sig0 = 127.37;
      double newsig0 = sig0 * 0.1;
      parammodifymap.emplace("Sig0", newsig0);
      parammodifymap.emplace("Sig1", newsig1);
    }
    parameterws = createPeakParameterWorkspace(parammodifymap, 1);
    // c) Reflection (111) and (110)
    double h110 = 1.0;
    double h111 = 1.0;
    std::vector<double> peakheights;
    peakheights.emplace_back(h111);
    peakheights.emplace_back(h110);
    std::vector<std::vector<int>> hkls;
    std::vector<int> p111;
    p111.emplace_back(1);
    p111.emplace_back(1);
    p111.emplace_back(1);
    hkls.emplace_back(p111);
    std::vector<int> p110;
    p110.emplace_back(1);
    p110.emplace_back(1);
    p110.emplace_back(0);
    hkls.emplace_back(p110);
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
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("FitResultWS"));
    TS_ASSERT(outws);
    if (!outws) {
      return;
    }

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);
    if (outws->getNumberHistograms() != 9) {
      return;
    }

    // 5. Check fit result
    DataObjects::TableWorkspace_sptr paramws = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(
        AnalysisDataService::Instance().retrieve("PeakParameters"));
    TS_ASSERT(paramws);
    if (!paramws) {
      return;
    }

    TS_ASSERT_EQUALS(paramws->columnCount(), 9);

    std::map<std::string, double> paramvalues;
    std::map<std::string, char> paramfitstatus;
    parseParameterTableWorkspace(paramws, paramvalues, paramfitstatus);

    if (testplan.compare("zero") == 0) {
      double zero = paramvalues["Zero"];
      cout << "Zero = " << zero << ".\n";
      TS_ASSERT_DELTA(zero, 0.0, 0.5);
    } else if (testplan.compare("alpha") == 0) {
      double alph0 = paramvalues["Alph0"];
      TS_ASSERT_DELTA(alph0, 4.026, 1.00);
    } else if (testplan.compare("sigma") == 0) {
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
  /** Test a complete LeBail Fit process with background by Monte Carlo
   * algorithm
   *  Using Run 4862 Bank 7 as the testing data
   */
  void Disabled_test_monteCarloLeBailFit_PG3Bank7() {
    // 1. Create input workspace
    API::MatrixWorkspace_sptr dataws;
    DataObjects::TableWorkspace_sptr parameterws;
    DataObjects::TableWorkspace_sptr hklws;
    DataObjects::TableWorkspace_sptr bkgdws;

    // a)  Reflections
    std::vector<std::vector<int>> hkls;
    // (222)
    vector<int> r222(3, 2);
    hkls.emplace_back(r222);
    // (311)
    vector<int> r311(3, 1);
    r311[0] = 3;
    hkls.emplace_back(r311);
    // (220)
    vector<int> r220(3, 2);
    r220[2] = 0;
    hkls.emplace_back(r220);
    // (200)
    vector<int> r200(3, 0);
    r200[0] = 2;
    hkls.emplace_back(r200);
    // (111)
    vector<int> r111(3, 1);
    hkls.emplace_back(r111);

    size_t numpeaks = hkls.size();
    std::cout << "[TESTx349] Nmber of (file imported) peaks = " << hkls.size() << '\n';

    // b) data
    dataws = createInputDataWorkspace(4);
    std::cout << "[TESTx349] Data Workspace Range: " << dataws->x(0)[0] << ", " << dataws->x(0).back() << '\n';

    // c) Generate TableWorkspaces
    std::vector<double> pkheights(numpeaks, 1.0);
    map<string, double> modmap{{"Alph0", 5.0}, {"Beta0", 5.0}};
    parameterws = createPeakParameterWorkspace(modmap, 2);
    hklws = createInputHKLWorkspace(hkls, pkheights);
    bkgdws = createBackgroundParameterWorksapce(1);

    AnalysisDataService::Instance().addOrReplace("Data", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws);
    AnalysisDataService::Instance().addOrReplace("Reflections", hklws);
    AnalysisDataService::Instance().addOrReplace("BackgroundParameters", bkgdws);

    // 2. Other properties
    std::vector<double> fitregion;
    fitregion.emplace_back(56198.0);
    fitregion.emplace_back(151239.0);

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
    if (!lbfit.isExecuted()) {
      return;
    }

    // 5. Exam
    // Take the output data:
    DataObjects::Workspace2D_sptr outws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("FittedData"));
    TS_ASSERT(outws);
    if (!outws)
      return;
    else {
      TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);
    }

    // Peaks table
    DataObjects::TableWorkspace_sptr peakparamws =
        std::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("FittedPeaks"));
    TS_ASSERT(peakparamws);
    if (!peakparamws) {
      return;
    }

    else {
      TS_ASSERT_EQUALS(peakparamws->rowCount(), 5);
    }

    // Parameters table
    DataObjects::TableWorkspace_sptr instrparamws = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(
        AnalysisDataService::Instance().retrieve("FittedParameters"));
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
  void Xtest_refineBackground() {
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
    vector<vector<int>> peakhkls;
    vector<double> peakheights;
    vector<int> p211(3, 0);
    p211[0] = 2;
    p211[1] = 1;
    p211[2] = 1;
    peakhkls.emplace_back(p211);
    peakheights.emplace_back(1.0);

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
    DataObjects::Workspace2D_sptr outws = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
        AnalysisDataService::Instance().retrieve("RefinedBackground"));
    TS_ASSERT(outws);
    if (!outws) {
      return;
    }

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 9);

#if 0

    // 4. Calcualte data
    double y25 = 1360.20;
    double y59 = 0.285529;
    double y86 = 648.998;

    TS_ASSERT_DELTA(outws->y(1)[25], y25, 0.1);
    TS_ASSERT_DELTA(outws->y(1)[59], y59, 0.0001);
    TS_ASSERT_DELTA(outws->y(1)[86], y86, 0.001);
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

  /// ===============================  Check Results
  /// =================================== ///

  /*
   * Parse parameter table workspace to 2 map
   */
  void parseParameterTableWorkspace(const DataObjects::TableWorkspace_sptr &paramws,
                                    std::map<std::string, double> &paramvalues,
                                    std::map<std::string, char> &paramfitstatus) {

    for (size_t irow = 0; irow < paramws->rowCount(); ++irow) {
      API::TableRow row = paramws->getRow(irow);
      std::string parname;
      double parvalue;
      std::string fitstatus;
      row >> parname >> parvalue >> fitstatus;

      char fitortie = 't';
      if (fitstatus.size() > 0) {
        fitortie = fitstatus[0];
      } else {
        std::cout << "ParameterWorkspace:  parameter " << parname << " has am empty field for fit/tie. \n";
      }

      paramvalues.emplace(parname, parvalue);
      paramfitstatus.emplace(parname, fitortie);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a table worskpace for background parameters
   * Note: It is just desired for bank 7 run 4862
   */
  DataObjects::TableWorkspace_sptr createBackgroundParameterWorksapce(int option) {
    // 1. Create map
    map<string, double> bkgdparmap;
    switch (option) {
    case 1:
      bkgdparmap.emplace("A0", -197456);
      bkgdparmap.emplace("A1", 15.5819);
      bkgdparmap.emplace("A2", -0.000467362);
      bkgdparmap.emplace("A3", 5.59069e-09);
      bkgdparmap.emplace("A4", 2.81875e-14);
      bkgdparmap.emplace("A5", -1.88986e-18);
      bkgdparmap.emplace("A6", 2.9137e-23);
      bkgdparmap.emplace("A7", -2.50121e-28);
      bkgdparmap.emplace("A8", 1.3279e-33);
      bkgdparmap.emplace("A9", -4.33776e-39);
      bkgdparmap.emplace("A10", 8.01018e-45);
      bkgdparmap.emplace("A11", -6.40846e-51);

      break;

    case 2:
      // NOMAD Bank4
      bkgdparmap.emplace("A0", 0.73);
      bkgdparmap.emplace("A1", -8.0E-5);
      bkgdparmap.emplace("A2", 0.0);
      bkgdparmap.emplace("A3", 0.0);
      bkgdparmap.emplace("A4", 0.0);
      bkgdparmap.emplace("A5", 0.0);

      break;

    default:
      stringstream errss;
      errss << "Option " << option << " is not supported.";
      throw runtime_error(errss.str());
      break;
    }

    // 2. Build table workspace
    DataObjects::TableWorkspace *tablewsptr = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr tablews(tablewsptr);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");

    map<string, double>::iterator mapiter;
    for (mapiter = bkgdparmap.begin(); mapiter != bkgdparmap.end(); ++mapiter) {
      string parname = mapiter->first;
      double parvalue = mapiter->second;

      API::TableRow newrow = tablews->appendRow();
      newrow << parname << parvalue;
    }

    return tablews;
  }
};

class LeBailFitTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFitTestPerformance *createSuite() { return new LeBailFitTestPerformance(); }
  static void destroySuite(LeBailFitTestPerformance *suite) { delete suite; }

  void setUp() override {
    dataws1 = createInputDataWorkspace(1);
    dataws9 = createInputDataWorkspace(9);
    map<string, double> modmap;
    parameterws1 = createPeakParameterWorkspace(modmap, 1);
    parameterws9 = createPeakParameterWorkspace(modmap, 9);

    //  Reflection (220)
    std::vector<double> peakheights(1, 660.0 / 0.0064);
    std::vector<std::vector<int>> hkls = {{2, 2, 0}};
    hkl220ws = createInputHKLWorkspace(hkls, peakheights);

    // Reflections (111) and (110)
    peakheights = {1370.0 / 0.008, 660.0 / 0.0064};
    hkls = {{1, 1, 1}, {1, 1, 0}};
    hkl111110ws = createInputHKLWorkspace(hkls, peakheights);
  }

  void test_CalculationSimpleModeProfile9() {
    // Same as unit test

    AnalysisDataService::Instance().addOrReplace("Data", dataws9);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws9);
    AnalysisDataService::Instance().addOrReplace("Reflections", hkl220ws);

    LeBailFit lbfit;
    lbfit.initialize();
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
    lbfit.execute();

    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("CalculatedPeaks");
    AnalysisDataService::Instance().remove("PeakParameterWS");
  }

  void test_CalculationModeFull() {
    // Same as unit test

    AnalysisDataService::Instance().addOrReplace("Data", dataws1);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", parameterws1);
    AnalysisDataService::Instance().addOrReplace("Reflections", hkl111110ws);

    LeBailFit lbfit;
    lbfit.initialize();
    lbfit.setPropertyValue("InputWorkspace", "Data");
    lbfit.setProperty("OutputWorkspace", "CalculatedPeaks");
    lbfit.setPropertyValue("InputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("OutputParameterWorkspace", "PeakParameters");
    lbfit.setPropertyValue("InputHKLWorkspace", "Reflections");
    lbfit.setProperty("OutputPeaksWorkspace", "PeakParameterWS");
    lbfit.setProperty("WorkspaceIndex", 0);
    lbfit.setProperty("BackgroundType", "Polynomial");
    lbfit.setPropertyValue("BackgroundParameters", "101.0, 0.001");
    lbfit.setProperty("Function", "Calculation");
    lbfit.setProperty("UseInputPeakHeights", false);
    lbfit.setProperty("PeakRadius", 8);
    lbfit.execute();

    AnalysisDataService::Instance().remove("Data");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("Reflections");
    AnalysisDataService::Instance().remove("CalculatedPeaks");
    AnalysisDataService::Instance().remove("PeakParameterWS");
  }

private:
  API::MatrixWorkspace_sptr dataws1;
  API::MatrixWorkspace_sptr dataws9;
  TableWorkspace_sptr parameterws1;
  TableWorkspace_sptr parameterws9;
  TableWorkspace_sptr hkl220ws;
  TableWorkspace_sptr hkl111110ws;
};
