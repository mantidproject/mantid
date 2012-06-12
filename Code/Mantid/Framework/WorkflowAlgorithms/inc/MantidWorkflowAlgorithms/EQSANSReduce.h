#ifndef MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_
#define MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
class DLLExport EQSANSReduce : public API::DataProcessorAlgorithm
{
public:
  /// (Empty) Constructor
  EQSANSReduce() : API::DataProcessorAlgorithm() {}
  /// Virtual destructor
  virtual ~EQSANSReduce() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSReduce"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void initializeReduction();
  void performReduction(API::Workspace_sptr workspace);
  API::Workspace_sptr postProcess(API::Workspace_sptr workspace);
  API::Workspace_sptr loadInputData();

};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_*/
