#ifndef MANTID_MDEVENTS_MDCENTROIDPEAKS_H_
#define MANTID_MDEVENTS_MDCENTROIDPEAKS_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace MDEvents
{

  /** Find the centroid of single-crystal peaks in a MDEventWorkspace, in order to refine their positions.
   * 
   * @author Janik Zikovsky
   * @date 2011-06-01
   */
  class DLLExport MDCentroidPeaks  : public API::Algorithm
  {
  public:
    MDCentroidPeaks();
    ~MDCentroidPeaks();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MDCentroidPeaks";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDCENTROIDPEAKS_H_ */
