/*
* PredictSatellitePeaks.cpp
*
*  Created on: July 15, 2018
*      Author: Vickie Lynch
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
                      string("ModVector1"), "0.0,0.0,0.0"),
                  "Offsets for h, k, l directions ");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("ModVector2"), "0.0,0.0,0.0"),
                  "Offsets for h, k, l directions ");
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<double>>(
                      string("ModVector3"), "0.0,0.0,0.0"),
                  "Offsets for h, k, l directions ");
  declareProperty(
      make_unique<PropertyWithValue<int>>("MaxOrder", 0, Direction::Input),
      "Maximum order to apply ModVectors. Default = 0");

  declareProperty(
      "IncludeIntegerHKL", true,
      "If false order 0 peaks are not included in workspace (integer HKL)");

  declareProperty("IncludeAllPeaksInRange", false, "If false only offsets from "
                                                   "peaks from Peaks workspace "
                                                   "in input are used");

  declareProperty(make_unique<PropertyWithValue<double>>("WavelengthMin", 0.1,
                                                         Direction::Input),
                  "Minimum wavelength limit at which to start looking for "
                  "single-crystal peaks.");
  declareProperty(make_unique<PropertyWithValue<double>>("WavelengthMax", 100.0,
                                                         Direction::Input),
                  "Maximum wavelength limit at which to start looking for "
                  "single-crystal peaks.");
  declareProperty(make_unique<PropertyWithValue<double>>("MinDSpacing", 0.1,
                                                         Direction::Input),
                  "Minimum d-spacing of peaks to consider. Default = 1.0");
  declareProperty(make_unique<PropertyWithValue<double>>("MaxDSpacing", 100.0,
                                                         Direction::Input),
                  "Maximum d-spacing of peaks to consider");

  setPropertySettings(
      "WavelengthMin",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "WavelengthMax",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings(
      "MinDSpacing",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings(
      "MaxDSpacing",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          string("IncludeAllPeaksInRange"), Kernel::IS_EQUAL_TO, "1"));
}

/// Run the algorithm
void PredictSatellitePeaks::exec() {
  PeaksWorkspace_sptr Peaks = getProperty("Peaks");
  if (!Peaks)
    throw std::invalid_argument(
        "Input workspace is not a PeaksWorkspace. Type=" + Peaks->id());

  vector<double> offsets1 = getProperty("ModVector1");
  vector<double> offsets2 = getProperty("ModVector2");
  vector<double> offsets3 = getProperty("ModVector3");
  int maxOrder = getProperty("MaxOrder");
  if (offsets1.empty()) {
    offsets1.push_back(0.0);
    offsets1.push_back(0.0);
    offsets1.push_back(0.0);
  }
  if (offsets2.empty()) {
    offsets2.push_back(0.0);
    offsets2.push_back(0.0);
    offsets2.push_back(0.0);
  }
  if (offsets3.empty()) {
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

  API::Sample sample = Peaks->mutableSample();

  OrientedLattice lattice = sample.getOrientedLattice();

  const auto instrument = Peaks->getInstrument();

  auto OutPeaks = boost::dynamic_pointer_cast<IPeaksWorkspace>(
      WorkspaceFactory::Instance().createPeaks());
  OutPeaks->setInstrument(instrument);
  OutPeaks->mutableSample().setOrientedLattice(&lattice);

  V3D hkl;
  int peakNum = 0;
  const auto NPeaks = Peaks->getNumberPeaks();
  Kernel::Matrix<double> goniometer;
  goniometer.identityMatrix();

  const double lambdaMin = getProperty("WavelengthMin");
  const double lambdaMax = getProperty("WavelengthMax");
  IPeak &peak0 = Peaks->getPeak(0);

  std::vector<V3D> possibleHKLs;
  if (includePeaksInRange) {
    const double dMin = getProperty("MinDSpacing");
    const double dMax = getProperty("MaxDSpacing");
    Geometry::HKLGenerator gen(lattice, dMin);
    auto filter = boost::make_shared<HKLFilterDRange>(lattice, dMin, dMax);

    V3D hkl = *(gen.begin());
    g_log.information() << "HKL range for d_min of " << dMin << " to d_max of "
                        << dMax << " is from " << hkl << " to " << hkl * -1.0
                        << ", a total of " << gen.size() << " possible HKL's\n";
    if (gen.size() > 10000000000)
      throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                  "your d_min value too small?");

    possibleHKLs.clear();
    possibleHKLs.reserve(gen.size());
    std::remove_copy_if(gen.begin(), gen.end(),
                        std::back_inserter(possibleHKLs), (~filter)->fn());
  } else {
    hkl[0] = peak0.getH();
    hkl[1] = peak0.getK();
    hkl[2] = peak0.getL();
  }

  size_t N = NPeaks * (1 + 2 * maxOrder);
  if (includePeaksInRange) {
    N = possibleHKLs.size();
    N = max<size_t>(100, N);
  }
  auto RunNumber = peak0.getRunNumber();
  auto &UB = lattice.getUB();
  goniometer = peak0.getGoniometerMatrix();
  Progress prog(this, 0.0, 1.0, N);
  vector<vector<int>> AlreadyDonePeaks;
  bool done = false;
  DblMatrix orientedUB = goniometer * UB;
  HKLFilterWavelength lambdaFilter(orientedUB, lambdaMin, lambdaMax);
  int seqNum = 0;
  size_t next = 0;
  while (!done) {
    std::string offsetName = "Offset1";
    predictOffsets(Peaks, OutPeaks, offsets1, offsetName, maxOrder, hkl,
                   goniometer, UB, lambdaFilter, includePeaksInRange,
                   includeOrderZero, RunNumber, seqNum, AlreadyDonePeaks);
    offsetName = "Offset2";
    predictOffsets(Peaks, OutPeaks, offsets2, offsetName, maxOrder, hkl,
                   goniometer, UB, lambdaFilter, includePeaksInRange,
                   includeOrderZero, RunNumber, seqNum, AlreadyDonePeaks);
    offsetName = "Offset3";
    predictOffsets(Peaks, OutPeaks, offsets3, offsetName, maxOrder, hkl,
                   goniometer, UB, lambdaFilter, includePeaksInRange,
                   includeOrderZero, RunNumber, seqNum, AlreadyDonePeaks);
    if (includePeaksInRange) {
      next++;
      if (next == possibleHKLs.size())
        break;
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
        goniometer = peak1.getGoniometerMatrix();
        RunNumber = peak1.getRunNumber();
      }
    }
    prog.report();
  }

  setProperty("SatellitePeaks", OutPeaks);
}

void PredictSatellitePeaks::predictOffsets(
    DataObjects::PeaksWorkspace_sptr Peaks,
    boost::shared_ptr<Mantid::API::IPeaksWorkspace> &OutPeaks,
    std::vector<double> offsets, std::string &label, int &maxOrder, V3D &hkl,
    Kernel::Matrix<double> &goniometer, const Kernel::DblMatrix &UB,
    HKLFilterWavelength &lambdaFilter, bool &includePeaksInRange,
    bool &includeOrderZero, int &RunNumber, int &seqNum,
    vector<vector<int>> &AlreadyDonePeaks) {
  OutPeaks->mutableRun().addProperty<std::vector<double>>(label, offsets, true);
  Geometry::InstrumentRayTracer tracer(Peaks->getInstrument());
  for (int order = -maxOrder; order <= maxOrder; order++) {
    if (order == 0 && !includeOrderZero)
      continue; // exclude order 0
    V3D hkl1(hkl);

    hkl1[0] += order * offsets[0];
    hkl1[1] += order * offsets[1];
    hkl1[2] += order * offsets[2];
    if (!lambdaFilter.isAllowed(hkl1) && includePeaksInRange)
      continue;

    Kernel::V3D Qs = goniometer * UB * hkl1 * 2.0 * M_PI;

    if (Qs[2] <= 0)
      continue;

    try {
      auto peak(Peaks->createPeak(Qs, 1));

      peak->setGoniometerMatrix(goniometer);

      if (peak->findDetector(tracer)) {
        vector<int> SavPk{RunNumber, boost::math::iround(1000.0 * hkl1[0]),
                          boost::math::iround(1000.0 * hkl1[1]),
                          boost::math::iround(1000.0 * hkl1[2])};

        auto it = find(AlreadyDonePeaks.begin(), AlreadyDonePeaks.end(), SavPk);

        if (it == AlreadyDonePeaks.end()) {
          AlreadyDonePeaks.push_back(SavPk);
          std::sort(AlreadyDonePeaks.begin(), AlreadyDonePeaks.end());
        } else {
          continue;
        }

        peak->setHKL(hkl1);
        peak->setIntHKL(hkl);
        peak->setPeakNumber(seqNum);
        seqNum++;
        peak->setRunNumber(RunNumber);
        peak->setIntMNP(V3D(order, 0, 0));
        OutPeaks->addPeak(*peak);
      }
    } catch (std::runtime_error &) {
      throw std::invalid_argument("Invalid Q vector");
    }
  }
}

} // namespace Crystal
} // namespace Mantid
