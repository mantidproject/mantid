// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * PredictSatellitePeaks.cpp
 *
 *  Created on: July 15, 2018
 *      Author: Vickie Lynch
 */
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/PeakAlgorithmHelpers.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/math/special_functions/round.hpp>

namespace Mantid {
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Crystal {

// handy shortcuts

DECLARE_ALGORITHM(PredictSatellitePeaks)

/** Constructor
 */

PredictSatellitePeaks::PredictSatellitePeaks() : m_qConventionFactor(qConventionFactor()) {}

/// Initialise the properties
void PredictSatellitePeaks::init() {
  auto latticeValidator = std::make_shared<OrientedLatticeValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("Peaks", "", Direction::Input, latticeValidator),
                  "Workspace of Peaks with orientation matrix that indexed the peaks and "
                  "instrument loaded");

  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("SatellitePeaks", "", Direction::Output),
                  "Workspace of Peaks with peaks with fractional h,k, and/or l values");

  ModulationProperties::appendTo(this);

  declareProperty("GetModVectorsFromUB", false, "If false Modulation Vectors will be read from input");

  declareProperty("IncludeIntegerHKL", true, "If false order 0 peaks are not included in workspace (integer HKL)");

  declareProperty("IncludeAllPeaksInRange", false,
                  "If false only offsets from "
                  "peaks from Peaks workspace "
                  "in input are used");

  declareProperty(std::make_unique<PropertyWithValue<double>>("WavelengthMin", 0.1, Direction::Input),
                  "Minimum wavelength limit at which to start looking for "
                  "single-crystal peaks.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("WavelengthMax", 100.0, Direction::Input),
                  "Maximum wavelength limit at which to start looking for "
                  "single-crystal peaks.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("MinDSpacing", 0.1, Direction::Input),
                  "Minimum d-spacing of peaks to consider. Default = 0.1");
  declareProperty(std::make_unique<PropertyWithValue<double>>("MaxDSpacing", 100.0, Direction::Input),
                  "Maximum d-spacing of peaks to consider");

  setPropertySettings("WavelengthMin", std::make_unique<Kernel::EnabledWhenProperty>(string("IncludeAllPeaksInRange"),
                                                                                     Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings("WavelengthMax", std::make_unique<Kernel::EnabledWhenProperty>(string("IncludeAllPeaksInRange"),
                                                                                     Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings("MinDSpacing", std::make_unique<Kernel::EnabledWhenProperty>(string("IncludeAllPeaksInRange"),
                                                                                   Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings("MaxDSpacing", std::make_unique<Kernel::EnabledWhenProperty>(string("IncludeAllPeaksInRange"),
                                                                                   Kernel::IS_EQUAL_TO, "1"));
}

/// Run the algorithm
void PredictSatellitePeaks::exec() {
  bool includePeaksInRange = getProperty("IncludeAllPeaksInRange");
  Peaks = getProperty("Peaks");

  if (!Peaks)
    throw std::invalid_argument("Input workspace is not a IPeaksWorkspace. Type=" + Peaks->id());
  if (!includePeaksInRange) {
    exec_peaks();
    return;
  }

  V3D offsets1 = getOffsetVector(ModulationProperties::ModVector1);
  V3D offsets2 = getOffsetVector(ModulationProperties::ModVector2);
  V3D offsets3 = getOffsetVector(ModulationProperties::ModVector3);
  int maxOrder = getProperty(ModulationProperties::MaxOrder);
  bool crossTerms = getProperty(ModulationProperties::CrossTerms);
  bool includeOrderZero = getProperty("IncludeIntegerHKL");
  // boolean for only including order zero once
  bool notOrderZero = false;

  API::Sample sample = Peaks->mutableSample();
  auto lattice = std::make_unique<OrientedLattice>(sample.getOrientedLattice());

  bool fromUB = getProperty("GetModVectorsFromUB");
  if (fromUB) {
    offsets1 = lattice->getModVec(0);
    offsets2 = lattice->getModVec(1);
    offsets3 = lattice->getModVec(2);
    if (maxOrder == 0)
      maxOrder = lattice->getMaxOrder();
    crossTerms = lattice->getCrossTerm();
  } else {
    lattice->setModVec1(offsets1);
    lattice->setModVec2(offsets2);
    lattice->setModVec3(offsets3);
    lattice->setMaxOrder(maxOrder);
    lattice->setCrossTerm(crossTerms);
  }

  outPeaks = std::dynamic_pointer_cast<IPeaksWorkspace>(WorkspaceFactory::Instance().createPeaks(Peaks->id()));
  outPeaks->copyExperimentInfoFrom(Peaks.get());
  outPeaks->mutableSample().setOrientedLattice(std::move(lattice));

  Kernel::Matrix<double> goniometer;
  goniometer.identityMatrix();

  const double lambdaMin = getProperty("WavelengthMin");
  const double lambdaMax = getProperty("WavelengthMax");

  std::vector<V3D> possibleHKLs;
  const double dMin = getProperty("MinDSpacing");
  const double dMax = getProperty("MaxDSpacing");
  Geometry::HKLGenerator gen(outPeaks->sample().getOrientedLattice(), dMin);
  auto dSpacingFilter = std::make_shared<HKLFilterDRange>(outPeaks->sample().getOrientedLattice(), dMin, dMax);

  V3D hkl_begin = *(gen.begin());
  g_log.information() << "HKL range for d_min of " << dMin << " to d_max of " << dMax << " is from " << hkl_begin
                      << " to " << hkl_begin * -1.0 << ", a total of " << gen.size() << " possible HKL's\n";
  if (gen.size() > MAX_NUMBER_HKLS)
    throw std::invalid_argument("More than 10 billion HKLs to search. Is "
                                "your d_min value too small?");

  possibleHKLs.clear();
  possibleHKLs.reserve(gen.size());
  std::remove_copy_if(gen.begin(), gen.end(), std::back_inserter(possibleHKLs), (~dSpacingFilter)->fn());

  size_t N = possibleHKLs.size();
  N = max<size_t>(100, N);
  const auto &UB = outPeaks->sample().getOrientedLattice().getUB();
  goniometer = Peaks->run().getGoniometerMatrix();
  int runNumber = Peaks->getRunNumber();
  Progress prog(this, 0.0, 1.0, N);
  vector<vector<int>> alreadyDonePeaks;
  auto orientedUB = goniometer * UB;
  HKLFilterWavelength lambdaFilter(orientedUB, lambdaMin, lambdaMax);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset1", offsets1, true);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset2", offsets2, true);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset3", offsets3, true);
  for (auto &hkl : possibleHKLs) {
    if (crossTerms) {
      predictOffsetsWithCrossTerms(offsets1, offsets2, offsets3, maxOrder, runNumber, goniometer, hkl, lambdaFilter,
                                   includePeaksInRange, includeOrderZero, alreadyDonePeaks);
    } else {
      predictOffsets(0, offsets1, maxOrder, runNumber, goniometer, hkl, lambdaFilter, includePeaksInRange,
                     includeOrderZero, alreadyDonePeaks);
      predictOffsets(1, offsets2, maxOrder, runNumber, goniometer, hkl, lambdaFilter, includePeaksInRange, notOrderZero,
                     alreadyDonePeaks);
      predictOffsets(2, offsets3, maxOrder, runNumber, goniometer, hkl, lambdaFilter, includePeaksInRange, notOrderZero,
                     alreadyDonePeaks);
    }
  }
  // Sort peaks by run number so that peaks with equal goniometer matrices are
  // adjacent
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.emplace_back("RunNumber", true);
  auto isPeaksWorkspace = std::dynamic_pointer_cast<PeaksWorkspace>(outPeaks);
  if (isPeaksWorkspace)
    criteria.emplace_back("BankName", true);
  criteria.emplace_back("h", true);
  criteria.emplace_back("k", true);
  criteria.emplace_back("l", true);
  outPeaks->sort(criteria);

  for (int i = 0; i < static_cast<int>(outPeaks->getNumberPeaks()); ++i) {
    outPeaks->getPeak(i).setPeakNumber(i);
  }
  setProperty("SatellitePeaks", outPeaks);
}

void PredictSatellitePeaks::exec_peaks() {

  V3D offsets1 = getOffsetVector(ModulationProperties::ModVector1);
  V3D offsets2 = getOffsetVector(ModulationProperties::ModVector2);
  V3D offsets3 = getOffsetVector(ModulationProperties::ModVector3);
  int maxOrder = getProperty(ModulationProperties::MaxOrder);
  bool crossTerms = getProperty(ModulationProperties::CrossTerms);

  API::Sample sample = Peaks->mutableSample();

  auto lattice = std::make_unique<OrientedLattice>(sample.getOrientedLattice());

  bool fromUB = getProperty("GetModVectorsFromUB");
  if (fromUB) {
    offsets1 = lattice->getModVec(0);
    offsets2 = lattice->getModVec(1);
    offsets3 = lattice->getModVec(2);
    if (maxOrder == 0)
      maxOrder = lattice->getMaxOrder();
    crossTerms = lattice->getCrossTerm();
  } else {
    lattice->setModVec1(offsets1);
    lattice->setModVec2(offsets2);
    lattice->setModVec3(offsets3);
    lattice->setMaxOrder(maxOrder);
    lattice->setCrossTerm(crossTerms);
  }

  bool includePeaksInRange = false;
  bool includeOrderZero = getProperty("IncludeIntegerHKL");
  // boolean for only including order zero once
  bool notOrderZero = false;

  if (Peaks->getNumberPeaks() <= 0) {
    g_log.error() << "There are No peaks in the input PeaksWorkspace\n";
    return;
  }

  outPeaks = std::dynamic_pointer_cast<IPeaksWorkspace>(WorkspaceFactory::Instance().createPeaks(Peaks->id()));
  outPeaks->copyExperimentInfoFrom(Peaks.get());
  outPeaks->mutableSample().setOrientedLattice(std::move(lattice));

  vector<vector<int>> alreadyDonePeaks;
  HKLFilterWavelength lambdaFilter(DblMatrix(3, 3, true), 0.1, 100.);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset1", offsets1, true);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset2", offsets2, true);
  outPeaks->mutableRun().addProperty<std::vector<double>>("Offset3", offsets3, true);

  for (int i = 0; i < static_cast<int>(Peaks->getNumberPeaks()); ++i) {

    const Kernel::Matrix<double> peakGoniometerMatrix = Peaks->getPeak(i).getGoniometerMatrix();

    int runNumber = Peaks->getPeak(i).getRunNumber();

    V3D hkl = Peaks->getPeak(i).getHKL();

    if (crossTerms) {
      predictOffsetsWithCrossTerms(offsets1, offsets2, offsets3, maxOrder, runNumber, peakGoniometerMatrix, hkl,
                                   lambdaFilter, includePeaksInRange, includeOrderZero, alreadyDonePeaks);
    } else {
      predictOffsets(0, offsets1, maxOrder, runNumber, peakGoniometerMatrix, hkl, lambdaFilter, includePeaksInRange,
                     includeOrderZero, alreadyDonePeaks);

      predictOffsets(1, offsets2, maxOrder, runNumber, peakGoniometerMatrix, hkl, lambdaFilter, includePeaksInRange,
                     notOrderZero, alreadyDonePeaks);

      predictOffsets(2, offsets3, maxOrder, runNumber, peakGoniometerMatrix, hkl, lambdaFilter, includePeaksInRange,
                     notOrderZero, alreadyDonePeaks);
    }
  }
  // Sort peaks by run number so that peaks with equal goniometer matrices are
  // adjacent
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.emplace_back("RunNumber", true);

  workspace_type_enum const workspace_type = determineWorkspaceType(Peaks);
  if (workspace_type == workspace_type_enum::regular_peaks) {
    criteria.emplace_back("BankName", true);
  }

  criteria.emplace_back("h", true);
  criteria.emplace_back("k", true);
  criteria.emplace_back("l", true);
  outPeaks->sort(criteria);

  for (int i = 0; i < static_cast<int>(outPeaks->getNumberPeaks()); ++i) {
    outPeaks->getPeak(i).setPeakNumber(i);
  }

  setProperty("SatellitePeaks", outPeaks);
}

PredictSatellitePeaks::workspace_type_enum
PredictSatellitePeaks::determineWorkspaceType(API::IPeaksWorkspace_sptr const &iPeaksWorkspace) const {
  if (std::dynamic_pointer_cast<PeaksWorkspace>(iPeaksWorkspace) != nullptr) {
    return workspace_type_enum::regular_peaks;
  }

  else if (std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(iPeaksWorkspace) != nullptr) {
    return workspace_type_enum::lean_elastic_peaks;
  }

  else {
    return workspace_type_enum::invalid;
  }
}

void PredictSatellitePeaks::predictOffsets(const int indexModulatedVector, const V3D &offsets, const int maxOrder,
                                           const int runNumber, const Kernel::Matrix<double> &goniometer,
                                           const V3D &hkl, const HKLFilterWavelength &lambdaFilter,
                                           const bool includePeaksInRange, const bool includeOrderZero,
                                           vector<vector<int>> &alreadyDonePeaks) {
  if (offsets == V3D(0, 0, 0) && !includeOrderZero)
    return;
  for (int order = -maxOrder; order <= maxOrder; order++) {
    if (order == 0 && !includeOrderZero)
      continue; // exclude order 0
    V3D satelliteHKL(hkl);

    satelliteHKL += offsets * order;
    if (!lambdaFilter.isAllowed(satelliteHKL) && includePeaksInRange)
      continue;

    std::shared_ptr<IPeak> satellite_iPeak = createPeakForOutputWorkspace(goniometer, satelliteHKL);

    V3D mnp;
    mnp[indexModulatedVector] = order;

    addPeakToOutputWorkspace(satellite_iPeak, goniometer, hkl, satelliteHKL, runNumber, alreadyDonePeaks, mnp);
  }
}

void PredictSatellitePeaks::predictOffsetsWithCrossTerms(V3D offsets1, V3D offsets2, V3D offsets3, const int maxOrder,
                                                         const int runNumber,
                                                         Kernel::Matrix<double> const &peakGoniometerMatrix, V3D &hkl,
                                                         const HKLFilterWavelength &lambdaFilter,
                                                         const bool includePeaksInRange, const bool includeOrderZero,
                                                         vector<vector<int>> &alreadyDonePeaks) {
  if (offsets1 == V3D(0, 0, 0) && offsets2 == V3D(0, 0, 0) && offsets3 == V3D(0, 0, 0) && !includeOrderZero)
    return;
  DblMatrix offsetsMat(3, 3);
  offsetsMat.setColumn(0, offsets1);
  offsetsMat.setColumn(1, offsets2);
  offsetsMat.setColumn(2, offsets3);
  int maxOrder1 = maxOrder;
  if (offsets1 == V3D(0, 0, 0))
    maxOrder1 = 0;
  int maxOrder2 = maxOrder;
  if (offsets2 == V3D(0, 0, 0))
    maxOrder2 = 0;
  int maxOrder3 = maxOrder;
  if (offsets3 == V3D(0, 0, 0))
    maxOrder3 = 0;
  for (int m = -maxOrder1; m <= maxOrder1; m++)
    for (int n = -maxOrder2; n <= maxOrder2; n++)
      for (int p = -maxOrder3; p <= maxOrder3; p++) {
        if (m == 0 && n == 0 && p == 0 && !includeOrderZero)
          continue; // exclude 0,0,0
        V3D satelliteHKL(hkl);
        V3D mnp = V3D(m, n, p);
        satelliteHKL -= offsetsMat * mnp;
        if (!lambdaFilter.isAllowed(satelliteHKL) && includePeaksInRange)
          continue;

        std::shared_ptr<IPeak> satellite_iPeak = createPeakForOutputWorkspace(peakGoniometerMatrix, satelliteHKL);

        addPeakToOutputWorkspace(satellite_iPeak, peakGoniometerMatrix, hkl, satelliteHKL, runNumber, alreadyDonePeaks,
                                 mnp);
      }
}

std::shared_ptr<Geometry::IPeak>
PredictSatellitePeaks::createPeakForOutputWorkspace(const Kernel::Matrix<double> &peakGoniometerMatrix,
                                                    const Kernel::V3D &satelliteHKL) {
  workspace_type_enum workspace_type = determineWorkspaceType(Peaks);

  const Kernel::DblMatrix &UB = Peaks->sample().getOrientedLattice().getUB();
  if (workspace_type == workspace_type_enum::regular_peaks) {
    Kernel::V3D const Qs = peakGoniometerMatrix * UB * satelliteHKL * 2.0 * M_PI * m_qConventionFactor;

    // Check if Q is non-physical
    if (Qs.Z() * m_qConventionFactor <= 0)
      return nullptr;

    std::shared_ptr<IPeak> satellite_iPeak = Peaks->createPeak(Qs, 1);

    return satellite_iPeak;
  }

  else if (workspace_type == workspace_type_enum::lean_elastic_peaks) {
    Kernel::V3D const Qs = UB * satelliteHKL * 2.0 * M_PI * m_qConventionFactor;

    std::shared_ptr<IPeak> satellite_iPeak = Peaks->createPeakQSample(Qs);

    return satellite_iPeak;
  }

  else
    return nullptr;
}

void PredictSatellitePeaks::addPeakToOutputWorkspace(const std::shared_ptr<IPeak> &satellite_iPeak,
                                                     const Kernel::Matrix<double> &peak_goniometer_matrix,
                                                     const Kernel::V3D &hkl, const Kernel::V3D &satelliteHKL,
                                                     const int runNumber,
                                                     std::vector<std::vector<int>> &alreadyDonePeaks,
                                                     const Kernel::V3D &mnp) {
  if (satellite_iPeak == nullptr)
    return;

  const workspace_type_enum workspace_type = determineWorkspaceType(Peaks);

  if (workspace_type == workspace_type_enum::regular_peaks) {
    const Geometry::InstrumentRayTracer tracer(Peaks->getInstrument());

    const std::shared_ptr<Peak> peak = std::dynamic_pointer_cast<Peak>(satellite_iPeak);

    if (!peak->findDetector(tracer))
      return;
  }

  const std::vector<int> savPk{runNumber, boost::math::iround(1000.0 * satelliteHKL[0]),
                               boost::math::iround(1000.0 * satelliteHKL[1]),
                               boost::math::iround(1000.0 * satelliteHKL[2])};

  const bool foundPeak = binary_search(alreadyDonePeaks.begin(), alreadyDonePeaks.end(), savPk);

  if (!foundPeak) {
    alreadyDonePeaks.emplace_back(savPk);
  }

  else
    return;

  satellite_iPeak->setGoniometerMatrix(peak_goniometer_matrix);
  satellite_iPeak->setHKL(satelliteHKL * m_qConventionFactor);
  satellite_iPeak->setIntHKL(hkl * m_qConventionFactor);
  satellite_iPeak->setRunNumber(runNumber);
  satellite_iPeak->setIntMNP(mnp * m_qConventionFactor);

  outPeaks->addPeak(*satellite_iPeak);

  return;
}

V3D PredictSatellitePeaks::getOffsetVector(const std::string &label) {
  vector<double> offsets = getProperty(label);
  if (offsets.empty()) {
    offsets.emplace_back(0.0);
    offsets.emplace_back(0.0);
    offsets.emplace_back(0.0);
  }
  V3D offsets1 = V3D(offsets[0], offsets[1], offsets[2]);
  return offsets1;
}

} // namespace Crystal
} // namespace Mantid
