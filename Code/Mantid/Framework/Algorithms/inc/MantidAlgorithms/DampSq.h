#ifndef MANTID_ALGORITHMS_DAMPSQ_H_
#define MANTID_ALGORITHMS_DAMPSQ_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** DampSq : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-25 13:32:12.202430
   */
  class DLLExport DampSq  : public API::Algorithm
  {
  public:
    DampSq();
    ~DampSq();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "DampSq";};
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
    /// damp function 1
    double dampcoeff1(double q, double qmax, double dqmax);
    double dampcoeff2(double q, double qmax, double dqmax);
    double dampcoeff3(double q, double qmax, double dqmax);
    double dampcoeff4(double q, double qmax, double dqmax);

  };


} // namespace Mantid
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_DAMPSQ_H_ */
