/*
* PredictFractionalPeaks.cpp
*
*  Created on: Dec 5, 2012
*      Author: ruth
*/
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <boost/math/special_functions/round.hpp>

namespace Mantid {
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
namespace Crystal {

DECLARE_ALGORITHM(PredictFractionalPeaks)

/// Initialise the properties
void PredictFractionalPeaks::init() {
  declareProperty(
      make_unique<WorkspaceProperty<IPeaksWorkspace>>("Peaks", "",
                                                      Direction::Input),
      "Workspace of Peaks with orientation matrix that indexed the peaks and "
      "instrument loaded");

  declareProperty(
      make_unique<WorkspaceProperty<IPeaksWorkspace>>("FracPeaks", "",
                                                      Direction::Output),
      "Workspace of Peaks with peaks with fractional h,k, and/or l values");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("HOffset"), "-0.5,0.0,0.5"),
                  "Offset in the h direction");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("KOffset"), "0"),
                  "Offset in the h direction");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("LOffset"), "-0.5,0.5"),
                  "Offset in the h direction");

  declareProperty("IncludeAllPeaksInRange", false,
                  "If false only offsets from peaks from Peaks are used");

  declareProperty(
      make_unique<PropertyWithValue<double>>("Hmin", -8.0, Direction::Input),
      "Minimum H value to use");
  declareProperty(
      make_unique<PropertyWithValue<double>>("Hmax", 8.0, Direction::Input),
      "Maximum H value to use");
  declareProperty(
      make_unique<PropertyWithValue<double>>("Kmin", -8.0, Direction::Input),
      "Minimum K value to use");
  declareProperty(
      make_unique<PropertyWithValue<double>>("Kmax", 8.0, Direction::Input),
      "Maximum K value to use");
  declareProperty(
      make_unique<PropertyWithValue<double>>("Lmin", -8.0, Direction::Input),
      "Minimum L value to use");
  declareProperty(
      make_unique<PropertyWithValue<double>>("Lmax", 8.0, Direction::Input),
      "Maximum L value to use");

  setPropertySettings(
      "Hmin", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Hmax", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "Kmin", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Kmax", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "Lmin", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "Lmax", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
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
    N = boost::math::iround((Hmax - Hmin + 1) * (Kmax - Kmin + 1) *
                            (Lmax - Lmin + 1));
    N = max<int>(100, N);
  }
  IPeak &peak0 = Peaks->getPeak(0);
  int RunNumber = peak0.getRunNumber();
  Gon = peak0.getGoniometerMatrix();
  Progress prog(this, 0.0, 1.0, N);
  if (includePeaksInRange) {

    hkl[0] = Hmin;
    hkl[1] = Kmin;
    hkl[2] = Lmin;
  } else {
    hkl[0] = peak0.getH();
    hkl[1] = peak0.getK();
    hkl[2] = peak0.getL();
  }

  const Kernel::DblMatrix &UB = ol.getUB();
  vector<vector<int>> AlreadyDonePeaks;
  bool done = false;
  int ErrPos = 1; // Used to determine position in code of a throw
  while (!done) {
    for (double hOffset : hOffsets) {
      for (double kOffset : kOffsets) {
        for (double lOffset : lOffsets) {
          try {
            V3D hkl1(hkl);
            ErrPos = 0;

            hkl1[0] += hOffset;
            hkl1[1] += kOffset;
            hkl1[2] += lOffset;

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
              vector<int> SavPk{RunNumber,
                                boost::math::iround(1000.0 * hkl1[0]),
                                boost::math::iround(1000.0 * hkl1[1]),
                                boost::math::iround(1000.0 * hkl1[2])};

              // TODO keep list sorted so searching is faster?
              auto it =
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
              throw std::invalid_argument("Invalid data at this point");
          }
        }
      }
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
