#ifndef MANTID_DATAOBJECTS_PEAKSPACE_H_
#define MANTID_DATAOBJECTS_PEAKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
 */
class DLLExport PeaksWorkspace : public Mantid::API::IPeaksWorkspace {
public:
  virtual const std::string id() const { return "PeaksWorkspace"; }

  PeaksWorkspace();

  PeaksWorkspace(const PeaksWorkspace &other);

  PeaksWorkspace *clone() const;

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
  API::LogManager_sptr logs();
  /**Get constant access to shared pointer containing workspace porperties;
     Copies logs into new LogManager variable
     Meaningfull only for some multithereaded methods when a thread wants to
     have its own copy of logs   */
  API::LogManager_const_sptr getLogs() const {
    return API::LogManager_const_sptr(new API::LogManager(this->run()));
  }

  virtual ~PeaksWorkspace();

  boost::shared_ptr<PeaksWorkspace> clone();

  void appendFile(std::string filename, Geometry::Instrument_sptr inst);

  /** @return true because this type of the workspace needs custom sorting calls
   */
  virtual bool customSort() const { return true; }

  void sort(std::vector<std::pair<std::string, bool>> &criteria);

  int getNumberPeaks() const;
  void removePeak(int peakNum);
  void addPeak(const API::IPeak &ipeak);
  Peak &getPeak(int peakNum);
  const Peak &getPeak(int peakNum) const;

  API::IPeak *createPeak(Kernel::V3D QFrame,
                         double detectorDistance = 1.0) const;
  std::vector<std::pair<std::string, std::string>>
  peakInfo(Kernel::V3D qFrame, bool labCoords) const;
  int peakInfoNumber(Kernel::V3D qFrame, bool labCoords) const;

  std::vector<Peak> &getPeaks();
  const std::vector<Peak> &getPeaks() const;
  virtual bool hasIntegratedPeaks() const;
  virtual size_t getMemorySize() const;

  /// Creates a new TableWorkspace giving the IDs of the detectors that
  /// contribute to the
  /// peaks within the workspace
  API::ITableWorkspace_sptr createDetectorTable() const;

  /// Set the special coordinate system.
  virtual void setCoordinateSystem(
      const Mantid::API::SpecialCoordinateSystem coordinateSystem);

  /// Get the special coordinate system.
  virtual Mantid::API::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const;

  // ====================================== ITableWorkspace Methods
  // ==================================
  /// Number of columns in the workspace.
  virtual size_t columnCount() const {
    return static_cast<int>(columns.size());
  }

  /// Number of rows in the workspace.
  virtual size_t rowCount() const { return getNumberPeaks(); }

  /// Gets the shared pointer to a column by name.
  virtual boost::shared_ptr<Mantid::API::Column>
  getColumn(const std::string &name) {
    return getColumn(getColumnIndex(name));
  }

  /// Gets the shared pointer to a column by name.
  virtual boost::shared_ptr<const Mantid::API::Column>
  getColumn(const std::string &name) const {
    return getColumn(getColumnIndex(name));
  }

  /// @return the index of the column with the given name.
  virtual size_t getColumnIndex(const std::string &name) const;

  /// Gets the shared pointer to a column by index.
  virtual boost::shared_ptr<Mantid::API::Column> getColumn(size_t index);

  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  API::Column_const_sptr getColumn(size_t index) const;
  // ====================================== End ITableWorkspace Methods
  // ==================================

  //---------------------------------------------------------------------------------------------
  /// Returns a vector of all column names.
  virtual std::vector<std::string> getColumnNames() const {
    return this->columnNames;
  }
  /// This is always threadsafe
  virtual bool threadSafe() const { return true; }

  // --- Nexus Methods ---
  // Save to Nexus
  void saveNexus(::NeXus::File *file) const;

private:
  /** Vector of Peak contained within. */
  std::vector<Peak> peaks;

  /** Column shared pointers. */
  std::vector<boost::shared_ptr<Mantid::DataObjects::PeakColumn>> columns;

  /** Column names */
  std::vector<std::string> columnNames;

  /// Initialize the table structure
  void initColumns();
  /// Adds a new PeakColumn of the given type
  void addPeakColumn(const std::string &name);

  // ====================================== ITableWorkspace Methods
  // ==================================

  // ===== Methods that are not implemented (read-only table) ==========
  virtual API::Column_sptr addColumn(const std::string & /*type*/,
                                     const std::string & /*name*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot add column.");
  }

  virtual bool addColumns(const std::string & /*type*/,
                          const std::string & /*name*/, size_t /*n*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot add columns.");
  }

  virtual void removeColumn(const std::string & /*name*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot remove column.");
  }

  virtual void setRowCount(size_t /*count*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot setRowCount");
  }

  virtual size_t insertRow(size_t /*index*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot insertRow");
  }

  virtual void removeRow(size_t /*index*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace structure is read-only. Cannot removeRow.");
  }

  /// find method to get the index of integer cell value in a table workspace
  virtual void find(size_t /*value*/, size_t & /*row*/,
                    const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  double cell value in a table workspace
  virtual void find(double /*value*/, size_t & /*row*/,
                    const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  float cell value in a table workspace
  virtual void find(float /*value*/, size_t & /*row*/, const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  API::Boolean value cell in a table
  /// workspace
  virtual void find(API::Boolean /*value*/, size_t & /*row*/,
                    const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of cellstd::string  value in a table
  /// workspace
  virtual void find(std::string /*value*/, size_t & /*row*/,
                    const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }
  /// find method to get the index of  Mantid::Kernel::V3D cell value in a table
  /// workspace
  virtual void find(Mantid::Kernel::V3D /*value*/, size_t & /*row*/,
                    const size_t & /*col*/) {
    throw Mantid::Kernel::Exception::NotImplementedError(
        "PeaksWorkspace::find() not implemented.");
  }

  // ====================================== End ITableWorkspace Methods
  // ==================================

  // adapter for logs() function, which create reference to this class itself
  // and does not allow to delete the shared pointers,
  // returned by logs() function when they go out of scope
  API::LogManager_sptr m_logCash;
};

/// Typedef for a shared pointer to a peaks workspace.
typedef boost::shared_ptr<PeaksWorkspace> PeaksWorkspace_sptr;

/// Typedef for a shared pointer to a const peaks workspace.
typedef boost::shared_ptr<const PeaksWorkspace> PeaksWorkspace_const_sptr;
}
}
#endif
