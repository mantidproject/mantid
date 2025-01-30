// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidKernel/V3D.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {

/** PoldiPeakCollection :
 *
  PoldiPeakCollection stores PoldiPeaks and acts as a bridge
  to TableWorkspace

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2014
*/

class PoldiPeakCollection;

using PoldiPeakCollection_sptr = std::shared_ptr<PoldiPeakCollection>;

class MANTID_SINQ_DLL PoldiPeakCollection {
public:
  enum IntensityType { Maximum, Integral };

  PoldiPeakCollection(IntensityType intensityType = Maximum);
  PoldiPeakCollection(const DataObjects::TableWorkspace_sptr &workspace);
  PoldiPeakCollection(const Geometry::CrystalStructure &crystalStructure, double dMin, double dMax);

  virtual ~PoldiPeakCollection() = default;

  PoldiPeakCollection_sptr clone();

  size_t peakCount() const;

  void addPeak(const PoldiPeak_sptr &newPeak);
  PoldiPeak_sptr peak(size_t index) const;
  const std::vector<PoldiPeak_sptr> &peaks() const;

  IntensityType intensityType() const;

  void setProfileFunctionName(std::string newProfileFunction);
  const std::string &getProfileFunctionName() const;
  bool hasProfileFunctionName() const;

  void setPointGroup(const Geometry::PointGroup_sptr &pointGroup);
  Geometry::PointGroup_sptr pointGroup() const;

  void setUnitCell(const Geometry::UnitCell &unitCell);
  Geometry::UnitCell unitCell() const;

  DataObjects::TableWorkspace_sptr asTableWorkspace();

protected:
  void prepareTable(const DataObjects::TableWorkspace_sptr &table);
  void dataToTableLog(const DataObjects::TableWorkspace_sptr &table);
  void peaksToTable(const DataObjects::TableWorkspace_sptr &table);

  void constructFromTableWorkspace(const DataObjects::TableWorkspace_sptr &tableWorkspace);
  bool checkColumns(const DataObjects::TableWorkspace_sptr &tableWorkspace);

  void recoverDataFromLog(const DataObjects::TableWorkspace_sptr &TableWorkspace);
  void setPeaks(const std::vector<Kernel::V3D> &hkls, const std::vector<double> &dValues,
                const std::vector<double> &fSquared);

  std::string getIntensityTypeFromLog(const API::LogManager_sptr &tableLog);
  std::string getProfileFunctionNameFromLog(const API::LogManager_sptr &tableLog);
  std::string getPointGroupStringFromLog(const API::LogManager_sptr &tableLog);
  std::string getUnitCellStringFromLog(const API::LogManager_sptr &tableLog);

  std::string getStringValueFromLog(const API::LogManager_sptr &logManager, const std::string &valueName);

  std::string intensityTypeToString(IntensityType type) const;
  IntensityType intensityTypeFromString(std::string typeString) const;

  std::string pointGroupToString(const Geometry::PointGroup_sptr &pointGroup) const;
  Geometry::PointGroup_sptr pointGroupFromString(const std::string &pointGroupString) const;

  Geometry::UnitCell unitCellFromString(const std::string &unitCellString) const;

  std::vector<PoldiPeak_sptr> m_peaks;
  IntensityType m_intensityType;
  std::string m_profileFunctionName;

  Geometry::PointGroup_sptr m_pointGroup;
  Geometry::UnitCell m_unitCell;
};
} // namespace Poldi
} // namespace Mantid
