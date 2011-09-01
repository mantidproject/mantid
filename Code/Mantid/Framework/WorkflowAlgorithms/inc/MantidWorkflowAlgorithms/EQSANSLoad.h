#ifndef MANTID_ALGORITHMS_EQSANSLOAD_H_
#define MANTID_ALGORITHMS_EQSANSLOAD_H_

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

    Subtract dark current for EQSANS.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> MinEfficiency - Minimum efficiency for a pixel to be considered (default: no minimum)</LI>
    <LI> MaxEfficiency - Maximum efficiency for a pixel to be considered (default: no maximum)</LI>
    <LI> Factor        - Exponential factor for detector efficiency as a function of wavelength (default: 1.0)</LI>
    <LI> Error         - Error on Factor property (default: 0.0)</LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
const double default_slit_positions[3][8] = {{5.0, 10.0, 10.0, 15.0, 20.0, 20.0, 25.0, 40.0},
                                             {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0},
                                             {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0}};

class DLLExport EQSANSLoad : public API::Algorithm
{
public:
  /// (Empty) Constructor
  EQSANSLoad() : API::Algorithm(), m_low_TOF_cut(0), m_high_TOF_cut(0),
  m_center_x(0), m_center_y(0), m_moderator_position(0) {
    m_mask_as_string = "";
    m_output_message = "";
    for(int i=0; i<3; i++)
      for(int j=0; j<8; j++) m_slit_positions[i][j] = default_slit_positions[i][j];

    // Slit to source distance in mm for the three slit wheels
    m_slit_to_source[0] = 10080;
    m_slit_to_source[1] = 11156;
    m_slit_to_source[2] = 12150;
  }
  /// Virtual destructor
  virtual ~EQSANSLoad() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSLoad"; }
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
  std::string findConfigFile(const int& run);
  void readConfigFile(const std::string& filePath);
  void readRectangularMasks(const std::string& line);
  void readTOFcuts(const std::string& line);
  void readBeamCenter(const std::string& line);
  void readModeratorPosition(const std::string& line);
  void readSourceSlitSize(const std::string& line);
  void getSourceSlitSize();
  void moveToBeamCenter();

  //std::vector< std::vector<int> > m_rectangular_masks;
  double m_low_TOF_cut;
  double m_high_TOF_cut;
  double m_center_x;
  double m_center_y;
  std::string m_mask_as_string;
  std::string m_output_message;
  double m_moderator_position;
  API::MatrixWorkspace_sptr dataWS;
  double m_slit_positions[3][8];
  int m_slit_to_source[3];
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSLOAD_H_*/
