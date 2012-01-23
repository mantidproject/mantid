#ifndef MANTID_CRYSTAL_SORTHKL_H_
#define MANTID_CRYSTAL_SORTHKL_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
   * 
   * @author Vickie Lynch, SNS
   * @date 2012-01-20
   */

  class DLLExport SortHKL  : public API::Algorithm
  {
  public:
    SortHKL();
    ~SortHKL();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SortHKL";};
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
    void Outliers(std::vector<double>& data, std::vector<double>& err);

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_SORTHKL_H_ */
