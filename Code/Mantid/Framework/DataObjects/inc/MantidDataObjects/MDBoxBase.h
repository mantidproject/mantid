#ifndef MDBOXBASE_H_
#define MDBOXBASE_H_

#include "MantidAPI/IMDNode.h"
#include <iosfwd>
#include "MantidDataObjects/MDBin.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"

/// Define to keep the centroid around as a field on each MDBoxBase.
#define MDBOX_TRACK_CENTROID

namespace Mantid {
namespace DataObjects {

#ifndef __INTEL_COMPILER // As of July 13, the packing has no effect for the
                         // Intel compiler and produces a warning
#pragma pack(push, 4)    // Ensure the structure is no larger than it needs to
#endif

//===============================================================================================
/** Templated super-class of a multi-dimensional event "box".
 * To be subclassed by MDBox<> and MDGridBox<>
 *
 * A box is a container of MDEvents within a certain range of values
 * within the nd dimensions. This range defines a n-dimensional "box"
 * or rectangular prism.
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
class DLLExport MDBoxBase : public Mantid::API::IMDNode {
public:
  //-----------------------------------------------------------------------------------------------
  MDBoxBase(Mantid::API::BoxController *const BoxController = NULL,
            const uint32_t depth = 0, const size_t boxID = UNDEF_SIZET);

  MDBoxBase(Mantid::API::BoxController *const BoxController,
            const uint32_t depth, const size_t boxID,
            const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &
                extentsVector);

  MDBoxBase(const MDBoxBase<MDE, nd> &box,
            Mantid::API::BoxController *const otherBC);

  /// Destructor
  virtual ~MDBoxBase() {}
  ///@return the type of the event this box contains
  virtual std::string getEventType() const { return MDE::getTypeName(); }
  ///@return the length of the coordinates (in bytes), the events in the box
  /// contain.
  virtual unsigned int getCoordType() const { return sizeof(coord_t); }

  ///@return The special ID which specify location of this node in the chain of
  /// ordered boxes (e.g. on a file)
  virtual size_t getID() const { return m_fileID; }
  /// sets the special id, which specify the position of this node in the chain
  /// linearly ordered nodes
  virtual void setID(const size_t &newID) { m_fileID = newID; }
  // -------------------------------- Parents/Children-Related
  // -------------------------------------------
  /// Return a pointer to the parent box
  void setParent(IMDNode *parent) { m_parent = parent; }

  /// Return a pointer to the parent box
  IMDNode *getParent() { return m_parent; }

  /// Return a pointer to the parent box (const)
  const IMDNode *getParent() const { return m_parent; }

  /// Returns the lowest-level box at the given coordinates
  virtual const IMDNode *getBoxAtCoord(const coord_t * /*coords*/) {
    return this;
  }

  // -------------------------------- Events-Related
  // -------------------------------------------
  /** The method to convert events in a box into a table of
   * coodrinates/signal/errors casted into coord_t type
   *   Used to conver events into plain data array. Does nothing for GridBox */
  virtual void getEventsData(std::vector<coord_t> & /*coordTable*/,
                             size_t &nColumns) const {
    nColumns = 0;
  }
  /** The method to convert the table of data into vector of events
   *   Used to convert from a vector of values (2D table in Fortran
   representation (by rows) into box events.
           Does nothing for GridBox (may be temporary) -- can be combined with
   build and add events	 */
  virtual void setEventsData(const std::vector<coord_t> & /*coordTable*/) {}
  /// Return a copy of contained events
  virtual std::vector<MDE> *getEventsCopy() = 0;

  //----------------------------------------------------------------------------------------------------------------------
  /// Add a single event
  virtual void addEvent(const MDE &point) = 0;
  /// Add a single event, with no mutex locking
  virtual void addEventUnsafe(const MDE &point) = 0;
  //----------------------------------------------------------------------------------------------------------------------
  // add range of events
  virtual size_t addEvents(const std::vector<MDE> &events);
  virtual size_t addEventsUnsafe(const std::vector<MDE> &events);
  //----------------------------------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events
   * @param bin :: MDBin object giving the limits of events to accept.
   * @param fullyContained :: optional bool array sized [nd] of which dimensions
   * are known to be fully contained (for MDSplitBox)
   */
  virtual void centerpointBin(MDBin<MDE, nd> &bin,
                              bool *fullyContained) const = 0;

  /// General binning method for any shape.
  virtual void
  generalBin(MDBin<MDE, nd> &bin,
             Mantid::Geometry::MDImplicitFunction &function) const = 0;

  /** Sphere (peak) integration */
  virtual void integrateSphere(Mantid::API::CoordTransform &radiusTransform,
                               const coord_t radiusSquared, signal_t &signal,
                               signal_t &errorSquared) const = 0;

  /** Find the centroid around a sphere */
  virtual void centroidSphere(Mantid::API::CoordTransform &radiusTransform,
                              const coord_t radiusSquared, coord_t *centroid,
                              signal_t &signal) const = 0;

  /** Cylinder (peak) integration */
  virtual void integrateCylinder(Mantid::API::CoordTransform &radiusTransform,
                                 const coord_t radius, const coord_t length,
                                 signal_t &signal, signal_t &errorSquared,
                                 std::vector<signal_t> &signal_fit) const = 0;

  // -------------------------------------------------------------------------------------------
  /// @return the const box controller for this box.
  Mantid::API::BoxController *getBoxController() const {
    return m_BoxController;
  }
  /// @return the box controller for this box.
  virtual Mantid::API::BoxController *getBoxController() {
    return m_BoxController;
  }

  // -------------------------------- Geometry/vertexes-Related
  // -------------------------------------------

  virtual std::vector<Mantid::Kernel::VMD> getVertexes() const;
  virtual coord_t *getVertexesArray(size_t &numVertices) const;
  virtual coord_t *getVertexesArray(size_t &numVertices,
                                    const size_t outDimensions,
                                    const bool *maskDim) const;
  virtual void transformDimensions(std::vector<double> &scaling,
                                   std::vector<double> &offset);

  //-----------------------------------------------------------------------------------------------
  /** Set the extents of this box.
   * @param dim :: index of dimension
   * @param min :: min edge of the dimension
   * @param max :: max edge of the dimension

   * Dangerous function with side effects as volume and possibly other box
   statistics has to be recalculated after this excecuted.
   * has not done so because of productivity reasons;
   */
  void setExtents(size_t dim, double min, double max) {
    if (dim >= nd)
      throw std::invalid_argument(
          "Invalid dimension passed to MDBox::setExtents");

    extents[dim].setExtents(min, max);
    // volume has to be recalculated as extents have changed;
    this->calcVolume();
  }
  /** Set the extents of this box.
     * @param min :: min edge of the dimension
   * @param max :: max edge of the dimension
   */
  void setExtents(double min[nd], double max[nd]) {
    for (size_t dim = 0; dim < nd; dim++) {
      this->extents[dim].setExtents(min[dim], max[dim]);
    }
    this->calcVolume();
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the extents for this box */
  virtual Mantid::Geometry::MDDimensionExtents<coord_t> &
  getExtents(size_t dim) {
    return extents[dim];
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the extents as a string, for convenience */
  std::string getExtentsStr() const {
    std::string mess("");
    size_t ndm1 = nd - 1;
    if (ndm1 > 32)
      return mess;

    for (size_t d = 0; d < ndm1; ++d)
      mess += extents[d].extentsStr() + ",";

    mess += extents[ndm1].extentsStr();
    return mess;
  }

  /** For testing: return the internal-stored size of each box in each dimension
   */
  coord_t getBoxSize(size_t d) { return extents[d].getSize(); }

  //-----------------------------------------------------------------------------------------------
  /** Get the center of the box
   * @param center :: bare array of size[nd] that will get set with the
   * mid-point of each dimension.
   */
  virtual void getCenter(coord_t *const center) const {
    for (size_t d = 0; d < nd; ++d)
      center[d] = extents[d].getCentre();
  }

  //-----------------------------------------------------------------------------------------------
  /** Compute the volume of the box by simply multiplying each dimension range.
   * Call this after setExtents() is set for all dimensions.
   * This is saved for getSignalNormalized() */
  inline void calcVolume() {
    double volume(1);
    for (size_t d = 0; d < nd; d++) {
      volume *= double(extents[d].getSize());
    }
    /// Floating point multiplication is much faster than division, so cache
    /// 1/volume.
    m_inverseVolume = coord_t(1. / volume);
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated signal from all points within.
   */
  virtual signal_t getSignal() const { return m_signal; }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error from all points within.
   */
  virtual signal_t getError() const { return sqrt(m_errorSquared); }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error squared from all points within.
   */
  virtual signal_t getErrorSquared() const { return m_errorSquared; }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total weight of all events within. Typically this is
   * equal to the number of events (weight of 1 per event)
   */
  virtual signal_t getTotalWeight() const { return m_totalWeight; }

  //-----------------------------------------------------------------------------------------------
  /** Sets the integrated signal from all points within  (mostly used for
   * testing)
   * @param signal :: new Signal amount.
   */
  virtual void setSignal(const signal_t signal) { m_signal = signal; }

  //-----------------------------------------------------------------------------------------------
  /** Sets the integrated error squared from all points within (mostly used for
   * testing)
   * @param ErrorSquared :: new squared error.
   */
  virtual void setErrorSquared(const signal_t ErrorSquared) {
    m_errorSquared = ErrorSquared;
  }

  //-----------------------------------------------------------------------------------------------
  /** Sets the total weight from all points within  (mostly used for testing)
   * @param total :: new weight amount.
   */
  virtual void setTotalWeight(const signal_t total) { m_totalWeight = total; }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated signal from all points within, normalized for the
   * cell volume
   */
  virtual signal_t getSignalNormalized() const {
    return m_signal * m_inverseVolume;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error squared from all points within, normalized
   * for the cell volume
   */
  virtual signal_t getErrorSquaredNormalized() const {
    return m_errorSquared * m_inverseVolume;
  }

  //-----------------------------------------------------------------------------------------------
  /** For testing, mostly: return the recursion depth of this box.
   * 0 is the top-level box, 1 is one deeper, etc.
   * @return split recursion depth*/
  uint32_t getDepth() const { return m_depth; }

  //-----------------------------------------------------------------------------------------------
  /** For testing, mostly: set the recursion depth of this box. SHOULD NOT BE
   * CALLED OUTSIDE OF TESTS!
   * @param depth :: split recursion depth */
  void setDepth(uint32_t depth) { m_depth = depth; }

  //-----------------------------------------------------------------------------------------------
  /** Return the volume of the cell */
  coord_t getVolume() const { return 1.0f / m_inverseVolume; }

  //-----------------------------------------------------------------------------------------------
  /** Return the inverse of the volume of the cell */
  virtual coord_t getInverseVolume() const { return m_inverseVolume; }

  //-----------------------------------------------------------------------------------------------
  /** Sets the inverse of the volume of the cell
   * @param invVolume :: value to set. */
  void setInverseVolume(const coord_t invVolume) {
    m_inverseVolume = invVolume;
  }

protected:
  /** Array of MDDimensionStats giving the extents and
   * other stats on the box dimensions.
   */
  Mantid::Geometry::MDDimensionExtents<coord_t> extents[nd];

  mutable coord_t m_centroid[nd];
  /** Cached total signal from all points within.
   * Set when refreshCache() is called. */
  mutable signal_t m_signal;

  /** Cached total error (squared) from all points within.
  * Set when refreshCache() is called. */
  mutable signal_t m_errorSquared;

  /** Cached total weight of all events
   * Set when refreshCache() is called. */
  mutable signal_t m_totalWeight;

  /// The box splitting controller, shared with all boxes in the hierarchy
  Mantid::API::BoxController *const m_BoxController;

  /// Inverse of the volume of the cell, to be used for normalized signal.
  coord_t m_inverseVolume;

  /// Recursion depth
  uint32_t m_depth;

  /// Pointer to the parent of this box. NULL if no parent.
  Mantid::API::IMDNode *m_parent;

  /// The id which specify location of this box in a linear chain of ordered
  /// boxes (e.g. on file). Calculated algorithmically
  size_t m_fileID;
  /// Mutex for modifying the event list or box averages
  Mantid::Kernel::Mutex m_dataMutex;

private:
  MDBoxBase(const MDBoxBase<MDE, nd> &box);

public:
  /// Convenience typedef for a shared pointer to a this type of class
  typedef boost::shared_ptr<MDBoxBase<MDE, nd>> sptr;

}; //(end class MDBoxBase)

#ifndef __INTEL_COMPILER
#pragma pack(pop) // Return to default packing size
#endif

} // namespace DataObjects
} // namespace Mantid

#endif /* MDBOXBASE_H_ */
