#ifndef MANTID_ALGORITHMS_PDFFourierTransform_H_
#define MANTID_ALGORITHMS_PDFFourierTransform_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** PDFFourierTransform : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-22 13:55:27.513553
   */
  class DLLExport PDFFourierTransform  : public API::Algorithm
  {
  public:
    PDFFourierTransform();
    ~PDFFourierTransform();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const;
    /// Algorithm's version for identification 
    virtual int version() const;
    /// Algorithm's category for identification
    virtual const std::string category() const;  // category better be in diffraction than general
    /// @copydoc Algorithm::validateInputs()
    virtual std::map<std::string, std::string> validateInputs();

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
    double CalculateGrFromQ(double r, double& egr, double qmin, double qmax, bool sofq);
    /// Calculate PDF, i.e., G(r), for a certain r value from S(d)
    double CalculateGrFromD(double r, double& egr, double qmin, double qmax, bool sofq);
  };


} // namespace Mantid
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_PDFFourierTransform_H_ */
