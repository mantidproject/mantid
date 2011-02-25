#ifndef BOXCONTROLLER_H_
#define BOXCONTROLLER_H_

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace MDEvents
{

  /** This class is used by MDBox and MDGridBox in order to intelligently
   * determine optimal behavior. It informs:
   *  - When a MDBox needs to split into a MDGridBox.
   *  - How the splitting will occur.
   *
   * @author Janik Zikovsky
   * @date Feb 21, 2011
   */
  class DLLExport BoxController
  {
  public:

    //-----------------------------------------------------------------------------------
    /** Constructor
     *
     * @return
     */
    BoxController(size_t nd)
    : nd(nd)
    {
    }

    //-----------------------------------------------------------------------------------
    /** Get # of dimensions
     * @return # of dimensions
     */
    size_t getNDims()
    {
      return nd;
    }

    //-----------------------------------------------------------------------------------
    /** Set the splitting threshold
    * @param threshold :: # of points at which the MDBox splits
    */
    void setSplitThreshold(size_t threshold)
    {
      m_SplitThreshold = threshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return true if the MDBox should split, given :
     *
     * @param original :: current size (# of points) in the box
     * @param added :: # to add
     * @return bool
     */
    bool willSplit(size_t original, size_t added)
    {
      return (original+added) > m_SplitThreshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return into how many to split along a dimension
     *
     * @param dim :: index of the dimension to split
     * @return the dimension will be split into this many even boxes.
     */
    size_t splitInto(size_t dim)
    {
      return m_splitInto[dim];
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     * @param num :: amount in which to split
     * */
    void setSplitInto(size_t num)
    {
      m_splitInto.resize(nd, num);
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     *
     * @param dim :: dimension to set
     * @param num :: amount in which to split
     */
    void setSplitInto(size_t dim, size_t num)
    {
      m_splitInto[dim] = num;
    }


  protected:
    // Number of dimensions
    size_t nd;

    /// Splitting threshold
    size_t m_SplitThreshold;

    // Even splitting for all dimensions
    std::vector<size_t> m_splitInto;

  };

  /// Shared ptr to BoxController
  typedef boost::shared_ptr<BoxController> BoxController_sptr;

}//namespace MDEvents

}//namespace Mantid



#endif /* SPLITCONTROLLER_H_ */
