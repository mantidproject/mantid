/*
* PredictFractionalPeaks.cpp
*
*  Created on: Dec 5, 2012
*      Author: ruth
*/
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

//#include "MantidKernel/Strings.h"
namespace Mantid {
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
namespace Crystal {

DECLARE_ALGORITHM(PredictFractionalPeaks)

PredictFractionalPeaks::~PredictFractionalPeaks() {}

PredictFractionalPeaks::PredictFractionalPeaks() : Algorithm() {}

/// Initialise the properties
void PredictFractionalPeaks::init() {
  declareProperty(
      new WorkspaceProperty<IPeaksWorkspace>("Peaks", "", Direction::Input),
      "Workspace of Peaks with orientation matrix that indexed the peaks and "
      "instrument loaded");

  declareProperty(
      new WorkspaceProperty<IPeaksWorkspace>("FracPeaks", "",
                                             Direction::Output),
      "Workspace of Peaks with peaks with fractional h,k, and/or l values");

  declareProperty(
      new Kernel::ArrayProperty<double>(string("HOffset"), string("-.5,0, .5")),
      "Offset in the h direction");

  declareProperty(
      new Kernel::ArrayProperty<double>(string("KOffset"), string("0")),
      "Offset in the h direction");

  declareProperty(
      new Kernel::ArrayProperty<double>(string("LOffset"), string("-.5,.5")),
      "Offset in the h direction");

  declareProperty("IncludeAllPeaksInRange", false,
                  "If false only offsets from peaks from Peaks are used");

  declareProperty(new PropertyWithValue<double>("Hmin", -8.0, Direction::Input),
                  "Minimum H value to use");
  declareProperty(new PropertyWithValue<double>("Hmax", 8.0, Direction::Input),
                  "Maximum H value to use");
  declareProperty(new PropertyWithValue<double>("Kmin", -8.0, Direction::Input),
                  "Minimum K value to use");
  declareProperty(new PropertyWithValue<double>("Kmax", 8.0, Direction::Input),
                  "Maximum K value to use");
  declareProperty(new PropertyWithValue<double>("Lmin", -8.0, Direction::Input),
                  "Minimum L value to use");
  declareProperty(new PropertyWithValue<double>("Lmax", 8.0, Direction::Input),
                  "Maximum L value to use");

  setPropertySettings(
      "Hmin", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Hmax", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "Kmin", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Kmax", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "Lmin", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Lmax", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                                              Kernel::IS_EQUAL_TO, "1"));
}

/// Run the algorithm
void PredictFractionalPeaks::exec() {
  IPeaksWorkspace_sptr ipeaks = getProperty("Peaks");
  auto Peaks = boost::dynamic_pointer_cast<PeaksWorkspace>(ipeaks);
  if (!Peaks)
    throw std::invalid_argument(
        "Input workspace is not a PeaksWorkspace. Type=" + ipeaks->id());

  vector<double> hOffsets = getProperty("HOffset");
  vector<double> kOffsets = getProperty("KOffset");
  vector<double> lOffsets = getProperty("LOffset");
  if (hOffsets.empty())
    hOffsets.push_back(0.0);
  if (kOffsets.empty())
    kOffsets.push_back(0.0);
  if (lOffsets.empty())
    lOffsets.push_back(0.0);

  bool includePeaksInRange = getProperty("IncludeAllPeaksInRange");

  if (Peaks->getNumberPeaks() <= 0) {
    g_log.error() << "There are No peaks in the input PeaksWorkspace\n";
    return;
  }

  API::Sample samp = Peaks->sample();

  Geometry::OrientedLattice &ol = samp.getOrientedLattice();

  Geometry::Instrument_const_sptr Instr = Peaks->getInstrument();

  boost::shared_ptr<IPeaksWorkspace> OutPeaks =
      WorkspaceFactory::Instance().createPeaks();
  OutPeaks->setInstrument(Instr);

  V3D hkl;
  int peakNum = 0;
  int NPeaks = Peaks->getNumberPeaks();
  Kernel::Matrix<double> Gon;
  Gon.identityMatrix();

  double Hmin = getProperty("Hmin");
  double Hmax = getProperty("Hmax");
  double Kmin = getProperty("Kmin");
  double Kmax = getProperty("Kmax");
  double Lmin = getProperty("Lmin");
  double Lmax = getProperty("Lmax");

  int N = NPeaks;
  if (includePeaksInRange) {
    N = (int)((Hmax - Hmin + 1) * (Kmax - Kmin + 1) * (Lmax - Lmin + 1) + .5);
    N = max<int>(100, N);
  }
  IPeak &peak0 = Peaks->getPeak(0);
  int RunNumber = peak0.getRunNumber();
  Gon = peak0.getGoniometerMatrix();
  Progress prog(this, 0, 1, N);
  if (includePeaksInRange) {

    hkl[0] = Hmin;
    hkl[1] = Kmin;
    hkl[2] = Lmin;
  } else {
    hkl[0] = peak0.getH();
    hkl[1] = peak0.getK();
    hkl[2] = peak0.getL();
  }

  Kernel::DblMatrix UB = ol.getUB();
  vector<vector<int>> AlreadyDonePeaks;
  bool done = false;
  int ErrPos = 1; // Used to determine position in code of a throw
  while (!done) {
    for (size_t hoffset = 0; hoffset < hOffsets.size(); hoffset++)
      for (size_t koffset = 0; koffset < kOffsets.size(); koffset++)
        for (size_t loffset = 0; loffset < lOffsets.size(); loffset++)
          try {
            V3D hkl1(hkl);
            ErrPos = 0;

            hkl1[0] += hOffsets[hoffset];
            hkl1[1] += kOffsets[koffset];
            hkl1[2] += lOffsets[loffset];

            Kernel::V3D Qs = UB * hkl1;
            Qs *= 2.0;
            Qs *= M_PI;
            Qs = Gon * Qs;
            if (Qs[2] <= 0)
              continue;

            ErrPos = 1;

            boost::shared_ptr<IPeak> peak(Peaks->createPeak(Qs, 1));

            peak->setGoniometerMatrix(Gon);

            if (Qs[2] > 0 && peak->findDetector()) {
              ErrPos = 2;
              vector<int> SavPk;
              SavPk.push_back(RunNumber);
              SavPk.push_back((int)floor(1000 * hkl1[0] + .5));
              SavPk.push_back((int)floor(1000 * hkl1[1] + .5));
              SavPk.push_back((int)floor(1000 * hkl1[2] + .5));

              // TODO keep list sorted so searching is faster?
              vector<vector<int>>::iterator it =
                  find(AlreadyDonePeaks.begin(), AlreadyDonePeaks.end(), SavPk);

              if (it == AlreadyDonePeaks.end())
                AlreadyDonePeaks.push_back(SavPk);
              else
                continue;

              peak->setHKL(hkl1);
              peak->setRunNumber(RunNumber);
              OutPeaks->addPeak(*peak);
            }
          } catch (...) {
            if (ErrPos != 1) // setQLabFrame in createPeak throws exception
              throw new std::invalid_argument("Invalid data at this point");
          }
    if (includePeaksInRange) {
      hkl[0]++;
      if (hkl[0] > Hmax) {
        hkl[0] = Hmin;
        hkl[1]++;
        if (hkl[1] > Kmax) {

          hkl[1] = Kmin;
          hkl[2]++;
          if (hkl[2] > Lmax)
            done = true;
        }
      }
    } else {
      peakNum++;
      if (peakNum >= NPeaks)
        done = true;
      else { // peak0= Peaks->getPeak(peakNum);
        IPeak &peak1 = Peaks->getPeak(peakNum);
        //??? could not assign to peak0 above. Did not work
        // the peak that peak0 was associated with did NOT change
        hkl[0] = peak1.getH();
        hkl[1] = peak1.getK();
        hkl[2] = peak1.getL();
        Gon = peak1.getGoniometerMatrix();
        RunNumber = peak1.getRunNumber();
      }
    }
    prog.report();
  }

  setProperty("FracPeaks", OutPeaks);
}

} // namespace Crystal
} // namespace Mantid
