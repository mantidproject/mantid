#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace MDEvents
{

  /** BinToMDHistoWorkspace : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-29 11:28:06.048254
   */
  class DLLExport BinToMDHistoWorkspace  : public API::Algorithm
  {
  public:
    BinToMDHistoWorkspace();
    ~BinToMDHistoWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "BinToMDHistoWorkspace";};
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

#endif  /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_ */
