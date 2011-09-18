#ifndef MANTID_MDEVENTS_ONESTEPMDEW_H_
#define MANTID_MDEVENTS_ONESTEPMDEW_H_
/*WIKI* 

This algorithm is used in the Paraview event nexus loader to both load an event nexus file and convert it into a [[MDEventWorkspace]] for use in visualization.

The [[LoadEventNexus]] algorithm is called with default parameters to load into an [[EventWorkspace]].

After, the [[ConvertToDiffractionMDWorkspace]] algorithm is called with the new EventWorkspace as input. The parameters are set to convert to Q in the lab frame, with Lorentz correction, and default size/splitting behavior parameters.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace MDEvents
{

  /** OneStepMDEW : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-04-06 10:19:10.284945
   */
  class DLLExport OneStepMDEW  : public API::Algorithm
  {
  public:
    OneStepMDEW();
    ~OneStepMDEW();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "OneStepMDEW";};
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

#endif  /* MANTID_MDEVENTS_ONESTEPMDEW_H_ */
