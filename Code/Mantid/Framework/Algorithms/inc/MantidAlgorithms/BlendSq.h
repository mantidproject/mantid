#ifndef MANTID_ALGORITHMS_BLENDSQ_H_
#define MANTID_ALGORITHMS_BLENDSQ_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidKernel/ArrayProperty.h"

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
    virtual const std::string category() const { return "Diffraction";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    void rebinData(API::MatrixWorkspace_const_sptr sourcews, API::MatrixWorkspace_sptr targetws, double qmin, double qmax, double dq);

    // void RebinData(size_t sourcesize, double* sqs, double* sss, double* ses, size_t targetsize, double* tqs, double* tss, double* tes);

    void fillArray(API::MatrixWorkspace_sptr ws);

    void extendWorkspaceRange(API::MatrixWorkspace_sptr sourcews, API::MatrixWorkspace_sptr targetws, double qmin, double dq);

    void blendBanks(std::vector<API::MatrixWorkspace_sptr> sqwspaces, API::MatrixWorkspace_sptr blendworkspace, std::vector<double> lowerbounds, std::vector<double> upperbounds);

    void extendToZeroQ(API::MatrixWorkspace_sptr ws, double qmin);
  };


} // namespace Mantid
} // namespace Algorithms

#endif  /* MANTID_ALGORITHMS_BLENDSQ_H_ */
