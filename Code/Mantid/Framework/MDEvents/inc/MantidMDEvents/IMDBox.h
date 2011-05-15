#ifndef IMDBOX_H_
#define IMDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidMDEvents/MDEvent.h"
#include <iosfwd>

namespace Mantid
{
namespace MDEvents
{

  /// Forward declaration
  TMDE_CLASS
  class MDBoxTask;

  //===============================================================================================
  /** Abstract Interface for a multi-dimensional event "box".
   * To be subclassed by MDBox and MDGridBox
   *
   * A box is a container of MDEvents within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport IMDBox
  {
  public:

    //-----------------------------------------------------------------------------------------------
    /** Default constructor.
     */
    IMDBox() : m_signal(0.0), m_errorSquared(0.0), m_depth(0)
    { }

    /// Copy constructor
    IMDBox(IMDBox<MDE,nd> * box);

    // -------------------------------------------------------------------------------------------
    /// Destructor
    virtual ~IMDBox() {}

    /// Clear all contained data
    virtual void clear() = 0;

    /// Get total number of points
    virtual size_t getNPoints() const = 0;

    /// Get number of dimensions
    virtual size_t getNumDims() const = 0;

    /// Get the total # of unsplit MDBoxes contained.
    virtual size_t getNumMDBoxes() const = 0;

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
     */
    virtual void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const = 0;

    /** Sphere (peak) integration */
    virtual void integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const = 0;

    // -------------------------------------------------------------------------------------------
    /** Split sub-boxes, if this is possible and neede for this box */
    virtual void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL)
    {
      // Do nothing by default.
//      // Can't split it!
//      throw std::runtime_error("splitAllIfNeeded called on a MDBox (call splitBox() first). Call MDEventWorkspace::splitBox() before!");
    }


    // -------------------------------------------------------------------------------------------
    /** Recalculate signal etc. */
    virtual void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL)
    {
      // Do nothing by default.
    }



    /// Run a generic MDBoxTask onto this, going recursively.
//    virtual void runMDBoxTask(MDBoxTask<MDE,nd> * task, const bool fullyContained) = 0;


    // -------------------------------------------------------------------------------------------
    /// Return the box controller saved.
    BoxController_sptr getBoxController() const
    { return m_BoxController; }

    /** Set the box controller used.
     * @param controller :: BoxController_sptr
     */
    void setBoxController(BoxController_sptr controller)
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
    MDDimensionExtents & getExtents(size_t dim)
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
        center[d] = (extents[d].max + extents[d].min) / 2.0;
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
      m_inverseVolume = 1.0 / volume;
    }


    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated signal from all points within.
     */
    virtual signal_t getSignal() const
    {
      return m_signal;
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
    virtual void setSignal(const double signal)
    {
      m_signal = signal;
    }

    //-----------------------------------------------------------------------------------------------
    /** Sets the integrated error squared from all points within (mostly used for testing)
     * @param ErrorSquared :: new squared error.
     */
    virtual void setErrorSquared(const double ErrorSquared)
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
     * e.g. 1: means this box is in a MDGridBox, which is the top level.
     *      2: this box's parent MDGridBox is itself a MDGridBox. */
    size_t getDepth() const
    {
      return m_depth;
    }

    //-----------------------------------------------------------------------------------------------
    /** Return the volume of the cell */
    coord_t getVolume() const
    {
      return 1.0 / m_inverseVolume;
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

  protected:

    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    MDDimensionExtents extents[nd];

    /** Cached total signal from all points within */
    signal_t m_signal;

    /** Cached total error (squared) from all points within */
    signal_t m_errorSquared;

    /// Inverse of the volume of the cell, to be used for normalized signal.
    coord_t m_inverseVolume;

    /// The box splitting controller, shared with all boxes in the hierarchy
    BoxController_sptr m_BoxController;

    /// Recursion depth
    size_t m_depth;

  public:
    /// Convenience typedef for a shared pointer to a this type of class
    typedef boost::shared_ptr< IMDBox<MDE, nd> > sptr;

  };







}//namespace MDEvents

}//namespace Mantid

#endif /* IMDBOX_H_ */
