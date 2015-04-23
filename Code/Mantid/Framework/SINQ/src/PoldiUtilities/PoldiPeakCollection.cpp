#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/LogManager.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "boost/format.hpp"
#include "boost/algorithm/string/join.hpp"

#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

namespace Mantid {
namespace Poldi {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

PoldiPeakCollection::PoldiPeakCollection(IntensityType intensityType)
    : m_peaks(), m_intensityType(intensityType), m_profileFunctionName(),
      m_pointGroup() {}

PoldiPeakCollection::PoldiPeakCollection(const TableWorkspace_sptr &workspace)
    : m_peaks(), m_intensityType(Maximum), m_profileFunctionName(),
      m_pointGroup() {
  if (workspace) {
    constructFromTableWorkspace(workspace);
  }
}

PoldiPeakCollection::PoldiPeakCollection(
    const Geometry::CrystalStructure_sptr &crystalStructure, double dMin,
    double dMax)
    : m_peaks(), m_intensityType(Integral), m_profileFunctionName(),
      m_pointGroup() {
  if (!crystalStructure) {
    throw std::invalid_argument(
        "Cannot create PoldiPeakCollection from invalid CrystalStructure.");
  }

  m_pointGroup =
      pointGroupFromString(pointGroupToString(crystalStructure->pointGroup()));

  std::vector<V3D> uniqueHKL = crystalStructure->getUniqueHKLs(
      dMin, dMax, Geometry::CrystalStructure::UseStructureFactor);
  std::vector<double> dValues = crystalStructure->getDValues(uniqueHKL);
  std::vector<double> structureFactors =
      crystalStructure->getFSquared(uniqueHKL);

  setPeaks(uniqueHKL, dValues, structureFactors);
}

PoldiPeakCollection_sptr PoldiPeakCollection::clone() {
  PoldiPeakCollection_sptr clone =
      boost::make_shared<PoldiPeakCollection>(m_intensityType);
  clone->setProfileFunctionName(m_profileFunctionName);

  for (size_t i = 0; i < m_peaks.size(); ++i) {
    clone->addPeak(m_peaks[i]->clone());
  }

  return clone;
}

size_t PoldiPeakCollection::peakCount() const { return m_peaks.size(); }

void PoldiPeakCollection::addPeak(const PoldiPeak_sptr &newPeak) {
  m_peaks.push_back(newPeak);
}

PoldiPeak_sptr PoldiPeakCollection::peak(size_t index) const {
  if (index >= m_peaks.size()) {
    throw std::range_error("Peak access index out of range.");
  }

  return m_peaks[index];
}

const std::vector<PoldiPeak_sptr> &PoldiPeakCollection::peaks() const {
  return m_peaks;
}

PoldiPeakCollection::IntensityType PoldiPeakCollection::intensityType() const {
  return m_intensityType;
}

void
PoldiPeakCollection::setProfileFunctionName(std::string newProfileFunction) {
  m_profileFunctionName = newProfileFunction;
}

std::string PoldiPeakCollection::getProfileFunctionName() const {
  return m_profileFunctionName;
}

bool PoldiPeakCollection::hasProfileFunctionName() const {
  return !m_profileFunctionName.empty();
}

void PoldiPeakCollection::setPointGroup(const PointGroup_sptr &pointGroup) {
  m_pointGroup = pointGroup;
}

PointGroup_sptr PoldiPeakCollection::pointGroup() const { return m_pointGroup; }

TableWorkspace_sptr PoldiPeakCollection::asTableWorkspace() {
  TableWorkspace_sptr peaks = boost::dynamic_pointer_cast<TableWorkspace>(
      WorkspaceFactory::Instance().createTable());

  prepareTable(peaks);
  dataToTableLog(peaks);
  peaksToTable(peaks);

  return peaks;
}

void PoldiPeakCollection::prepareTable(const TableWorkspace_sptr &table) {
  table->addColumn("str", "HKL");
  table->addColumn("str", "d");
  table->addColumn("str", "Q");
  table->addColumn("str", "Intensity");
  table->addColumn("str", "FWHM (rel.)");
}

void PoldiPeakCollection::dataToTableLog(const TableWorkspace_sptr &table) {
  LogManager_sptr tableLog = table->logs();
  tableLog->addProperty<std::string>("IntensityType",
                                     intensityTypeToString(m_intensityType));
  tableLog->addProperty<std::string>("ProfileFunctionName",
                                     m_profileFunctionName);
  tableLog->addProperty<std::string>("PointGroup",
                                     pointGroupToString(m_pointGroup));
}

void PoldiPeakCollection::peaksToTable(const TableWorkspace_sptr &table) {
  for (std::vector<PoldiPeak_sptr>::const_iterator peak = m_peaks.begin();
       peak != m_peaks.end(); ++peak) {
    TableRow newRow = table->appendRow();
    newRow << MillerIndicesIO::toString((*peak)->hkl())
           << UncertainValueIO::toString((*peak)->d())
           << UncertainValueIO::toString((*peak)->q())
           << UncertainValueIO::toString((*peak)->intensity())
           << UncertainValueIO::toString((*peak)->fwhm(PoldiPeak::Relative));
  }
}

void PoldiPeakCollection::constructFromTableWorkspace(
    const TableWorkspace_sptr &tableWorkspace) {
  if (checkColumns(tableWorkspace)) {
    size_t newPeakCount = tableWorkspace->rowCount();
    m_peaks.resize(newPeakCount);

    recoverDataFromLog(tableWorkspace);

    for (size_t i = 0; i < newPeakCount; ++i) {
      TableRow nextRow = tableWorkspace->getRow(i);
      std::string hklString, dString, qString, intensityString, fwhmString;
      nextRow >> hklString >> dString >> qString >> intensityString >>
          fwhmString;

      PoldiPeak_sptr peak =
          PoldiPeak::create(MillerIndicesIO::fromString(hklString),
                            UncertainValueIO::fromString(dString),
                            UncertainValueIO::fromString(intensityString),
                            UncertainValueIO::fromString(fwhmString));
      m_peaks[i] = peak;
    }
  }
}

bool
PoldiPeakCollection::checkColumns(const TableWorkspace_sptr &tableWorkspace) {
  if (tableWorkspace->columnCount() != 5) {
    return false;
  }

  std::vector<std::string> shouldNames;
  shouldNames.push_back("HKL");
  shouldNames.push_back("d");
  shouldNames.push_back("Q");
  shouldNames.push_back("Intensity");
  shouldNames.push_back("FWHM (rel.)");

  std::vector<std::string> columnNames = tableWorkspace->getColumnNames();

  return columnNames == shouldNames;
}

void PoldiPeakCollection::setPeaks(const std::vector<V3D> &hkls,
                                   const std::vector<double> &dValues,
                                   const std::vector<double> &fSquared) {
  if (hkls.size() != dValues.size()) {
    throw std::invalid_argument(
        "hkl-vector and d-vector do not have the same length.");
  }

  if (!m_pointGroup) {
    throw std::runtime_error("Cannot set peaks without point group.");
  }

  m_peaks.clear();

  for (size_t i = 0; i < hkls.size(); ++i) {
    double multiplicity =
        static_cast<double>(m_pointGroup->getEquivalents(hkls[i]).size());
    addPeak(PoldiPeak::create(
        MillerIndices(hkls[i]), UncertainValue(dValues[i]),
        UncertainValue(multiplicity * fSquared[i]), UncertainValue(0.0)));
  }
}

void PoldiPeakCollection::recoverDataFromLog(
    const TableWorkspace_sptr &tableWorkspace) {
  LogManager_sptr tableLog = tableWorkspace->logs();

  m_intensityType = intensityTypeFromString(getIntensityTypeFromLog(tableLog));
  m_profileFunctionName = getProfileFunctionNameFromLog(tableLog);
  m_pointGroup = pointGroupFromString(getPointGroupStringFromLog(tableLog));
}

std::string
PoldiPeakCollection::getIntensityTypeFromLog(const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "IntensityType");
}

std::string PoldiPeakCollection::getProfileFunctionNameFromLog(
    const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "ProfileFunctionName");
}

std::string PoldiPeakCollection::getPointGroupStringFromLog(
    const LogManager_sptr &tableLog) {
  return getStringValueFromLog(tableLog, "PointGroup");
}

std::string
PoldiPeakCollection::getStringValueFromLog(const LogManager_sptr &logManager,
                                           std::string valueName) {
  if (logManager->hasProperty(valueName)) {
    return logManager->getPropertyValueAsType<std::string>(valueName);
  }

  return "";
}

std::string PoldiPeakCollection::intensityTypeToString(
    PoldiPeakCollection::IntensityType type) const {
  switch (type) {
  case Maximum:
    return "Maximum";
  case Integral:
    return "Integral";
  }

  throw std::runtime_error("Unkown intensity type can not be processed.");
}

PoldiPeakCollection::IntensityType
PoldiPeakCollection::intensityTypeFromString(std::string typeString) const {
  std::string lowerCaseType(typeString);
  std::transform(lowerCaseType.begin(), lowerCaseType.end(),
                 lowerCaseType.begin(), ::tolower);

  if (lowerCaseType == "integral") {
    return Integral;
  }

  return Maximum;
}

std::string PoldiPeakCollection::pointGroupToString(
    const PointGroup_sptr &pointGroup) const {
  if (pointGroup) {
    return pointGroup->getSymbol();
  }

  return "";
}

PointGroup_sptr PoldiPeakCollection::pointGroupFromString(
    const std::string &pointGroupString) const {
  if (PointGroupFactory::Instance().isSubscribed(pointGroupString)) {
    return PointGroupFactory::Instance().createPointGroup(pointGroupString);
  }

  return PointGroup_sptr();
}
}
}
