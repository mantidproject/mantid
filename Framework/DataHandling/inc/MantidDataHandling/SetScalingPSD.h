// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace Geometry {
class IComponent;
}

namespace Kernel {
class V3D;
}

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
*/
class DLLExport SetScalingPSD : public API::Algorithm {
public:
  /// Default constructor
  SetScalingPSD();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SetScalingPSD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "For an instrument with Position Sensitive Detectors (PSDs) the "
           "'engineering' positions of individual detectors may not match the "
           "true areas where neutrons are detected. This algorithm reads data "
           "on the calibrated location of the detectors and adjusts the "
           "parametrized instrument geometry accordingly.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\InstrumentCorrections"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of the input file
  std::string m_filename;
  /// An integer option controlling the scaling method
  int m_scalingOption;
  bool processScalingFile(const std::string &scalingFile, std::vector<Kernel::V3D> &truepos);
  API::MatrixWorkspace_sptr m_workspace; ///< Pointer to the workspace
  // void runMoveInstrumentComp(const int& detIndex, const Kernel::V3D& shift);

  /// apply the shifts in posMap to the detectors in WS
  void movePos(API::MatrixWorkspace_sptr &WS, std::map<int, Kernel::V3D> &posMap, std::map<int, double> &scaleMap);
  /// read the positions of detectors defined in the raw file
  void getDetPositionsFromRaw(const std::string &rawfile, std::vector<int> &detID, std::vector<Kernel::V3D> &pos);
};

} // namespace DataHandling
} // namespace Mantid
