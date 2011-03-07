#ifndef MANTID_CRYSTAL_LOADPEAKSFILE_H_
#define MANTID_CRYSTAL_LOADPEAKSFILE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** LoadPeaksFile : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-07 15:22:11.897153
   */
  class DLLExport LoadPeaksFile  : public API::Algorithm
  {
  public:
    LoadPeaksFile();
    ~LoadPeaksFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadPeaksFile";};
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
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_LOADPEAKSFILE_H_ */
