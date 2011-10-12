#ifndef MANTID_ALGORITHMS_HFIRLOAD_H_
#define MANTID_ALGORITHMS_HFIRLOAD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**

    Subtract dark current for HFIR SANS.

    Required Properties:
    <UL>
    <LI> Filename - The name of the input event Nexus file to load </LI>
    <LI> OutputWorkspace - Then name of the output EventWorkspace </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> UseConfigBeam - If true, the beam center defined in the configuration file will be used</LI>
    <LI> BeamCenterX - Beam position in X pixel coordinates (used only if UseConfigBeam is false)</LI>
    <LI> BeamCenterY        - Beam position in Y pixel coordinates (used only if UseConfigBeam is false)</LI>
    <LI> UseConfigTOFCuts         - If true, the edges of the TOF distribution will be cut according to the configuration file</LI>
    <LI> UseConfigMask         - If true, the masking information found in the configuration file will be used</LI>
    <LI> UseConfig         - If true, the best configuration file found will be used)</LI>
    <LI> CorrectForFlightPath         - If true, the TOF will be modified for the true flight path from the sample to the detector pixel</LI>
    <LI> SampleDetectorDistance         - Sample to detector distance to use (overrides meta data), in mm</LI>
    <LI> SampleDetectorDistanceOffset         - Offset to the sample to detector distance (use only when using the distance found in the meta data), in mm</LI>
    </UL>

    Output Property:
    <UL>
    <LI> OutputMessage - Human readable output message </LI>
    </UL>


    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport HFIRLoad : public API::Algorithm
{
public:
  /// Constructor
  HFIRLoad() : API::Algorithm(), m_center_x(0), m_center_y(0), m_moderator_position(0) {
    m_mask_as_string = "";
    m_output_message = "";
  }
  /// Virtual destructor
  virtual ~HFIRLoad() {}
  /// Algorithm's name
  virtual const std::string name() const { return "HFIRLoad"; }
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
  void moveToBeamCenter();

  double m_center_x;
  double m_center_y;
  std::string m_mask_as_string;
  std::string m_output_message;
  double m_moderator_position;
  API::MatrixWorkspace_sptr dataWS;
  double m_slit_positions[3][8];
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_HFIRLOAD_H_*/
