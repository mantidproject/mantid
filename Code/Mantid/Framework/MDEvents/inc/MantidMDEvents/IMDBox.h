#ifndef IMDBOX_H_
#define IMDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include <iosfwd>
#include "MantidKernel/VMD.h"


/// Define to keep the centroid around as a field on each IMDBox.
#undef MDBOX_TRACK_CENTROID

namespace Mantid
{
namespace MDEvents
{

#pragma pack(push, 4) //Ensure the structure is no larger than it needs to

  //===============================================================================================
  /** Abstract Interface for a multi-dimensional event "box".
   * To be subclassed by MDBox and MDGridBox
   *
   * A box is a container of MDEvents within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * @tparam nd :: the number of dimensions that each MDLeanEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport IMDBox : public Mantid::Kernel::ISaveable
  {
  public:

    //-----------------------------------------------------------------------------------------------
    IMDBox();

    IMDBox(const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector);

    IMDBox(const IMDBox<MDE,nd> & box);

    /// Destructor
    virtual ~IMDBox() {}

    /// Get number of dimensions
    virtual size_t getNumDims() const = 0;



    // ----------------------------- ISaveable Methods ------------------------------------------------------

    /// Save the data - to be overriden
    virtual void save() const
    {
      std::cerr << "ID " << getId() << std::endl;
      throw std::runtime_error("IMDBox::save() called and should have been overridden.");
    }

    /// Flush the data to disk. Allows NXS api to actually write out the file.
    virtual void flushData() const
    {
      ::NeXus::File * file = this->m_BoxController->getFile();
      if (file)
      {
        MDE::closeNexusData(file);
        MDE::openNexusData(file);
      }
    }

    /// Load the data - to be overriden
    virtual void load()
    { }

    /// @return the amount of memory that the object takes up in the MRU.
    virtual uint64_t getMRUMemorySize() const
    { return 0; }

    /// @return true if it the data of the object is busy and so cannot be cleared; false if the data was released and can be cleared/written.
    virtual bool dataBusy() const
    { return false; }

    /** @return the position in the file where the data will be stored. This is used to optimize file writing. */
    virtual uint64_t getFilePosition() const
    { return 0; }
    // -----------------------------------------------------------------------------------------------------


    // ----------------------------- Helper Methods --------------------------------------------------------
    static void sortBoxesByFilePos(std::vector<IMDBox<MDE,nd> *> & boxes);
    // -----------------------------------------------------------------------------------------------------


    // -------------------------------- Parents/Children-Related -------------------------------------------
    /// Get the total # of unsplit MDBoxes contained.
    virtual size_t getNumMDBoxes() const = 0;


    /// Get the # of children IMDBox'es (non-recursive)
    virtual size_t getNumChildren() const = 0;

    /// Return the indexth child IMDBox.
    virtual IMDBox<MDE,nd> * getChild(size_t index) = 0;

    /// Sets the children from a vector of children
    virtual void setChildren(const std::vector<IMDBox<MDE,nd> *> & boxes, const size_t indexStart, const size_t indexEnd) = 0;

    /// Return a pointer to the parent box
    void setParent(IMDBox<MDE,nd> * parent)
    { m_parent = parent; }

    /// Return a pointer to the parent box
    IMDBox<MDE,nd> * getParent()
    { return m_parent; }

    /// Return a pointer to the parent box (const)
    const IMDBox<MDE,nd> * getParent() const
    { return m_parent; }

    /// Fill a vector with all the boxes up to a certain depth
    virtual void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly) = 0;

    /// Fill a vector with all the boxes up to a certain depth
    virtual void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function) = 0;

    /** Split sub-boxes, if this is possible and neede for this box */
    virtual void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL)
    {  /* Do nothing by default. */ }

    /** Recalculate signal etc. */
    virtual void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL)
    {  /* Do nothing by default. */ }

    /// Returns the lowest-level box at the given coordinates
    virtual const IMDBox<MDE,nd> * getBoxAtCoord(const coord_t * /*coords*/) const
    { return this; }



    // -------------------------------- Geometry/vertexes-Related -------------------------------------------

    std::vector<Mantid::Kernel::VMD> getVertexes() const;

    coord_t * getVertexesArray(size_t & numVertices) const;

    coord_t * getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const;

    virtual void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset);


    // -------------------------------- Events-Related -------------------------------------------

    /// Clear all contained data
    virtual void clear() = 0;

    /// Get total number of points
    virtual uint64_t getNPoints() const = 0;

    /// Return a copy of contained events
    virtual std::vector< MDE > * getEventsCopy() = 0;

    /// Add a single event
    virtual void addEvent(const MDE & point) = 0;

    /** Add several events from a vector
     * @param events :: vector of MDEvents to add (all of it)
     * @return the number of events that were rejected (because of being out of bounds)
     */
    virtual size_t addEvents(const std::vector<MDE> & events)
    {
      return addEvents(events, 0, events.size());
    }

    /// Add several events, within a given range
    virtual size_t addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);

    /** Perform centerpoint binning of events
     * @param bin :: MDBin object giving the limits of events to accept.
     * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
     */
    virtual void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const = 0;

    /// General binning method for any shape.
    virtual void generalBin(MDBin<MDE,nd> & bin, Mantid::Geometry::MDImplicitFunction & function) const = 0;

    /** Sphere (peak) integration */
    virtual void integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const = 0;

    /** Find the centroid around a sphere */
    virtual void centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const = 0;


    // -------------------------------------------------------------------------------------------
    /** Cache the centroid of this box and all sub-boxes. */
    virtual void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL) {} //= 0;

    virtual void calculateCentroid(coord_t * /*centroid*/) const {};

    // -------------------------------------------------------------------------------------------
    /// @return the box controller saved.
    Mantid::API::BoxController_sptr getBoxController() const
    { return m_BoxController; }

    /** Set the box controller used.
     * @param controller :: Mantid::API::BoxController_sptr
     */
    void setBoxController(Mantid::API::BoxController_sptr controller)
    { m_BoxController = controller; }

    //-----------------------------------------------------------------------------------------------
    /** Set the extents of this box.
     * @param dim :: index of dimension
     * @param min :: min edge of the dimension
     * @param max :: max edge of the dimension
     */
    void setExtents(size_t dim, coord_t min, coord_t max)
    {
      if (dim >= nd)
        throw std::invalid_argument("Invalid dimension passed to MDBox::setExtents");
      extents[dim].min = min;
      extents[dim].max = max;
    }

    //-----------------------------------------------------------------------------------------------
    /** Get the extents for this box */
    Mantid::Geometry::MDDimensionExtents & getExtents(size_t dim)
    {
      return extents[dim];
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the extents as a string, for convenience */
    std::string getExtentsStr() const
    {
      std::stringstream mess;
      for (size_t d=0; d<nd; ++d)
      {
        mess << extents[d].min << "-"<< extents[d].max;
        if (d+1 < nd) mess << ",";
      }
      return mess.str();
    }

    //-----------------------------------------------------------------------------------------------
    /** Get the center of the box
     * @param center :: bare array of size[nd] that will get set with the mid-point of each dimension.
     */
    void getCenter(coord_t * center) const
    {
      for (size_t d=0; d<nd; ++d)
        center[d] = (extents[d].max + extents[d].min) / coord_t(2.0);
    }

    //-----------------------------------------------------------------------------------------------
    /** Compute the volume of the box by simply multiplying each dimension range.
     * Call this after setExtents() is set for all dimensions.
     * This is saved for getSignalNormalized() */
    inline void calcVolume()
    {
      coord_t volume = 1;
      for (size_t d=0; d<nd; d++)
      {
        volume *= (extents[d].max - extents[d].min);
      }
      /// Floating point multiplication is much faster than division, so cache 1/volume.
      m_inverseVolume = coord_t(1.0) / volume;
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated signal from all points within.
     */
    virtual signal_t getSignal() const
    {
      return m_signal;
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated error from all points within.
     */
    virtual signal_t getError() const
    {
      return sqrt(m_errorSquared);
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated error squared from all points within.
     */
    virtual signal_t getErrorSquared() const
    {
      return m_errorSquared;
    }

    //-----------------------------------------------------------------------------------------------
    /** Sets the integrated signal from all points within  (mostly used for testing)
     * @param signal :: new Signal amount.
     */
    virtual void setSignal(const signal_t signal)
    {
      m_signal = signal;
    }

    //-----------------------------------------------------------------------------------------------
    /** Sets the integrated error squared from all points within (mostly used for testing)
     * @param ErrorSquared :: new squared error.
     */
    virtual void setErrorSquared(const signal_t ErrorSquared)
    {
      m_errorSquared = ErrorSquared;
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated signal from all points within, normalized for the cell volume
     */
    virtual signal_t getSignalNormalized() const
    {
      return m_signal * m_inverseVolume;
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated error squared from all points within, normalized for the cell volume
     */
    virtual signal_t getErrorSquaredNormalized() const
    {
      return m_errorSquared * m_inverseVolume;
    }

    //-----------------------------------------------------------------------------------------------
    /** For testing, mostly: return the recursion depth of this box.
     * 0 is the top-level box, 1 is one deeper, etc.
     * @return split recursion depth*/
    size_t getDepth() const
    {
      return m_depth;
    }

    //-----------------------------------------------------------------------------------------------
    /** For testing, mostly: set the recursion depth of this box. SHOULD NOT BE CALLED OUTSIDE OF TESTS!
     * @param depth :: split recursion depth */
    void setDepth(size_t depth)
    {
      m_depth = depth;
    }

    //-----------------------------------------------------------------------------------------------
    /** Return the volume of the cell */
    coord_t getVolume() const
    {
      return coord_t(1.0) / m_inverseVolume;
    }

    //-----------------------------------------------------------------------------------------------
    /** Return the inverse of the volume of the cell */
    coord_t getInverseVolume() const
    {
      return m_inverseVolume;
    }

    //-----------------------------------------------------------------------------------------------
    /** Sets the inverse of the volume of the cell
     * @param invVolume :: value to set. */
    void setInverseVolume(coord_t invVolume)
    {
      m_inverseVolume = invVolume;
    }

#ifdef MDBOX_TRACK_CENTROID
    //-----------------------------------------------------------------------------------------------
    /** Return the centroid of the box.
     * @param d :: index of the dimension to return.
     */
    coord_t getCentroid(size_t d) const
    {
      return m_centroid[d];
    }

    //-----------------------------------------------------------------------------------------------
    /** Return the centroid array of the box.
     */
    const coord_t * getCentroid() const
    {
      return m_centroid;
    }
#endif

  protected:
    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    Mantid::Geometry::MDDimensionExtents extents[nd];

    /** Cached total signal from all points within.
     * Set when refreshCache() is called. */
    mutable signal_t m_signal;

    /** Cached total error (squared) from all points within.
    * Set when refreshCache() is called. */
    mutable signal_t m_errorSquared;

    /// Inverse of the volume of the cell, to be used for normalized signal.
    coord_t m_inverseVolume;

    /// The box splitting controller, shared with all boxes in the hierarchy
    Mantid::API::BoxController_sptr m_BoxController;

    /// Recursion depth
    size_t m_depth;

    /// Pointer to the parent of this box. NULL if no parent.
    IMDBox<MDE,nd> * m_parent;

#ifdef MDBOX_TRACK_CENTROID
    /** The centroid (weighted center of mass) of the events in this MDBox.
     * Set when refreshCentroid() is called.
     */
    coord_t m_centroid[nd];
#endif

  public:
    /// Convenience typedef for a shared pointer to a this type of class
    typedef boost::shared_ptr< IMDBox<MDE, nd> > sptr;

  }; //(end class IMDBox)

#pragma pack(pop) //Return to default packing size





}//namespace MDEvents

}//namespace Mantid

#endif /* IMDBOX_H_ */
