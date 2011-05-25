#ifndef MANTID_CRYSTAL_LOADISAWUB_H_
#define MANTID_CRYSTAL_LOADISAWUB_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** Algorithm to load an ISAW-style ASCII UB matrix and lattice
   * parameters file, and place its information into
   * a workspace.
   * 
   * @author Janik Zikovsky
   * @date 2011-05-25
   */
  class DLLExport LoadIsawUB  : public API::Algorithm
  {
  public:
    LoadIsawUB();
    ~LoadIsawUB();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadIsawUB";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal";}
    
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

#endif  /* MANTID_CRYSTAL_LOADISAWUB_H_ */
