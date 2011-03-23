#ifndef MANTID_ALGORITHMS_PDFFT_H_
#define MANTID_ALGORITHMS_PDFFT_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** PDFFT : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-22 13:55:27.513553
   */
  class DLLExport PDFFT  : public API::Algorithm
  {
  public:
    PDFFT();
    ~PDFFT();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "PDFFT";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    API::MatrixWorkspace_const_sptr Sspace;
    API::MatrixWorkspace_sptr Gspace;

    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();
    /// Calculate PDF, i.e., G(r), for a certain r value from S(Q)
    double CalculateGrFromQ(double r, double& egr);
    /// Calculate PDF, i.e., G(r), for a certain r value from S(d)
    double CalculateGrFromD(double r, double& egr);

  };


} // namespace Mantid
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_PDFFT_H_ */
