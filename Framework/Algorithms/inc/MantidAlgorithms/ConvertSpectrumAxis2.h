#ifndef MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_
#define MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
namespace API {
class MatrixWorkspace;
}
namespace Algorithms {
/** Converts the representation of the vertical axis (the one up the side of
    a matrix in MantidPlot) of a Workspace2D from its default of holding the
    spectrum number to the target unit given - theta, elastic Q and elastic Q^2.

    The spectra will be reordered by increasing theta and duplicates will not be
   aggregated.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> Target          - The unit to which the spectrum axis should be
   converted. </LI>
    </UL>

        Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertSpectrumAxis2 : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertSpectrumAxis"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the axis of a Workspace2D which normally holds spectrum "
           "numbers to one of Q, Q^2 or theta.  'Note': After running this "
           "algorithm, some features will be unavailable on the workspace as "
           "it will have lost all connection to the instrument. This includes "
           "things like the 3D Instrument Display.";
  }

  /// Algorithm's version
  int version() const override { return (2); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertAxesToRealSpace", "ConvertUnits"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Units;Transforms\\Axes";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Converting to theta.
  void createThetaMap(API::Progress &progress, const std::string &targetUnit,
                      API::MatrixWorkspace_sptr &inputWS);
  /// Converting to Q and QSquared
  void createElasticQMap(API::Progress &progress, const std::string &targetUnit,
                         API::MatrixWorkspace_sptr &inputWS);
  /// Creates an output workspace.
  API::MatrixWorkspace_sptr
  createOutputWorkspace(API::Progress &progress, const std::string &targetUnit,
                        API::MatrixWorkspace_sptr &inputWS);

  /// Map to which the conversion to the unit is stored.
  std::multimap<double, size_t> m_indexMap;

  /// Vector of axis in case ordering is not asked.
  std::vector<double> m_axis;

  /// Flag whether ordering is needed.
  bool m_toOrder;

  /// Emplaces to value and the index pair into the map.
  void emplaceIndexMap(double value, size_t wsIndex);

  /// Getting Efixed
  double getEfixed(const size_t detectorIndex,
                   const Mantid::Geometry::DetectorInfo &detectorInfo,
                   const API::MatrixWorkspace &inputWS, const int emode) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_*/
