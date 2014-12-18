#ifndef MANTID_ALGORITHMS_SANSBEAMFLUXCORRECTION_H_
#define MANTID_ALGORITHMS_SANSBEAMFLUXCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Performs beam flux correction on TOF SANS data.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSBeamFluxCorrection : public API::DataProcessorAlgorithm {
public:
  /// (Empty) Constructor
  SANSBeamFluxCorrection() : API::DataProcessorAlgorithm() {}
  /// Virtual destructor
  virtual ~SANSBeamFluxCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSBeamFluxCorrection"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Performs beam flux correction on TOF SANS data.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Workflow\\SANS\\UsesPropertyManager;"
           "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void execEvent();
  API::MatrixWorkspace_sptr loadReference();
  boost::shared_ptr<Kernel::PropertyManager> m_reductionManager;
  std::string m_output_message;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSBEAMFLUXCORRECTION_H_*/
