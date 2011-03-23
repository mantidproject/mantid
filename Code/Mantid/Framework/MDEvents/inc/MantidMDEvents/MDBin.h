#ifndef MANTID_MDEVENTS_MDBIN_H_
#define MANTID_MDEVENTS_MDBIN_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Dimension.h"
#include "MantidMDEvents/MDEvent.h"


namespace Mantid
{
namespace MDEvents
{
  /** MDBin : Class describing a single bin in a dense, Multidimensional histogram.
   * This object will get passed around by MDBox'es and accumulate the total
   * signal of events contained in it.
   * The signal (and error) of each bin will be used to make the big dense histogram.
   * 
   * NOTE: For now, will only support bins that are aligned with the workspace axes (no rotation!),
   * but in future it will be extended.
   *
   * @tparam nd :: the number of dimensions **in the workspace being binned**
   *
   * @author Janik Zikovsky, SNS
   * @date 2011-03-23 17:04:02.621325
   */
  TMDE_CLASS
  class DLLExport MDBin 
  {
  public:
    MDBin();

    /// Destructor
    ~MDBin()
    {}
    
    /** The accumulated signal in this bin.
     * This is public so as to avoid the need (and slowdown) of getters/setters
     */
    double m_signal;

    /** The accumulated error (squared) in this bin.
    * This is public so as to avoid the need (and slowdown) of getters/setters
    */
    double m_errorSquared;

    /// The minimum edge of the bin for each dimension in the workspace
    CoordType m_min[nd];

    /// The maximum edge of the bin for each dimension in the workspace
    CoordType m_max[nd];



  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDBIN_H_ */
