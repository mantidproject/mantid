#ifndef MANTID_ALGORITHMS_HFIRSANSNORMALISE_H_
#define MANTID_ALGORITHMS_HFIRSANSNORMALISE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {

class DLLExport HFIRSANSNormalise : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "HFIRSANSNormalise"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Apply normalisation correction to HFIR SANS data.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HFIRSANSNORMALISE_H_*/
