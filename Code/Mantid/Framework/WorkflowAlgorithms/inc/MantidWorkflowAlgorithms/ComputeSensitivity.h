#ifndef MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_
#define MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
class DLLExport ComputeSensitivity : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ComputeSensitivity() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ComputeSensitivity() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ComputeSensitivity"; }
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
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_*/
