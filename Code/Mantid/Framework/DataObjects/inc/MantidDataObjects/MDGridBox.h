#ifndef MDGRIDBOX_H_
#define MDGRIDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"

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
class DLLExport MDGridBox : public MDBoxBase<MDE, nd> {
public:
  MDGridBox(boost::shared_ptr<API::BoxController> &bc, const uint32_t depth,
            const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &
                extentsVector);
  MDGridBox(Mantid::API::BoxController *const bc, const uint32_t depth,
            const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &
                extentsVector);

  MDGridBox(MDBox<MDE, nd> *box);

  MDGridBox(const MDGridBox<MDE, nd> &box,
            Mantid::API::BoxController *const otherBC);

  virtual ~MDGridBox();
  // ----------------------------- ISaveable Methods
  // ------------------------------------------------------
  /**get object responsible for saving the box to a file.
    *@return the const pointer to the object. The GridBox is not saveable at the
    *moment so it is always NULL */
  virtual Kernel::ISaveable *getISaveable() { return NULL; }
  /**get const object responsible for saving the box to a file.
    *@return the const pointer the const object. The GridBox is not saveable at
    *the moment so it is always NULL */
  virtual Kernel::ISaveable *getISaveable() const { return NULL; }
  /**Recursively make all underlaying boxes file-backed*/
  virtual void setFileBacked(const uint64_t /*fileLocation*/,
                             const size_t /*fileSize*/,
                             const bool /*markSaved*/);
  virtual void setFileBacked();
  virtual void clearFileBacked(bool loadDiskBackedData);
  void clear();
  void clearDataFromMemory() { /*it seems works on boxes only though recursive
                                  clearing makes sence, not yet implemented*/
  }
  /**Save the box at specific disk position using the class, respoinsible for
   * the file IO. */
  virtual void saveAt(API::IBoxControllerIO *const /* */,
                      uint64_t /*position*/) const { /*Not saveable */
  }
  /**Load the box data of specified size from the disk location provided using
   * the class, respoinsible for the file IO. */
  virtual void loadAndAddFrom(API::IBoxControllerIO *const /* */,
                              uint64_t /*position*/,
                              size_t /* Size */) { /*Not directly loadable */
  }
  virtual void
      reserveMemoryForLoad(uint64_t /* Size */) { /*Not directly loadable */
  }
  //-------------------------------------------------------------------------------------------------------

  /** Uses the cached value of points stored in the grid box
    *  @return the total number of points (events) in this box  (in memory and
   * in file if present)     */
  uint64_t getNPoints() const { return nPoints; }
  /// @return the amount of memory that the object's data ocupy. Currently uses
  /// cached value.
  virtual uint64_t getTotalDataSize() const { return nPoints; }
  /**  @return the number of points (events) this box keeps in memory. May be
   * different from total number of points for
     * file based workspaces/boxes.   Calculates recursively from child boxes */
  size_t getDataInMemorySize() const;

  size_t getNumDims() const;
  size_t getNumMDBoxes() const;
  size_t getNumChildren() const;
  /// to avoid casting (which need also the number of dimensions) method say if
  /// Node is a box. if not, it is gridbox
  virtual bool isBox() const { return false; }

  size_t getChildIndexFromID(size_t childId) const;
  API::IMDNode *getChild(size_t index);
  void setChild(size_t index, MDGridBox<MDE, nd> *newChild);

  void setChildren(const std::vector<API::IMDNode *> &boxes,
                   const size_t indexStart, const size_t indexEnd);

  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth,
                bool leafOnly);
  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth,
                bool leafOnly, Mantid::Geometry::MDImplicitFunction *function);

  const API::IMDNode *getBoxAtCoord(const coord_t *coords);

  void transformDimensions(std::vector<double> &scaling,
                           std::vector<double> &offset);
  //----------------------------------------------------------------------------

  std::vector<MDE> *getEventsCopy();

  //----------------------------------------------------------------------------------------------------------------------
  void addEvent(const MDE &event);
  void addEventUnsafe(const MDE &event);

  /*--------------->  EVENTS from event data
   * <-------------------------------------------------------------*/
  virtual void buildAndAddEvent(const signal_t Signal, const signal_t errorSq,
                                const std::vector<coord_t> &point,
                                uint16_t runIndex, uint32_t detectorId);
  virtual void buildAndAddEventUnsafe(const signal_t Signal,
                                      const signal_t errorSq,
                                      const std::vector<coord_t> &point,
                                      uint16_t runIndex, uint32_t detectorId);
  virtual size_t buildAndAddEvents(const std::vector<signal_t> &sigErrSq,
                                   const std::vector<coord_t> &Coord,
                                   const std::vector<uint16_t> &runIndex,
                                   const std::vector<uint32_t> &detectorId);
  //----------------------------------------------------------------------------------------------------------------------

  void centerpointBin(MDBin<MDE, nd> &bin, bool *fullyContained) const;

  void generalBin(MDBin<MDE, nd> & /*bin*/,
                  Mantid::Geometry::MDImplicitFunction & /*function*/) const {}

  void integrateSphere(Mantid::API::CoordTransform &radiusTransform,
                       const coord_t radiusSquared, signal_t &signal,
                       signal_t &errorSquared) const;

  void centroidSphere(Mantid::API::CoordTransform &radiusTransform,
                      const coord_t radiusSquared, coord_t *centroid,
                      signal_t &signal) const;

  void integrateCylinder(Mantid::API::CoordTransform &radiusTransform,
                         const coord_t radius, const coord_t length,
                         signal_t &signal, signal_t &errorSquared,
                         std::vector<signal_t> &signal_fit) const;

  void splitContents(size_t index, Kernel::ThreadScheduler *ts = NULL);

  void splitAllIfNeeded(Kernel::ThreadScheduler *ts = NULL);

  void refreshCache(Kernel::ThreadScheduler *ts = NULL);

  virtual bool getIsMasked() const;
  /// Setter for masking the box
  virtual void mask();
  /// Setter for unmasking the box
  virtual void unmask();
  // ======================= Testing/Debugging Methods =================
  /** For testing: get (a reference to) the vector of boxes */
  std::vector<MDBoxBase<MDE, nd> *> &getBoxes() { return m_Children; }

  //-------------------------------------------------------------------------
  /** The function used to satisfy IMDNode interface but the physical meaning is
   * unclear */
  void calculateCentroid(coord_t * /*centroid*/) const {
    throw(std::runtime_error("This function should not be called on MDGridBox "
                             "(as its meaning for MDbox is dubious too)"));
  }
  //-------------------------------------------------------------------------
  /** The function used to satisfy IMDNode interface but the physical meaning is
   * unclear */
  coord_t *getCentroid() const {
    throw(std::runtime_error("This function should not be called on MDGridBox "
                             "(as its meaning for MDbox is dubious too)"));
  }

public:
  /// Typedef for a shared pointer to a MDGridBox
  typedef boost::shared_ptr<MDGridBox<MDE, nd>> sptr;

  /// Typedef for a vector of MDBoxBase pointers
  typedef std::vector<MDBoxBase<MDE, nd> *> boxVector_t;

private:
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
  void fillBoxShell(const size_t tot, const coord_t inverseVolume);
  /**private default copy constructor as the only correct constructor is the one
   * with box controller */
  MDGridBox(const MDGridBox<MDE, nd> &box);
  /**Private constructor as it does not work without box controller */
  MDGridBox() {}
  /// common part of MDGridBox contstructor;
  void initGridBox();
};

#ifndef __INTEL_COMPILER
#pragma pack(pop) // Return to default packing size
#endif

} // namespace DataObjects
} // namespace Mantid

#endif /* MDGRIDBOX_H_ */
