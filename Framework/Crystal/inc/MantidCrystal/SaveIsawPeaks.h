// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {

namespace Crystal {

/** Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.
 *
 * @author Janik Zikovsky
 * @date 2011-05-25
 */
class MANTID_CRYSTAL_DLL SaveIsawPeaks final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawPeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a PeaksWorkspace to a ISAW-style ASCII .peaks file."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadIsawPeaks"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Isaw"; }

private:
  /// Flag for writing modulated structures
  bool m_isModulatedStructure = false;

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// find position for rectangular and non-rectangular
  Kernel::V3D findPixelPos(const std::string &bankName, int col, int row);
  void sizeBanks(const std::string &bankName, int &NCOLS, int &NROWS, double &xsize, double &ysize);
  bool bankMasked(const Geometry::IComponent_const_sptr &parent, const Geometry::DetectorInfo &detectorInfo);
  void writeOffsets(std::ofstream &out, double qSign, const std::vector<double> &offset);
  Geometry::Instrument_const_sptr inst;
};

} // namespace Crystal
} // namespace Mantid
