#ifndef MANTID_ALGORITHMS_HFIRLOAD_H_
#define MANTID_ALGORITHMS_HFIRLOAD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Subtract dark current for HFIR SANS.

    Required Properties:
    <UL>
    <LI> Filename - The name of the input event Nexus file to load </LI>
    <LI> OutputWorkspace - Then name of the output EventWorkspace </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> BeamCenterX - Beam position in X pixel coordinates (used only if
   UseConfigBeam is false)</LI>
    <LI> BeamCenterY        - Beam position in Y pixel coordinates (used only if
   UseConfigBeam is false)</LI>
    <LI> SampleDetectorDistance         - Sample to detector distance to use
   (overrides meta data), in mm</LI>
    <LI> SampleDetectorDistanceOffset         - Offset to the sample to detector
   distance (use only when using the distance found in the meta data), in
   mm</LI>
    </UL>

    Output Property:
    <UL>
    <LI> OutputMessage - Human readable output message </LI>
    </UL>


    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport HFIRLoad : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "HFIRLoad"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load HFIR SANS data."; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void moveToBeamCenter(API::MatrixWorkspace_sptr &dataWS, double &center_x,
                        double &center_y);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HFIRLOAD_H_*/
