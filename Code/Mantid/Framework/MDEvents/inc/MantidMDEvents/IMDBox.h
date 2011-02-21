#ifndef IMDBOX_H_
#define IMDBOX_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

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
    IMDBox() : m_signal(0.0), m_errorSquared(0.0)
    {
    }


    /// Clear all contained data
    virtual void clear() = 0;

    /// Get total number of points
    virtual size_t getNPoints() const = 0;

    /// Get number of dimensions
    virtual size_t getNumDims() const = 0;

    /// Return a copy of contained events
    virtual std::vector< MDE > * getEventsCopy() = 0;

    /// Return true if the box should split into a gridded box.
    virtual bool willSplit(size_t num) const = 0;

    /// Add a single event
    virtual void addEvent(const MDE & point) = 0;

    /// Add several events
    virtual void addEvents(const std::vector<MDE> & events) = 0;


    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated signal from all points within.
     */
    virtual double getSignal() const
    {
      return m_signal;
    }

    //-----------------------------------------------------------------------------------------------
    /** Returns the integrated error squared from all points within.
     */
    virtual double getErrorSquared() const
    {
      return m_errorSquared;
    }


  protected:

    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    MDDimensionExtents dimExtents[nd];

    /** Cached total signal from all points within */
    double m_signal;

    /** Cached total error (squared) from all points within */
    double m_errorSquared;


  public:
    /// Convenience typedef for a shared pointer to a this type of class
    typedef boost::shared_ptr< IMDBox<MDE, nd> > sptr;

  };







}//namespace MDEvents

}//namespace Mantid

#endif /* IMDBOX_H_ */
