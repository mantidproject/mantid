// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKSPACE_H_
#define MANTID_DATAOBJECTS_PEAKSPACE_H_ 1

#include "MantidAPI/Column.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakColumn.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>
#include <string>
#include <utility>
#include <vector>

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
/** @class Mantid::DataObjects::PeaksWorkspace

   The class PeaksWorkspace stores information about a set of SCD peaks.

    @author Ruth Mikkelson, SNS ORNL
    @date 3/10/2010
 */
class DLLExport PeaksWorkspace : public Mantid::API::IPeaksWorkspace {
public:
  const std::string id() const override { return "PeaksWorkspace"; }

  PeaksWorkspace();
  PeaksWorkspace &operator=(const PeaksWorkspace &other) = delete;
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
  std::unique_ptr<PeaksWorkspace> clone() const {
    return std::unique_ptr<PeaksWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<PeaksWorkspace> cloneEmpty() const {
    return std::unique_ptr<PeaksWorkspace>(doCloneEmpty());
  }

  void appendFile(std::string filename, Geometry::Instrument_sptr inst);

  /** @return true because this type of the workspace needs custom sorting calls
   */
  bool customSort() const override { return true; }

  void sort(std::vector<std::pair<std::string, bool>> &criteria) override;

  int getNumberPeaks() const override;
  std::string getConvention() const override;
  void removePeak(int peakNum) override;
  void removePeaks(std::vector<int> badPeaks) override;
  void addPeak(const Geometry::IPeak &peak) override;
  /// Move a peak object into this peaks workspace
  void addPeak(Peak &&peak);
  void addPeak(const Kernel::V3D &position,
               const Kernel::SpecialCoordinateSystem &frame) override;
  Peak &getPeak(int peakNum) override;
  const Peak &getPeak(int peakNum) const override;

  Geometry::IPeak *createPeak(
      const Kernel::V3D &QLabFrame,
      boost::optional<double> detectorDistance = boost::none) const override;

  std::unique_ptr<Geometry::IPeak>
  createPeak(const Kernel::V3D &Position,
             const Kernel::SpecialCoordinateSystem &frame) const override;

  std::vector<std::pair<std::string, std::string>>
  peakInfo(const Kernel::V3D &qFrame, bool labCoords) const override;

  Peak *createPeakHKL(const Kernel::V3D &HKL) const override;

  int peakInfoNumber(const Kernel::V3D &qFrame, bool labCoords) const override;

  std::vector<Peak> &getPeaks();
  const std::vector<Peak> &getPeaks() const;
  bool hasIntegratedPeaks() const override;
  size_t getMemorySize() const override;

  /// Creates a new TableWorkspace giving the IDs of the detectors that
  /// contribute to the
  /// peaks within the workspace
  API::ITableWorkspace_sptr createDetectorTable() const override;

  /// Set the special coordinate system.
  void setCoordinateSystem(
      const Kernel::SpecialCoordinateSystem coordinateSystem) override;

  /// Get the special coordinate system.
  Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const override;

  // ====================================== ITableWorkspace Methods
  // ==================================
  /// Number of columns in the workspace.
  size_t columnCount() const override {
    return static_cast<int>(columns.size());
  }

  /// Number of rows in the workspace.
  size_t rowCount() const override { return getNumberPeaks(); }

  /// Gets the shared pointer to a column by name.
  boost::shared_ptr<Mantid::API::Column>
  getColumn(const std::string &name) override {
    return getColumn(getColumnIndex(name));
  }

  /// Gets the shared pointer to a column by name.
  boost::shared_ptr<const Mantid::API::Column>
  getColumn(const std::string &name) const override {
    return getColumn(getColumnIndex(name));
  }

  /// @return the index of the column with the given name.
  virtual size_t getColumnIndex(const std::string &name) const;

  /// Gets the shared pointer to a column by index.
  boost::shared_ptr<Mantid::API::Column> getColumn(size_t index) override;

  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  API::Column_const_sptr getColumn(size_t index) const override;
  // ====================================== End ITableWorkspace Methods
  // ==================================

  //---------------------------------------------------------------------------------------------
  /// Returns a vector of all column names.
  std::vector<std::string> getColumnNames() const override {
    return this->columnNames;
  }
  /// This is always threadsafe
  bool threadSafe() const override { return true; }

  // --- Nexus Methods ---
  // Save to Nexus
  void saveNexus(::NeXus::File *file) const;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  PeaksWorkspace(const PeaksWorkspace &other);

private:
  PeaksWorkspace *doClone() const override { return new PeaksWorkspace(*this); }
  PeaksWorkspace *doCloneEmpty() const override { return new PeaksWorkspace(); }
  ITableWorkspace *
  doCloneColumns(const std::vector<std::string> &colNames) const override;

  /// Initialize the table structure
  void initColumns();
  /// Adds a new PeakColumn of the given type
  void addPeakColumn(const std::string &name);
  /// Create a peak from a QSample position
  Peak *createPeakQSample(const Kernel::V3D &position) const;

  // ====================================== ITableWorkspace Methods
  // ==================================

  // ===== Methods that are not implemented (read-only table) ==========
  API::Column_sptr addColumn(const std::string & /*type*/,
                             const std::string & /*name*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot add column.");
  }

  bool addColumns(const std::string & /*type*/, const std::string & /*name*/,
                  size_t /*n*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot add columns.");
  }

  void removeColumn(const std::string & /*name*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot remove column.");
  }

  void setRowCount(size_t /*count*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot setRowCount");
  }

  size_t insertRow(size_t /*index*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot insertRow");
  }

  void removeRow(size_t /*index*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot removeRow.");
  }

  /// find method to get the index of integer cell value in a table workspace
  void find(size_t /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  double cell value in a table workspace
  void find(double /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  float cell value in a table workspace
  void find(float /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  API::Boolean value cell in a table
  /// workspace
  void find(API::Boolean /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of cellstd::string  value in a table
  /// workspace
  void find(std::string /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  Mantid::Kernel::V3D cell value in a table
  /// workspace
  void find(Mantid::Kernel::V3D /*value*/, size_t & /*row*/,
            const size_t & /*col*/) override {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }

  // ====================================== End ITableWorkspace Methods
  // ==================================

  /** Vector of Peak contained within. */
  std::vector<Peak> peaks;

  /** Column shared pointers. */
  std::vector<boost::shared_ptr<Mantid::DataObjects::PeakColumn>> columns;

  /** Column names */
  std::vector<std::string> columnNames;

  /// Coordinates
  Kernel::SpecialCoordinateSystem m_coordSystem;
};

/// Typedef for a shared pointer to a peaks workspace.
using PeaksWorkspace_sptr = boost::shared_ptr<PeaksWorkspace>;

/// Typedef for a shared pointer to a const peaks workspace.
using PeaksWorkspace_const_sptr = boost::shared_ptr<const PeaksWorkspace>;
} // namespace DataObjects
} // namespace Mantid
#endif
