// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

namespace Mantid {
namespace DataObjects {

#ifndef __INTEL_COMPILER // As of July 13, the packing has no effect for the
                         // Intel compiler and produces a warning
#pragma pack(push, 4)    // Ensure the structure is no larger than it needs to
#endif

//===============================================================================================
/** Templated class for a GRIDDED multi-dimensional event "box".
 * A MDGridBox contains a dense array with nd dimensions
 * of MDBoxBase'es, each being either a regular MDBox or a MDGridBox itself.
 *
 * This means that MDGridBoxes can be recursively gridded finer and finer.
 *
 * @tparam nd :: the number of dimensions that each MDLeanEvent will be
 *tracking.
 *                  an int > 0.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 7, 2010
 *
 * */
TMDE_CLASS
class MANTID_DATAOBJECTS_DLL MDGridBox : public MDBoxBase<MDE, nd> {
public:
  MDGridBox(std::shared_ptr<API::BoxController> &bc, const uint32_t depth,
            const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector);
  MDGridBox(Mantid::API::BoxController *const bc, const uint32_t depth,
            const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector);

  MDGridBox(MDBox<MDE, nd> *box);

  MDGridBox(const MDGridBox<MDE, nd> &other, Mantid::API::BoxController *const otherBC);

  ~MDGridBox() override;
  // ----------------------------- ISaveable Methods
  // ------------------------------------------------------
  /**get object responsible for saving the box to a file.
   *@return the const pointer to the object. The GridBox is not saveable at the
   *moment so it is always NULL */
  Kernel::ISaveable *getISaveable() override { return nullptr; }
  /**get const object responsible for saving the box to a file.
   *@return the const pointer the const object. The GridBox is not saveable at
   *the moment so it is always NULL */
  Kernel::ISaveable *getISaveable() const override { return nullptr; }
  /**Recursively make all underlaying boxes file-backed*/
  void setFileBacked(const uint64_t /*fileLocation*/, const size_t /*fileSize*/, const bool /*markSaved*/) override;
  void setFileBacked() override;
  void clearFileBacked(bool loadDiskBackedData) override;
  void clear() override;
  void clearDataFromMemory() override { /*it seems works on boxes only though recursive
                                  clearing makes sence, not yet implemented*/
  }
  /**Save the box at specific disk position using the class, respoinsible for
   * the file IO. */
  void saveAt(API::IBoxControllerIO *const, uint64_t /*position*/) const override { /*Not saveable */ }

  /**Load the box data of specified size from the disk location provided using
   * the class, responsible for the file IO. Overload that allows passing temporary memory */
  void loadAndAddFrom(API::IBoxControllerIO *const, uint64_t /*position*/, size_t /* Size */,
                      std::vector<coord_t> & /*memory*/) override { /*Not directly loadable */ }

  /**Load the box data of specified size from the disk location provided using
   * the class, respoinsible for the file IO. */
  void loadAndAddFrom(API::IBoxControllerIO *const, uint64_t /*position*/,
                      size_t /* Size */) override { /*Not directly loadable */ }
  void reserveMemoryForLoad(uint64_t /* Size */) override { /*Not directly loadable */ }
  //-------------------------------------------------------------------------------------------------------
  // Setters for cached values
  void setNPoints(const uint64_t &n) { nPoints = n; }
  //-------------------------------------------------------------------------------------------------------
  /** Uses the cached value of points stored in the grid box
   *  @return the total number of points (events) in this box  (in memory and
   * in file if present)     */
  uint64_t getNPoints() const override { return nPoints; }
  /// @return the amount of memory that the object's data ocupy. Currently uses
  /// cached value.
  uint64_t getTotalDataSize() const override { return nPoints; }
  /**  @return the number of points (events) this box keeps in memory. May be
   * different from total number of points for
   * file based workspaces/boxes.   Calculates recursively from child boxes */
  size_t getDataInMemorySize() const override;

  size_t getNumDims() const override;
  size_t getNumMDBoxes() const override;
  size_t getNumChildren() const override;
  /// to avoid casting (which need also the number of dimensions) method say if
  /// Node is a box. if not, it is gridbox
  bool isBox() const override { return false; }

  size_t getChildIndexFromID(size_t childId) const;
  API::IMDNode *getChild(size_t index) override;
  void setChild(size_t index, MDGridBox<MDE, nd> *newChild);

  void setChildren(const std::vector<API::IMDNode *> &otherBoxes, const size_t indexStart,
                   const size_t indexEnd) override;

  void getBoxes(std::vector<API::IMDNode *> &outBoxes, size_t maxDepth, bool leafOnly) override;
  void getBoxes(std::vector<API::IMDNode *> &outBoxes, size_t maxDepth, bool leafOnly,
                Mantid::Geometry::MDImplicitFunction *function) override;

  void getBoxes(std::vector<API::IMDNode *> &outBoxes, const std::function<bool(API::IMDNode *)> &cond) final override;

  const API::IMDNode *getBoxAtCoord(const coord_t *coords) override;

  void transformDimensions(std::vector<double> &scaling, std::vector<double> &offset) override;
  //----------------------------------------------------------------------------

  std::vector<MDE> *getEventsCopy() override;

  //----------------------------------------------------------------------------------------------------------------------
  size_t addEvent(const MDE &event) override;
  size_t addEventUnsafe(const MDE &event) override;

  /*--------------->  EVENTS from event data
   * <-------------------------------------------------------------*/
  void buildAndAddEvent(const signal_t Signal, const signal_t errorSq, const std::vector<coord_t> &point,
                        uint16_t expInfoIndex, uint16_t goniometerIndex, uint32_t detectorId) override;
  void buildAndAddEventUnsafe(const signal_t Signal, const signal_t errorSq, const std::vector<coord_t> &point,
                              uint16_t expInfoIndex, uint16_t goniometerIndex, uint32_t detectorId) override;
  size_t buildAndAddEvents(const std::vector<signal_t> &sigErrSq, const std::vector<coord_t> &Coord,
                           const std::vector<uint16_t> &expInfoIndex, const std::vector<uint16_t> &goniometerIndex,
                           const std::vector<uint32_t> &detectorId) override;
  //----------------------------------------------------------------------------------------------------------------------

  void centerpointBin(MDBin<MDE, nd> &bin, bool *fullyContained) const override;

  void generalBin(MDBin<MDE, nd> & /*bin*/, Mantid::Geometry::MDImplicitFunction & /*function*/) const override {}

  void integrateSphere(Mantid::API::CoordTransform &radiusTransform, const coord_t radiusSquared, signal_t &signal,
                       signal_t &errorSquared, const coord_t innerRadiusSquared = 0.0,
                       const bool useOnePercentBackgroundCorrection = true) const override;

  void centroidSphere(Mantid::API::CoordTransform &radiusTransform, const coord_t radiusSquared, coord_t *centroid,
                      signal_t &signal) const override;

  void integrateCylinder(Mantid::API::CoordTransform &radiusTransform, const coord_t radius, const coord_t length,
                         signal_t &signal, signal_t &errorSquared, std::vector<signal_t> &signal_fit) const override;

  void splitContents(size_t index, Kernel::ThreadScheduler *ts = nullptr);

  void splitAllIfNeeded(Kernel::ThreadScheduler *ts = nullptr) override;

  void refreshCache(Kernel::ThreadScheduler *ts = nullptr) override;

  void calculateGridCaches() override final;

  bool getIsMasked() const override;
  /// Setter for masking the box
  void mask() override;
  /// Setter for unmasking the box
  void unmask() override;
  // ======================= Testing/Debugging Methods =================
  /** For testing: get (a reference to) the vector of boxes */
  std::vector<MDBoxBase<MDE, nd> *> &getBoxes() { return m_Children; }

  //-------------------------------------------------------------------------
  /** The function used to satisfy IMDNode interface but the physical meaning is
   * unclear */
  void calculateCentroid(coord_t * /*centroid*/) const override {
    throw(std::runtime_error("This function should not be called on MDGridBox "
                             "(as its meaning for MDbox is dubious too)"));
  }
  //-------------------------------------------------------------------------
  /** The function used to satisfy IMDNode interface but the physical meaning is
   * unclear */
  void calculateCentroid(coord_t * /*centroid*/, const int /*expInfoIndex*/) const override {
    throw(std::runtime_error("This function should not be called on MDGridBox "
                             "(as its meaning for MDbox is dubious too)"));
  }
  //-------------------------------------------------------------------------
  /** The function used to satisfy IMDNode interface but the physical meaning is
   * unclear */
  coord_t *getCentroid() const override {
    throw(std::runtime_error("This function should not be called on MDGridBox "
                             "(as its meaning for MDbox is dubious too)"));
  }

public:
  /// Typedef for a shared pointer to a MDGridBox
  using sptr = std::shared_ptr<MDGridBox<MDE, nd>>;

  /// Typedef for a vector of MDBoxBase pointers
  using boxVector_t = std::vector<MDBoxBase<MDE, nd> *>;

private:
  /// Compute the index of the child box for the given event
  size_t calculateChildIndex(const MDE &event) const;

  /// Each dimension is split into this many equally-sized boxes
  size_t split[nd];
  /** Cumulative dimension splitting: split[n] = 1*split[0]*split[..]*split[n-1]
   */
  size_t splitCumul[nd];
  /// size of each sub-box (the one this GridBox can be split into) in
  /// correspondent direction
  double m_SubBoxSize[nd];

  /// How many boxes in the boxes vector? This is just to avoid boxes.size()
  /// calls.
  size_t numBoxes;

  /** 1D array of boxes contained within. These map
   * to the nd-array.     */
  std::vector<MDBoxBase<MDE, nd> *> m_Children;

  /** Length (squared) of the diagonal through every dimension = sum(
   * boxSize[i]^2 )
   * Used in some calculations like peak integration */
  coord_t diagonalSquared;

  /// Cached number of points contained (including all sub-boxes)
  size_t nPoints;

  //=================== PRIVATE METHODS =======================================

  size_t getLinearIndex(size_t *indices) const;

  size_t computeSizesFromSplit();
  void fillBoxShell(const size_t tot, const coord_t ChildInverseVolume);
  /**private default copy constructor as the only correct constructor is the one
   * with box controller */
  MDGridBox(const MDGridBox<MDE, nd> &box);
  /**Private constructor as it does not work without box controller */
  MDGridBox() = default;
  /// common part of MDGridBox contstructor;
  size_t initGridBox();
};

#ifndef __INTEL_COMPILER
#pragma pack(pop) // Return to default packing size
#endif

} // namespace DataObjects
} // namespace Mantid
