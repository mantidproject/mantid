#ifndef MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_
#define MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**

    Apply correction to EQSANS data to account for its TOF structure. The algorithm modifies the
    TOF values to correct for the fact that T_0 is not properly recorded by the DAS.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport EQSANSTofStructure : public API::Algorithm
{
public:
  /// (Empty) Constructor
  EQSANSTofStructure() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSTofStructure() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSTofStructure"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_*/
