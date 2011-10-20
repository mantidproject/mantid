#ifndef MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_
#define MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_
/*WIKI* 


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**

    Sensitivity correction for SANS

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> Filename      - Data file to use as dark current</LI>
    <LI> ReductionTableWorkspace - Table workspace to use to keep track of the reduction</LI>
    <LI> OutputDarkCurrentWorkspace - Name of dark current workspace</LI>
    <LI> OutputMessage - Human readable output message</LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSSensitivityCorrection : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SANSSensitivityCorrection() : API::Algorithm() {
    m_output_message = "";
  }
  /// Virtual destructor
  virtual ~SANSSensitivityCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSSensitivityCorrection"; }
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

  std::string m_output_message;

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_*/
