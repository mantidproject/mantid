#ifndef MANTID_DATAHANDLING_SETSCALINGPSD_H_
#define MANTID_DATAHANDLING_SETSCALINGPSD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** @class SetScalingPSD SetScalingPSD.h MantidAlgorithm/SetScalingPSD.h

    Read the scaling information from a file (e.g. merlin_detector.sca) or from
the RAW file (.raw)
    and adjusts the detectors positions and scaling appropriately.

    Required Properties:
    <UL>
    <LI> ScalingFilename - The path to the file containing the detector ositions
to use either .raw or .sca</LI>
    <LI> Workspace - The name of the workspace to adjust </LI>
    </UL>
    Optional Properties:
    <UL>
    <LI> scalingOption - 0 => use average of left and right scaling (default).
    1 => use maximum scaling.
    2 => maximum + 5%</LI>
    </UL>

@author Ronald Fowler

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SetScalingPSD : public API::Algorithm {
public:
  /// Default constructor
  SetScalingPSD();

  /// Destructor
  ~SetScalingPSD() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SetScalingPSD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "For an instrument with Position Sensitive Detectors (PSDs) the "
           "'engineering' positions of individual detectors may not match the "
           "true areas where neutrons are detected. This algorithm reads data "
           "on the calibrated location of the detectors and adjusts the "
           "parametrized instrument geometry accordingly.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  /// Overwrites Algorithm method.
  void init();

  /// Overwrites Algorithm method
  void exec();

  /// The name and path of the input file
  std::string m_filename;
  /// An integer option controlling the scaling method
  int m_scalingOption;
  bool processScalingFile(const std::string &scalingFile,
                          std::vector<Kernel::V3D> &truePos);
  API::MatrixWorkspace_sptr m_workspace; ///< Pointer to the workspace
  // void runMoveInstrumentComp(const int& detIndex, const Kernel::V3D& shift);

  /// get a vector of shared pointers to each detector in the comp
  void findAll(boost::shared_ptr<const Geometry::IComponent> comp);
  /// the vector of shared pointers
  std::vector<boost::shared_ptr<const Geometry::IComponent>> m_vectDet;
  /// apply the shifts in posMap to the detectors in WS
  void movePos(API::MatrixWorkspace_sptr &WS,
               std::map<int, Kernel::V3D> &posMap,
               std::map<int, double> &scaleMap);
  /// read the positions of detectors defined in the raw file
  void getDetPositionsFromRaw(std::string rawfile, std::vector<int> &detID,
                              std::vector<Kernel::V3D> &pos);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SETSCALINGPSD_H_*/
