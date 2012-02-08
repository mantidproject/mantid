#ifndef MANTID_CRYSTAL_LOADHKL_H_
#define MANTID_CRYSTAL_LOADHKL_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  /** LoadHKL : Load an ISAW-style .peaks file
   * into a PeaksWorkspace
   * 
   * @author Vickie Lynch, SNS
   * @date 2012-01-25
   */
  class DLLExport LoadHKL  : public API::Algorithm
  {
  public:
    LoadHKL();
    ~LoadHKL();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadHKL";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal;DataHandling\\Text";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_LOADHKL_H_ */
