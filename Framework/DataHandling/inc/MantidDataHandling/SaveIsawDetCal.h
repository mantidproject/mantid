// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument_fwd.h"

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace DataHandling {

/** Saves an instrument with RectangularDetectors to an ISAW .DetCal file.

  @author Janik Zikovsky
  @date 2011-10-03
*/
class MANTID_DATAHANDLING_DLL SaveIsawDetCal final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawDetCal"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves an instrument with RectangularDetectors to an ISAW .DetCal "
           "file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadIsawDetCal"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Isaw"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// find position for rectangular and non-rectangular
  Kernel::V3D findPixelPos(const std::string &bankName, int col, int row, const Geometry::ComponentInfo &componentInfo);
  void sizeBanks(const std::string &bankName, int &NCOLS, int &NROWS, double &xsize, double &ysize,
                 const Geometry::ComponentInfo &componentInfo);
  Geometry::Instrument_const_sptr inst;
};

} // namespace DataHandling
} // namespace Mantid
