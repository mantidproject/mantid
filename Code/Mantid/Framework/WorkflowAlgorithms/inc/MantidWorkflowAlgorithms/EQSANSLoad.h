#ifndef MANTID_ALGORITHMS_EQSANSLOAD_H_
#define MANTID_ALGORITHMS_EQSANSLOAD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Subtract dark current for EQSANS.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport EQSANSLoad : public API::Algorithm {
public:
  /// Constructor
  EQSANSLoad()
      : API::Algorithm(), m_low_TOF_cut(0), m_high_TOF_cut(0), m_center_x(0),
        m_center_y(0), m_moderator_position(0) {
    m_mask_as_string = "";
    m_output_message = "";
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 8; j++)
        m_slit_positions[i][j] = EQSANSInstrument::default_slit_positions[i][j];

    // Slit to source distance in mm for the three slit wheels
    m_slit_to_source[0] = 10080;
    m_slit_to_source[1] = 11156;
    m_slit_to_source[2] = 12150;
  }
  /// Virtual destructor
  virtual ~EQSANSLoad() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSLoad"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const { return "Load EQSANS data."; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  std::string findConfigFile(const int &run);
  void readConfigFile(const std::string &filePath);
  void readRectangularMasks(const std::string &line);
  void readTOFcuts(const std::string &line);
  void readBeamCenter(const std::string &line);
  void readModeratorPosition(const std::string &line);
  void readSourceSlitSize(const std::string &line);
  void getSourceSlitSize();
  void moveToBeamCenter();

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
