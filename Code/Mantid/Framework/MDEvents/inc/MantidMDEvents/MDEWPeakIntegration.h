#ifndef MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_
#define MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace MDEvents
{

  /** Integrate single-crystal peaks in reciprocal-space.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-13 18:11:53.496539
   */
  class DLLExport MDEWPeakIntegration  : public API::Algorithm
  {
  public:
    MDEWPeakIntegration();
    ~MDEWPeakIntegration();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MDEWPeakIntegration";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
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

#endif  /* MANTID_MDEVENTS_MDEWPEAKINTEGRATION_H_ */
