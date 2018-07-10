/*
* PredictSatellitePeaks.cpp
*
*  Created on: Dec 5, 2012
*      Author: ruth
*/
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include <boost/math/special_functions/round.hpp>

namespace Mantid {
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
namespace Crystal {

DECLARE_ALGORITHM(PredictSatellitePeaks)

/// Initialise the properties
void PredictSatellitePeaks::init() {
  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>("Peaks", "",
                                                     Direction::Input),
      "Workspace of Peaks with orientation matrix that indexed the peaks and "
      "instrument loaded");

  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>("SatellitePeaks", "",
                                                     Direction::Output),
      "Workspace of Peaks with peaks with fractional h,k, and/or l values");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("OffsetVector1"), "0.0,0.0,0.0,0"),
                  "Offsets for h, k, l directions and order");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("OffsetVector2"), "0.0,0.0,0.0,0"),
                  "Offsets for h, k, l directions and order");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("OffsetVector3"), "0.0,0.0,0.0,0"),
                  "Offsets for h, k, l directions and order");

  declareProperty("IncludeIntegerHKL", true,
                  "If false order 0 peaks are not included in workspace (integer HKL)");

  declareProperty("IncludeAllPeaksInRange", false,
                  "If false only offsets from peaks from Peaks workspace in input are used");

  declareProperty(
      make_unique<PropertyWithValue<double>>("WavelengthMin", 0.1, Direction::Input),
      "Minimum wavelength limit at which to start looking for single-crystal peaks.");
  declareProperty(
      make_unique<PropertyWithValue<double>>("WavelengthMax", 100.0, Direction::Input),
      "Maximum wavelength limit at which to start looking for single-crystal peaks.");
  declareProperty(
      make_unique<PropertyWithValue<double>>("MinDSpacing", 0.1, Direction::Input),
      "Minimum d-spacing of peaks to consider. Default = 1.0");
  declareProperty(
      make_unique<PropertyWithValue<double>>("MaxDSpacing", 100.0, Direction::Input),
      "Maximum d-spacing of peaks to consider");

  setPropertySettings(
      "WavelengthMin", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "WavelengthMax", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "MinDSpacing", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "MaxDSpacing", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                  string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
}

/// Run the algorithm
void PredictSatellitePeaks::exec() {
  PeaksWorkspace_sptr Peaks = getProperty("Peaks");
  if (!Peaks)
    throw std::invalid_argument(
        "Input workspace is not a PeaksWorkspace. Type=" + Peaks->id());

  vector<double> offsets1 = getProperty("OffsetVector1");
  vector<double> offsets2 = getProperty("OffsetVector2");
  vector<double> offsets3 = getProperty("OffsetVector3");
  if (offsets1.empty()) {
    offsets1.push_back(0.0);
    offsets1.push_back(0.0);
    offsets1.push_back(0.0);
    offsets1.push_back(0.0);
  }
  if (offsets2.empty()) {
    offsets2.push_back(0.0);
    offsets2.push_back(0.0);
    offsets2.push_back(0.0);
    offsets2.push_back(0.0);
  }
  if (offsets3.empty()) {
    offsets3.push_back(0.0);
    offsets3.push_back(0.0);
    offsets3.push_back(0.0);
    offsets3.push_back(0.0);
  } 

  bool includePeaksInRange = getProperty("IncludeAllPeaksInRange");
  bool includeOrderZero = getProperty("IncludeIntegerHKL");

  if (Peaks->getNumberPeaks() <= 0) {
    g_log.error() << "There are No peaks in the input PeaksWorkspace\n";
    return;
  }

  API::Sample samp = Peaks->sample();

  Geometry::OrientedLattice &ol = samp.getOrientedLattice();

  Geometry::Instrument_const_sptr Instr = Peaks->getInstrument();

  auto OutPeaks = boost::dynamic_pointer_cast<IPeaksWorkspace>(
      WorkspaceFactory::Instance().createPeaks());
  OutPeaks->setInstrument(Instr);
  OutPeaks->mutableRun().addProperty<std::vector<double>>("Offset1", offsets1, true);
  OutPeaks->mutableRun().addProperty<std::vector<double>>("Offset2", offsets2, true);
  OutPeaks->mutableRun().addProperty<std::vector<double>>("Offset3", offsets3, true);

  V3D hkl;
  int peakNum = 0;
  const auto NPeaks = Peaks->getNumberPeaks();
  Kernel::Matrix<double> Gon;
  Gon.identityMatrix();

  const double lambdaMin = getProperty("WavelengthMin");
  const double lambdaMax = getProperty("WavelengthMax");
  IPeak &peak0 = Peaks->getPeak(0);

  std::vector<V3D> possibleHKLs;
  if (includePeaksInRange) {
    const double dMin = getProperty("MinDSpacing");
    const double dMax = getProperty("MaxDSpacing");
    Geometry::HKLGenerator gen(ol, dMin);
    auto filter =
        boost::make_shared<HKLFilterDRange>(ol, dMin, dMax);
  
    V3D hkl = *(gen.begin());
    g_log.information() << "HKL range for d_min of " << dMin << " to d_max of "
                      << dMax << " is from " << hkl << " to "
                      << hkl * -1.0 << ", a total of " << gen.size()
                      << " possible HKL's\n";
  if (gen.size() > 10000000000)
    throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                "your d_min value too small?");

  possibleHKLs.clear();
  possibleHKLs.reserve(gen.size());
  std::remove_copy_if(gen.begin(), gen.end(), std::back_inserter(possibleHKLs),
                      (~filter)->fn());
  } else {
    hkl[0] = peak0.getH();
    hkl[1] = peak0.getK();
    hkl[2] = peak0.getL();
  }

  size_t N = NPeaks*(1+int(offsets1[3]*2+offsets2[3]*2+offsets3[3]*2));
  if (includePeaksInRange) {
    N = possibleHKLs.size();
    N = max<size_t>(100, N);
  }
  auto RunNumber = peak0.getRunNumber();
  const Kernel::DblMatrix &UB = ol.getUB();
  Gon = peak0.getGoniometerMatrix();
  Progress prog(this, 0.0, 1.0, N);
  vector<vector<int>> AlreadyDonePeaks;
  bool done = false;
  int ErrPos = 1; // Used to determine position in code of a throw
  Geometry::InstrumentRayTracer tracer(Peaks->getInstrument());
  DblMatrix orientedUB = Gon * UB;
  HKLFilterWavelength lambdaFilter(orientedUB, lambdaMin, lambdaMax);
  int seqNum = 0;
  size_t next = 0;
  while (!done) {
    for (int order = -static_cast<int>(offsets1[3]); order <= static_cast<int>(offsets1[3]); order++) {
        if (order == 0 && !includeOrderZero) continue; // exclude order 0
          try {
            V3D hkl1(hkl);
            ErrPos = 0;

            hkl1[0] += order * offsets1[0];
            hkl1[1] += order * offsets1[1];
            hkl1[2] += order * offsets1[2];
            if(!lambdaFilter.isAllowed(hkl1) && includePeaksInRange) continue;

            Kernel::V3D Qs = UB * hkl1;
            Qs *= 2.0;
            Qs *= M_PI;
            Qs = Gon * Qs;
            if (Qs[2] <= 0)
              continue;

            ErrPos = 1;

            boost::shared_ptr<IPeak> peak(Peaks->createPeak(Qs, 1));

            peak->setGoniometerMatrix(Gon);

            if (Qs[2] > 0 && peak->findDetector(tracer)) {
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
              peak->setPeakNumber(seqNum);
              seqNum++;
              peak->setRunNumber(RunNumber);
              peak->setModStru(V3D(order,0,0));
              OutPeaks->addPeak(*peak);
            }
          } catch (...) {
            if (ErrPos != 1) // setQLabFrame in createPeak throws exception
              throw std::invalid_argument("Invalid data at this point");
    }
    }
    for (int order = -static_cast<int>(offsets2[3]); order <= static_cast<int>(offsets2[3]); order++) {
        if (order == 0) continue; // already added with 1st vector
          try {
            V3D hkl1(hkl);
            ErrPos = 0;

            hkl1[0] += order * offsets2[0];
            hkl1[1] += order * offsets2[1];
            hkl1[2] += order * offsets2[2];

            Kernel::V3D Qs = UB * hkl1;
            Qs *= 2.0;
            Qs *= M_PI;
            Qs = Gon * Qs;
            if (Qs[2] <= 0)
              continue;

            ErrPos = 1;

            boost::shared_ptr<IPeak> peak(Peaks->createPeak(Qs, 1));

            peak->setGoniometerMatrix(Gon);

            if (Qs[2] > 0 && peak->findDetector(tracer)) {
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
              peak->setPeakNumber(seqNum);
              seqNum++;
              peak->setRunNumber(RunNumber);
              peak->setModStru(V3D(0,order,0));
              OutPeaks->addPeak(*peak);
            }
          } catch (...) {
            if (ErrPos != 1) // setQLabFrame in createPeak throws exception
              throw std::invalid_argument("Invalid data at this point");
    }
    }
    for (int order = -static_cast<int>(offsets3[3]); order <= static_cast<int>(offsets3[3]); order++) {
        if (order == 0) continue; // already added with 1st vector
          try {
            V3D hkl1(hkl);
            ErrPos = 0;

            hkl1[0] += order * offsets3[0];
            hkl1[1] += order * offsets3[1];
            hkl1[2] += order * offsets3[2];

            Kernel::V3D Qs = UB * hkl1;
            Qs *= 2.0;
            Qs *= M_PI;
            Qs = Gon * Qs;
            if (Qs[2] <= 0)
              continue;

            ErrPos = 1;

            boost::shared_ptr<IPeak> peak(Peaks->createPeak(Qs, 1));

            peak->setGoniometerMatrix(Gon);

            if (Qs[2] > 0 && peak->findDetector(tracer)) {
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
              peak->setPeakNumber(seqNum);
              seqNum++;
              peak->setRunNumber(RunNumber);
              peak->setModStru(V3D(0,0,order));
              OutPeaks->addPeak(*peak);
            }
          } catch (...) {
            if (ErrPos != 1) // setQLabFrame in createPeak throws exception
              throw std::invalid_argument("Invalid data at this point");
    }
    }
    if (includePeaksInRange) {
      next++;
      if (next == possibleHKLs.size()) break;
      hkl = possibleHKLs[next];
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

  setProperty("SatellitePeaks", OutPeaks);
}

} // namespace Crystal
} // namespace Mantid
