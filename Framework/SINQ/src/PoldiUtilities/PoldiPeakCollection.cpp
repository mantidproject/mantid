// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/LogManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

#include "MantidGeometry/Crystal/ReflectionGenerator.h"

#include "boost/algorithm/string/join.hpp"
#include "boost/format.hpp"

#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

namespace Mantid::Poldi {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

PoldiPeakCollection::PoldiPeakCollection(IntensityType intensityType)
    : m_peaks(), m_intensityType(intensityType), m_profileFunctionName(),
      m_pointGroup(PointGroupFactory::Instance().createPointGroup("1")), m_unitCell() {}

PoldiPeakCollection::PoldiPeakCollection(const TableWorkspace_sptr &workspace)
    : m_peaks(), m_intensityType(Maximum), m_profileFunctionName(),
      m_pointGroup(PointGroupFactory::Instance().createPointGroup("1")), m_unitCell() {
  if (workspace) {
    constructFromTableWorkspace(workspace);
  }
}

PoldiPeakCollection::PoldiPeakCollection(const CrystalStructure &crystalStructure, double dMin, double dMax)
    : m_peaks(), m_intensityType(Integral), m_profileFunctionName(),
      m_pointGroup(PointGroupFactory::Instance().createPointGroupFromSpaceGroup(crystalStructure.spaceGroup())),
      m_unitCell(crystalStructure.cell()) {

  ReflectionGenerator generator(crystalStructure, ReflectionConditionFilter::StructureFactor);

  std::vector<V3D> hkls = generator.getUniqueHKLs(dMin, dMax);
  std::vector<double> dValues = generator.getDValues(hkls);
  std::vector<double> structureFactors = generator.getFsSquared(hkls);

  setPeaks(hkls, dValues, structureFactors);
}

PoldiPeakCollection_sptr PoldiPeakCollection::clonePeakCollection() {
  PoldiPeakCollection_sptr clone = std::make_shared<PoldiPeakCollection>(m_intensityType);
  clone->setProfileFunctionName(m_profileFunctionName);
  clone->setPointGroup(m_pointGroup);
  clone->setUnitCell(m_unitCell);

  for (const auto &peak : m_peaks) {
    clone->addPeak(peak->clonePeak());
  }

  return clone;
}

size_t PoldiPeakCollection::peakCount() const { return m_peaks.size(); }

void PoldiPeakCollection::addPeak(const PoldiPeak_sptr &newPeak) { m_peaks.emplace_back(newPeak); }

PoldiPeak_sptr PoldiPeakCollection::peak(size_t index) const {
  if (index >= m_peaks.size()) {
    throw std::range_error("Peak access index out of range.");
  }

  return m_peaks[index];
}

const std::vector<PoldiPeak_sptr> &PoldiPeakCollection::peaks() const { return m_peaks; }

PoldiPeakCollection::IntensityType PoldiPeakCollection::intensityType() const { return m_intensityType; }

void PoldiPeakCollection::setProfileFunctionName(std::string newProfileFunction) {
  m_profileFunctionName = std::move(newProfileFunction);
}

const std::string &PoldiPeakCollection::getProfileFunctionName() const { return m_profileFunctionName; }

bool PoldiPeakCollection::hasProfileFunctionName() const { return !m_profileFunctionName.empty(); }

void PoldiPeakCollection::setPointGroup(const PointGroup_sptr &pointGroup) {
  if (!pointGroup) {
    throw std::invalid_argument("Cannot assign null-pointer to pointgroup in PoldiPeakCollection.");
  }

  m_pointGroup = PointGroupFactory::Instance().createPointGroup(pointGroup->getSymbol());
}

PointGroup_sptr PoldiPeakCollection::pointGroup() const { return m_pointGroup; }

void PoldiPeakCollection::setUnitCell(const UnitCell &unitCell) { m_unitCell = unitCell; }

UnitCell PoldiPeakCollection::unitCell() const { return m_unitCell; }

TableWorkspace_sptr PoldiPeakCollection::asTableWorkspace() {
  TableWorkspace_sptr peaksws = std::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());

  prepareTable(peaksws);
  dataToTableLog(peaksws);
  peaksToTable(peaksws);

  return peaksws;
}

void PoldiPeakCollection::prepareTable(const TableWorkspace_sptr &table) {
  table->addColumn("str", "HKL");
  table->addColumn("double", "d");
  table->addColumn("double", "delta d");
  table->addColumn("double", "Q");
  table->addColumn("double", "delta Q");
  table->addColumn("double", "Intensity");
  table->addColumn("double", "delta Intensity");
  table->addColumn("double", "FWHM (rel.)");
  table->addColumn("double", "delta FWHM (rel.)");
}

void PoldiPeakCollection::dataToTableLog(const TableWorkspace_sptr &table) {
  LogManager_sptr tableLog = table->logs();
  tableLog->addProperty<std::string>("IntensityType", intensityTypeToString(m_intensityType));
  tableLog->addProperty<std::string>("ProfileFunctionName", m_profileFunctionName);
  tableLog->addProperty<std::string>("PointGroup", pointGroupToString(m_pointGroup));
  tableLog->addProperty<std::string>("UnitCell", Geometry::unitCellToStr(m_unitCell));
}

void PoldiPeakCollection::peaksToTable(const TableWorkspace_sptr &table) {
  for (std::vector<PoldiPeak_sptr>::const_iterator peak = m_peaks.begin(); peak != m_peaks.end(); ++peak) {
    TableRow newRow = table->appendRow();

    newRow << MillerIndicesIO::toString((*peak)->hkl()) << (*peak)->d().value() << (*peak)->d().error()
           << (*peak)->q().value() << (*peak)->q().error() << (*peak)->intensity().value()
           << (*peak)->intensity().error() << (*peak)->fwhm(PoldiPeak::Relative).value()
           << (*peak)->fwhm(PoldiPeak::Relative).error();
  }
}

void PoldiPeakCollection::constructFromTableWorkspace(const TableWorkspace_sptr &tableWorkspace) {
  if (checkColumns(tableWorkspace)) {
    size_t newPeakCount = tableWorkspace->rowCount();
    m_peaks.resize(newPeakCount);

    recoverDataFromLog(tableWorkspace);

    for (size_t i = 0; i < newPeakCount; ++i) {
      TableRow nextRow = tableWorkspace->getRow(i);
      std::string hklString;
      double d, deltaD, q, deltaQ, intensity, deltaIntensity, fwhm, deltaFwhm;
      nextRow >> hklString >> d >> deltaD >> q >> deltaQ >> intensity >> deltaIntensity >> fwhm >> deltaFwhm;

      PoldiPeak_sptr peak =
          PoldiPeak::create(MillerIndicesIO::fromString(hklString), UncertainValue(d, deltaD),
                            UncertainValue(intensity, deltaIntensity), UncertainValue(fwhm, deltaFwhm));
      m_peaks[i] = peak;
    }
  }
}

bool PoldiPeakCollection::checkColumns(const TableWorkspace_sptr &tableWorkspace) {
  if (tableWorkspace->columnCount() != 9) {
    return false;
  }

  std::vector<std::string> shouldNames{
      "HKL", "d", "delta d", "Q", "delta Q", "Intensity", "delta Intensity", "FWHM (rel.)", "delta FWHM (rel.)"};

  std::vector<std::string> columnNames = tableWorkspace->getColumnNames();

  return columnNames == shouldNames;
}

void PoldiPeakCollection::setPeaks(const std::vector<V3D> &hkls, const std::vector<double> &dValues,
                                   const std::vector<double> &fSquared) {
  if (hkls.size() != dValues.size()) {
    throw std::invalid_argument("hkl-vector and d-vector do not have the same length.");
  }

  if (!m_pointGroup) {
    throw std::runtime_error("Cannot set peaks without point group.");
  }

  m_peaks.clear();

  for (size_t i = 0; i < hkls.size(); ++i) {
    double multiplicity = static_cast<double>(m_pointGroup->getEquivalents(hkls[i]).size());
    addPeak(PoldiPeak::create(MillerIndices(hkls[i]), UncertainValue(dValues[i]),
                              UncertainValue(multiplicity * fSquared[i]), UncertainValue(0.0)));
  }
}

void PoldiPeakCollection::recoverDataFromLog(const TableWorkspace_sptr &tableWorkspace) {
  LogManager_sptr tableLog = tableWorkspace->logs();

  m_intensityType = intensityTypeFromString(getIntensityTypeFromLog(tableLog));
  m_profileFunctionName = getProfileFunctionNameFromLog(tableLog);
  m_pointGroup = pointGroupFromString(getPointGroupStringFromLog(tableLog));
  m_unitCell = unitCellFromString(getUnitCellStringFromLog(tableLog));
}

std::string PoldiPeakCollection::getIntensityTypeFromLog(const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "IntensityType");
}

std::string PoldiPeakCollection::getProfileFunctionNameFromLog(const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "ProfileFunctionName");
}

std::string PoldiPeakCollection::getPointGroupStringFromLog(const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "PointGroup");
}

std::string PoldiPeakCollection::getUnitCellStringFromLog(const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "UnitCell");
}

std::string PoldiPeakCollection::getStringValueFromLog(const LogManager_sptr &logManager,
                                                       const std::string &valueName) {
  if (logManager->hasProperty(valueName)) {
    return logManager->getPropertyValueAsType<std::string>(valueName);
  }

  return "";
}

std::string PoldiPeakCollection::intensityTypeToString(PoldiPeakCollection::IntensityType type) const {
  switch (type) {
  case Maximum:
    return "Maximum";
  case Integral:
    return "Integral";
  }

  throw std::runtime_error("Unkown intensity type can not be processed.");
}

PoldiPeakCollection::IntensityType PoldiPeakCollection::intensityTypeFromString(std::string typeString) const {
  std::string lowerCaseType(std::move(typeString));
  std::transform(lowerCaseType.begin(), lowerCaseType.end(), lowerCaseType.begin(), ::tolower);

  if (lowerCaseType == "integral") {
    return Integral;
  }

  return Maximum;
}

std::string PoldiPeakCollection::pointGroupToString(const PointGroup_sptr &pointGroup) const {
  if (pointGroup) {
    return pointGroup->getSymbol();
  }

  return "1";
}

PointGroup_sptr PoldiPeakCollection::pointGroupFromString(const std::string &pointGroupString) const {
  if (PointGroupFactory::Instance().isSubscribed(pointGroupString)) {
    return PointGroupFactory::Instance().createPointGroup(pointGroupString);
  }

  return PointGroupFactory::Instance().createPointGroup("1");
}

UnitCell PoldiPeakCollection::unitCellFromString(const std::string &unitCellString) const {
  UnitCell cell;

  try {
    cell = strToUnitCell(unitCellString);
  } catch (const std::runtime_error &) {
    // do nothing
  }

  return cell;
}
} // namespace Mantid::Poldi
