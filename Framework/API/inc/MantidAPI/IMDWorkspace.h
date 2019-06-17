// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IMDWORKSPACE_H_
#define MANTID_API_IMDWORKSPACE_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <cstdint>
#include <vector>

namespace Mantid {

namespace API {

static const signal_t MDMaskValue = std::numeric_limits<double>::quiet_NaN();

/** Basic MD Workspace Abstract Class.
 *
 *  This defines the interface that allows one to iterate through several types
 of workspaces:
 *
 *   - The regularly gridded MDHistoWorkspace
 *   - The recursively binned MDEventWorkspace
 *   - The regular (2D) MatrixWorkspace.
 *
 @author Janik Zikovsky
 @date 04/10/2010
*/

class MANTID_API_DLL IMDWorkspace : public Workspace, public API::MDGeometry {
public:
  IMDWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);
  IMDWorkspace &operator=(const IMDWorkspace &other) = delete;

  /**
   * Holds X, Y, E for a line plot
   */
  struct LinePlot {
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
  };

  /// Returns a clone of the workspace
  std::unique_ptr<IMDWorkspace> clone() const {
    return std::unique_ptr<IMDWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<IMDWorkspace> cloneEmpty() const {
    return std::unique_ptr<IMDWorkspace>(doCloneEmpty());
  }

  /// Get the number of points associated with the workspace.
  /// For MDEvenWorkspace it is the number of events contributing into the
  /// workspace
  /// For regularly gridded workspace (MDHistoWorkspace and MatrixWorkspace), it
  /// is
  /// the number of bins.
  virtual uint64_t getNPoints() const = 0;
  /*** Get the number of events, associated with the workspace
   * For MDEvenWorkspace it is equal to the number of points
   * For regularly gridded workspace (MDHistoWorkspace and MatrixWorkspace),
   * it is the number of contributed non-zero events.
   */
  virtual uint64_t getNEvents() const = 0;

  /// Creates a new iterator pointing to the first cell in the workspace
  virtual std::vector<std::unique_ptr<IMDIterator>>
  createIterators(size_t suggestedNumCores = 1,
                  Geometry::MDImplicitFunction *function = nullptr) const = 0;

  /// Returns the (normalized) signal at a given coordinates
  virtual signal_t
  getSignalAtCoord(const coord_t *coords,
                   const Mantid::API::MDNormalization &normalization) const = 0;

  /// Returns the (normalized) signal at a given coordinates or 0 if the value
  // is masked, used for plotting
  virtual signal_t getSignalWithMaskAtCoord(
      const coord_t *coords,
      const Mantid::API::MDNormalization &normalization) const = 0;

  /// Method to generate a line plot through a MD-workspace
  virtual LinePlot getLinePlot(const Mantid::Kernel::VMD &start,
                               const Mantid::Kernel::VMD &end,
                               Mantid::API::MDNormalization normalize) const;

  std::unique_ptr<IMDIterator>
  createIterator(Geometry::MDImplicitFunction *function = nullptr) const;

  std::string getConvention() const;
  void setConvention(std::string convention);
  std::string changeQConvention();

  signal_t getSignalAtVMD(const Mantid::Kernel::VMD &coords,
                          const Mantid::API::MDNormalization &normalization =
                              Mantid::API::VolumeNormalization) const;

  signal_t
  getSignalWithMaskAtVMD(const Mantid::Kernel::VMD &coords,
                         const Mantid::API::MDNormalization &normalization =
                             Mantid::API::VolumeNormalization) const;

  /// Setter for the masking region.
  virtual void
  setMDMasking(std::unique_ptr<Geometry::MDImplicitFunction> maskingRegion) = 0;

  /// Clear existing masks
  virtual void clearMDMasking() = 0;
  ///
  virtual Kernel::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const = 0;
  /// if a workspace was filebacked, this should clear file-based status, delete
  /// file-based information and close related files.
  virtual void clearFileBacked(bool /* loadFileContentsToMemory*/) {}
  /// this is the method to build table workspace from any workspace. It does
  /// not have much sence and may be placed here erroneously
  virtual ITableWorkspace_sptr makeBoxTable(size_t /*start*/, size_t /* num*/) {
    throw Kernel::Exception::NotImplementedError(
        "This method is not generally implemented ");
  }

  // Preferred normalization to use for display
  virtual MDNormalization displayNormalization() const;

  // Preferred normalization to use for displaying histo workspaces
  virtual MDNormalization displayNormalizationHisto() const;

  // Check if this class is an instance of MDHistoWorkspace
  virtual bool isMDHistoWorkspace() const { return false; }

  // Check if this class has an oriented lattice on a sample object
  virtual bool hasOrientedLattice() const = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IMDWorkspace(const IMDWorkspace &) = default;

  void makeSinglePointWithNaN(std::vector<coord_t> &x, std::vector<signal_t> &y,
                              std::vector<signal_t> &e) const;

  const std::string toString() const override;

private:
  std::string m_convention;
  IMDWorkspace *doClone() const override = 0;
  IMDWorkspace *doCloneEmpty() const override = 0;
};

/// Shared pointer to the IMDWorkspace base class
using IMDWorkspace_sptr = boost::shared_ptr<IMDWorkspace>;
/// Shared pointer to the IMDWorkspace base class (const version)
using IMDWorkspace_const_sptr = boost::shared_ptr<const IMDWorkspace>;
} // namespace API
} // namespace Mantid
#endif // MANTID_API_IMDWORKSPACE_H_
