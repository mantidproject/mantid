#ifndef IMD_NODE_H_
#define IMD_NODE_H_

#include <algorithm>
#include <string>
#include <vector>
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

namespace Mantid {
namespace Kernel {
class ISaveable;
class ThreadScheduler;
}

namespace Geometry {
template <typename T> class MDDimensionExtents;
class MDImplicitFunction;
}

namespace API {

class BoxController;
class IBoxControllerIO;
class CoordTransform;

class IMDNode {
  /** This is an interface to MDBox or MDGridBox of an MDWorkspace

      @date 01/03/2013

      Copyright &copy; 2009-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
     Ridge National Laboratory & European Spallation Source

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

      File change history is stored at:
     <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

public:
  virtual ~IMDNode(){}
  //---------------- ISAVABLE
  /**Return the pointer to the structure responsible for saving the box on disk
   * if the workspace occupies too much memory */
  virtual Kernel::ISaveable *getISaveable() = 0;
  /**Return the pointer to the sconst tructure responsible for saving the box on
   * disk if the workspace occupies too much memory */
  virtual Kernel::ISaveable *getISaveable() const = 0;
  /** initiate the structure responsible for swapping the box on HDD if out of
   * memory. */
  virtual void setFileBacked(const uint64_t /*fileLocation*/,
                             const size_t /*fileSize*/,
                             const bool /*markSaved*/) = 0;
  /** initiate the structure responsible for swapping the box on HDD if out of
   * memory with default parameters (it does not know its place on HDD and was
   * not saved). */
  virtual void setFileBacked() = 0;
  /** if node was fileBacked, the method clears file-backed information
   *@param loadFileData -- if true, the data on HDD and not yet in memory are
   loaded into memory before deleting fileBacked information,
                           if false, all on HDD contents are discarded, which
   can break the data integrity (used by destructor)  */
  virtual void clearFileBacked(bool loadFileData) = 0;
  virtual void reserveMemoryForLoad(uint64_t) = 0;

  /**Save the box at specific disk position using the class, respoinsible for
   * the file IO. */
  virtual void saveAt(API::IBoxControllerIO *const /*saver */,
                      uint64_t /*position*/) const = 0;
  /**Load the additional box data of specified size from the disk location
   * provided using the class, respoinsible for the file IO and append them to
   * the box */
  virtual void loadAndAddFrom(API::IBoxControllerIO *const /*saver */,
                              uint64_t /*position*/, size_t /* Size */) = 0;
  /// drop event data from memory but keep averages
  virtual void clearDataFromMemory() = 0;
  //-------------------------------------------------------------
  /// Clear all contained data including precalculated averages.
  virtual void clear() = 0;

  ///@return the type of the event this box contains
  virtual std::string getEventType() const = 0;
  ///@return the length of the coordinates (in bytes), the events in the box
  /// contain.
  virtual unsigned int getCoordType() const = 0;
  //-------------------------------------------------------------
  ///@return The special ID which specify location of this node in the chain of
  /// ordered boxes (e.g. on a file)
  virtual size_t getID() const = 0;
  /// sets the special id, which specify the position of this node in the chain
  /// linearly ordered nodes
  virtual void setID(const size_t &newID) = 0;

  /// Get number of dimensions, the box with this interface has
  virtual size_t getNumDims() const = 0;

  /// Getter for the masking
  virtual bool getIsMasked() const = 0;
  /// Setter for masking the box
  virtual void mask() = 0;
  /// Setter for unmasking the box
  virtual void unmask() = 0;

  /// get box controller
  virtual Mantid::API::BoxController *getBoxController() const = 0;
  virtual Mantid::API::BoxController *getBoxController() = 0;

  // -------------------------------- Parents/Children-Related
  // -------------------------------------------
  /// Get the total # of unsplit MDBoxes contained.
  virtual size_t getNumMDBoxes() const = 0;
  /// Get the # of children MDBoxBase'es (non-recursive)
  virtual size_t getNumChildren() const = 0;
  /// Return the indexth child MDBoxBase.
  virtual IMDNode *getChild(size_t index) = 0;
  /// Sets the children from a vector of children
  virtual void setChildren(const std::vector<IMDNode *> &boxes,
                           const size_t indexStart, const size_t indexEnd) = 0;
  /// Return a pointer to the parent box
  virtual void setParent(IMDNode *parent) = 0;
  /// Return a pointer to the parent box
  virtual IMDNode *getParent() = 0;
  /// Return a pointer to the parent box (const)
  virtual const IMDNode *getParent() const = 0;
  // -------------------------------------------------------------------------------------------
  // box-related
  /// Fill a vector with all the boxes who are the childred of this one up to a
  /// certain depth
  virtual void getBoxes(std::vector<IMDNode *> &boxes, size_t maxDepth,
                        bool leafOnly) = 0;
  /// Fill a vector with all the boxes who are the childred of this one  up to a
  /// certain depth and selected by the function.
  virtual void getBoxes(std::vector<IMDNode *> &boxes, size_t maxDepth,
                        bool leafOnly,
                        Mantid::Geometry::MDImplicitFunction *function) = 0;

  // -------------------------------- Events-Related
  // -------------------------------------------
  /// Get total number of points both in memory and on file if present;
  virtual uint64_t getNPoints() const = 0;
  /// get size of the data located in memory, it is equivalent to getNPoints
  /// above for memory based workspace but may be different for file based one ;
  virtual size_t getDataInMemorySize() const = 0;
  /// @return the amount of memory that the object takes up in the MRU.
  virtual uint64_t getTotalDataSize() const = 0;

  /** The method to convert events in a box into a table of
   * coodrinates/signal/errors casted into coord_t type
   *   Used to save events from plain binary file
   *   @returns coordTable -- vector of events parameters
   *   @return nColumns    -- number of parameters for each event
   */
  virtual void getEventsData(std::vector<coord_t> &coordTable,
                             size_t &nColumns) const = 0;
  /** The method to convert the table of data into vector of events
   *   Used to load events from plain binary file
   *   @param coordTable -- vector of events data, which would be packed into
   * events
   */
  virtual void setEventsData(const std::vector<coord_t> &coordTable) = 0;

  /// Add a single event defined by its components
  virtual void buildAndAddEvent(const signal_t Signal, const signal_t errorSq,
                                const std::vector<coord_t> &point,
                                uint16_t runIndex, uint32_t detectorId) = 0;
  /// Add a single event, with no mutex locking
  virtual void buildAndAddEventUnsafe(const signal_t Signal,
                                      const signal_t errorSq,
                                      const std::vector<coord_t> &point,
                                      uint16_t runIndex,
                                      uint32_t detectorId) = 0;
  /// Add several events from the vector of event parameters
  virtual size_t buildAndAddEvents(const std::vector<signal_t> &sigErrSq,
                                   const std::vector<coord_t> &Coord,
                                   const std::vector<uint16_t> &runIndex,
                                   const std::vector<uint32_t> &detectorId) = 0;

  // -------------------------------------------------------------------------------------------

  /** Sphere (peak) integration
  * The CoordTransform object could be used for more complex shapes, e.g.
  *"lentil" integration, as long
  * as it reduces the dimensions to a single value.
  *
  * @param radiusTransform :: nd-to-1 coordinate transformation that converts
  *from these
  *        dimensions to the distance (squared) from the center of the sphere.
  * @param radiusSquared :: radius^2 below which to integrate
  * @param signal [out] :: set to the integrated signal
  * @param errorSquared [out] :: set to the integrated squared error.
   */
  virtual void integrateSphere(Mantid::API::CoordTransform &radiusTransform,
                               const coord_t radiusSquared, signal_t &signal,
                               signal_t &errorSquared) const = 0;
  /** Find the centroid of all events contained within by doing a weighted
  *average
  * of their coordinates.
  *
  * @param radiusTransform :: nd-to-1 coordinate transformation that converts
  *from these
  *        dimensions to the distance (squared) from the center of the sphere.
  * @param radiusSquared :: radius^2 below which to integrate
  * @param[out] centroid :: array of size [nd]; its centroid will be added
  * @param[out] signal :: set to the integrated signal
  */
  virtual void centroidSphere(Mantid::API::CoordTransform &radiusTransform,
                              const coord_t radiusSquared, coord_t *centroid,
                              signal_t &signal) const = 0;
  /** Cylinder (peak) integration
   * The CoordTransform object could be used for more cylinder
   * reduces the dimensions to two values.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts
   *from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radius :: radius of cylinder below which to integrate
   * @param length :: length of cylinder below which to integrate
   * @param signal [out] :: set to the integrated signal
   * @param errorSquared [out] :: set to the integrated squared error.
   * @param signal_fit [out] :: array of values for the fit.
    */
  virtual void integrateCylinder(Mantid::API::CoordTransform &radiusTransform,
                                 const coord_t radius, const coord_t length,
                                 signal_t &signal, signal_t &errorSquared,
                                 std::vector<signal_t> &signal_fit) const = 0;

  /** Split sub-boxes, if this is possible and neede for this box */
  virtual void
  splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL) = 0;
  /** Recalculate signal etc. */
  virtual void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL) = 0;
  /** Calculate the centroid of this box and all sub-boxes. */
  virtual void calculateCentroid(coord_t * /*centroid*/) const = 0;
  /** Get the centroid of this box and all sub-boxes. */
  virtual coord_t *getCentroid() const = 0;
  //----------------------------------------------------------------------------------------------------------------------------------
  // MDBoxBase interface, related to average signals/error box parameters
  virtual signal_t getSignal() const = 0;
  virtual signal_t getError() const = 0;
  virtual signal_t getErrorSquared() const = 0;
  virtual coord_t getInverseVolume() const = 0;
  virtual Mantid::Geometry::MDDimensionExtents<coord_t> &
  getExtents(size_t dim) = 0;
  virtual const IMDNode *getBoxAtCoord(const coord_t * /*coords*/) = 0;
  virtual void getCenter(coord_t *const /*boxCenter*/) const = 0;
  virtual uint32_t getDepth() const = 0;
  virtual signal_t getSignalNormalized() const = 0;

  virtual void calcVolume() = 0;
  virtual void setInverseVolume(const coord_t) = 0;
  virtual void setSignal(const signal_t) = 0;
  virtual void setErrorSquared(const signal_t) = 0;

  // -------------------------------- Geometry/vertexes-Related
  // -------------------------------------------
  virtual std::vector<Mantid::Kernel::VMD> getVertexes() const = 0;
  virtual coord_t *getVertexesArray(size_t &numVertices) const = 0;
  virtual coord_t *getVertexesArray(size_t &numVertices,
                                    const size_t outDimensions,
                                    const bool *maskDim) const = 0;
  virtual void transformDimensions(std::vector<double> &scaling,
                                   std::vector<double> &offset) = 0;

  // to avoid casting (which need also the number of dimensions) method say if
  // Node is a box. if not, it is gridbox
  virtual bool isBox() const = 0;
  // ----------------------------- Helper Methods
  // --------------------------------------------------------
  //-----------------------------------------------------------------------------------------------
  /** Helper method for sorting MDBoxBasees by file position.
   * MDGridBoxes return 0 for file position and so aren't sorted.
   *
   * @param a :: an MDBoxBase pointer
   * @param b :: an MDBoxBase pointer
   * @return
   */

  static inline bool CompareFilePosition(const IMDNode *const a,
                                         const IMDNode *const b) {

    return (a->getID() < b->getID());
  }

  //-----------------------------------------------------------------------------------------------
  /** Static method for sorting a list of MDBoxBase pointers by their file
   *position,
   * ascending. This should optimize the speed of loading a bit by
   * reducing the amount of disk seeking.
   *
   * @param boxes :: ref to a vector of boxes. It will be sorted in-place.
   */
  static void sortObjByID(std::vector<IMDNode *> &boxes) {
    std::sort(boxes.begin(), boxes.end(), CompareFilePosition);
  }
};
}
}

#endif
