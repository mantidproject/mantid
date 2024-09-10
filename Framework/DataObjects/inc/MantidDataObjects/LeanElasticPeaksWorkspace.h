// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/PeakColumn.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"

// IsamplePosition should be IsampleOrientation
namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel {
class Logger;
}

namespace DataObjects {
//==========================================================================================
/** @class Mantid::DataObjects::LeanElasticPeaksWorkspace

   The class LeanElasticPeaksWorkspace stores information about a set of SCD
   lean peaks.

    @author Ruth Mikkelson, SNS ORNL
    @date 3/10/2010
 */
class MANTID_DATAOBJECTS_DLL LeanElasticPeaksWorkspace : public Mantid::API::IPeaksWorkspace {
public:
  using ColumnAndDirection = std::pair<std::string, bool>;

public:
  const std::string id() const override { return "LeanElasticPeaksWorkspace"; }

  LeanElasticPeaksWorkspace();
  LeanElasticPeaksWorkspace &operator=(const LeanElasticPeaksWorkspace &other) = delete;

  /** Get access to shared pointer containing workspace porperties. This
   function is there to provide common interface of iTableWorkspace
    * Despite it is non-constant method, one should be very carefull using it to
   change the log values when cloning of a table workspace can occur
      as the changes may depend on the order of PeakWorkspace cloning & changes
   applyed through this pointer.
    * See PeakWorkspaceTest (test_getSetLogAccess) -- for example of this
   behaviour.
    * Use mutableRun interface to change log values rather then this method.
   **/
  API::LogManager_sptr logs() override;
  API::LogManager_const_sptr getLogs() const override;

  /// Returns a clone of the workspace
  std::unique_ptr<LeanElasticPeaksWorkspace> clone() const {
    return std::unique_ptr<LeanElasticPeaksWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<LeanElasticPeaksWorkspace> cloneEmpty() const {
    return std::unique_ptr<LeanElasticPeaksWorkspace>(doCloneEmpty());
  }

  void appendFile(std::string filename, Geometry::Instrument_sptr inst);

  /** @return true because this type of the workspace needs custom sorting calls
   */
  bool customSort() const override { return true; }

  void sort(std::vector<ColumnAndDirection> &criteria) override;

  int getNumberPeaks() const override;
  std::string getConvention() const override;
  void removePeak(int peakNum) override;
  void removePeaks(std::vector<int> badPeaks) override;
  void addPeak(const Geometry::IPeak &peak) override;
  /// Move a peak object into this peaks workspace
  void addPeak(LeanElasticPeak &&peak);
  void addPeak(const Kernel::V3D &position, const Kernel::SpecialCoordinateSystem &frame) override;
  LeanElasticPeak &getPeak(int peakNum) override;
  const LeanElasticPeak &getPeak(int peakNum) const override;

  std::unique_ptr<Geometry::IPeak> createPeak(const Kernel::V3D &QLabFrame,
                                              std::optional<double> detectorDistance = std::nullopt) const override;

  std::unique_ptr<Geometry::IPeak> createPeak(const Kernel::V3D &Position,
                                              const Kernel::SpecialCoordinateSystem &frame) const override;

  std::unique_ptr<Geometry::IPeak> createPeakQSample(const Kernel::V3D &position) const override;

  std::vector<std::pair<std::string, std::string>> peakInfo(const Kernel::V3D &qFrame, bool labCoords) const override;

  std::unique_ptr<Geometry::IPeak> createPeakHKL(const Kernel::V3D &HKL) const override;

  std::unique_ptr<Geometry::IPeak> createPeak() const override;

  int peakInfoNumber(const Kernel::V3D &qFrame, bool labCoords) const override;

  std::vector<LeanElasticPeak> &getPeaks();
  const std::vector<LeanElasticPeak> &getPeaks() const;
  bool hasIntegratedPeaks() const override;
  size_t getMemorySize() const override;

  /// Creates a new TableWorkspace giving the IDs of the detectors that
  /// contribute to the
  /// peaks within the workspace
  API::ITableWorkspace_sptr createDetectorTable() const override;

  /// Set the special coordinate system.
  void setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordinateSystem) override;

  /// Get the special coordinate system.
  Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const override;

  // ====================================== ITableWorkspace Methods
  // ==================================
  /// Number of columns in the workspace.
  size_t columnCount() const override { return static_cast<int>(m_columns.size()); }

  /// Number of rows in the workspace.
  size_t rowCount() const override { return getNumberPeaks(); }

  /// Gets the shared pointer to a column by name.
  std::shared_ptr<Mantid::API::Column> getColumn(const std::string &name) override {
    return getColumn(getColumnIndex(name));
  }

  /// Gets the shared pointer to a column by name.
  std::shared_ptr<const Mantid::API::Column> getColumn(const std::string &name) const override {
    return getColumn(getColumnIndex(name));
  }

  /// @return the index of the column with the given name.
  virtual size_t getColumnIndex(const std::string &name) const;

  /// Gets the shared pointer to a column by index.
  std::shared_ptr<Mantid::API::Column> getColumn(size_t index) override;

  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  API::Column_const_sptr getColumn(size_t index) const override;
  // ====================================== End ITableWorkspace Methods
  // ==================================

  //---------------------------------------------------------------------------------------------
  /// Returns a vector of all column names.
  std::vector<std::string> getColumnNames() const override { return this->m_columnNames; }
  /// This is always threadsafe
  bool threadSafe() const override { return true; }

  // --- Nexus Methods ---
  // Save to Nexus
  void saveNexus(::NeXus::File *file) const override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  LeanElasticPeaksWorkspace(const LeanElasticPeaksWorkspace &other);

private:
  LeanElasticPeaksWorkspace *doClone() const override { return new LeanElasticPeaksWorkspace(*this); }
  LeanElasticPeaksWorkspace *doCloneEmpty() const override { return new LeanElasticPeaksWorkspace(); }
  ITableWorkspace *doCloneColumns(const std::vector<std::string> &colNames) const override;

  /// Initialize the table structure
  void initColumns();
  /// Adds a new PeakColumn of the given type
  void addPeakColumn(const std::string &name);

  // ====================================== ITableWorkspace Methods
  // ==================================

  // ===== Methods that are not implemented (read-only table) ==========
  API::Column_sptr addColumn(const std::string & /*type*/, const std::string & /*name*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "LeanElasticPeaksWorkspace structure is read-only. Cannot add column.");
  }

  bool addColumns(const std::string & /*type*/, const std::string & /*name*/, size_t /*n*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace structure is read-only. Cannot add "
                                                         "columns.");
  }

  void removeColumn(const std::string & /*name*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "LeanElasticPeaksWorkspace structure is read-only. Cannot remove "
        "column.");
  }

  void setRowCount(size_t /*count*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "LeanElasticPeaksWorkspace structure is read-only. Cannot setRowCount");
  }

  size_t insertRow(size_t /*index*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "LeanElasticPeaksWorkspace structure is read-only. Cannot insertRow");
  }

  void removeRow(size_t /*index*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "LeanElasticPeaksWorkspace structure is read-only. Cannot removeRow.");
  }

  /// find method to get the index of integer cell value in a table workspace
  void find(size_t /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  double cell value in a table workspace
  void find(double /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  float cell value in a table workspace
  void find(float /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  API::Boolean value cell in a table
  /// workspace
  void find(API::Boolean /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of cellstd::string  value in a table
  /// workspace
  void find(const std::string & /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  Mantid::Kernel::V3D cell value in a table
  /// workspace
  void find(const Mantid::Kernel::V3D & /*value*/, size_t & /*row*/, size_t /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace::find() not implemented.");
  }

  // ====================================== End ITableWorkspace Methods
  // ==================================

  /** Vector of Peak contained within. */
  std::vector<LeanElasticPeak> m_peaks;

  /** Column shared pointers. */
  std::vector<std::shared_ptr<Mantid::DataObjects::PeakColumn<LeanElasticPeak>>> m_columns;

  /** Column names */
  std::vector<std::string> m_columnNames;

  /// Coordinates
  Kernel::SpecialCoordinateSystem m_coordSystem;
};

/// Typedef for a shared pointer to a peaks workspace.
using LeanElasticPeaksWorkspace_sptr = std::shared_ptr<LeanElasticPeaksWorkspace>;

/// Typedef for a shared pointer to a const peaks workspace.
using LeanElasticPeaksWorkspace_const_sptr = std::shared_ptr<const LeanElasticPeaksWorkspace>;
} // namespace DataObjects
} // namespace Mantid
