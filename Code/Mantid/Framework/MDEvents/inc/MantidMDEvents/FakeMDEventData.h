#ifndef MANTID_MDEVENTS_FAKEMDEVENTDATA_H_
#define MANTID_MDEVENTS_FAKEMDEVENTDATA_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace MDEvents
{

  /** FakeMDEventData : Algorithm to create fake multi-dimensional event
   * data that gets added to MDEventWorkspace, for use in testing.
   * 
   * @author Janik Zikovsky
   * @date 2011-03-30 13:13:10.349627
   */
  class DLLExport FakeMDEventData  : public API::Algorithm
  {
  public:
    FakeMDEventData();
    ~FakeMDEventData();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "FakeMDEventData";};
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

    template<typename MDE, size_t nd>
    void addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws);


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_FAKEMDEVENTDATA_H_ */
