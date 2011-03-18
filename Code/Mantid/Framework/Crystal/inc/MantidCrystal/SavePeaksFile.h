#ifndef MANTID_CRYSTAL_SAVEPEAKSFILE_H_
#define MANTID_CRYSTAL_SAVEPEAKSFILE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** SavePeaksFile : Save a PeaksWorkspace to a .peaks text-format file.
   * 
   * @author Janik Zikovsky
   * @date 2011-03-18 14:18:59.523067
   */
  class DLLExport SavePeaksFile  : public API::Algorithm
  {
  public:
    SavePeaksFile();
    ~SavePeaksFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SavePeaksFile";};
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

#endif  /* MANTID_CRYSTAL_SAVEPEAKSFILE_H_ */
