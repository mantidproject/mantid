#ifndef MANTID_ALGORITHMS_BLENDSQ_H_
#define MANTID_ALGORITHMS_BLENDSQ_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** BlendSq : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-29 11:09:32.897883
   */
  class DLLExport BlendSq  : public API::Algorithm
  {
  public:
    BlendSq();
    ~BlendSq();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "BlendSq";};
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
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_BLENDSQ_H_ */
