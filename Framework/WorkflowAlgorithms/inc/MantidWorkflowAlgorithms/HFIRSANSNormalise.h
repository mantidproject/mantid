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
  /// (Empty) Constructor
  HFIRSANSNormalise() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~HFIRSANSNormalise() {}
  /// Algorithm's name
  virtual const std::string name() const { return "HFIRSANSNormalise"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Apply normalisation correction to HFIR SANS data.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HFIRSANSNORMALISE_H_*/
