// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_
#define MANTID_ALGORITHMS_CUBOIDGAUGEVOLUMEABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FlatPlateAbsorption.h"

namespace Mantid {
namespace Algorithms {
/** Calculates attenuation due to absorption and scattering in a generic sample,
   considering only the
    scattering within a cuboid shaped 'gauge volume'.

    This gauge volume will be an axis-aligned cuboid with its centre at the
   samplePos point. The
    sample object must have been previously defined and must fully enclose the
   gauge volume.

    @author Russell Taylor, Tessella
    @date 1/11/2010
*/
class DLLExport CuboidGaugeVolumeAbsorption : public FlatPlateAbsorption {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "CuboidGaugeVolumeAbsorption";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates bin-by-bin correction factors for attenuation due to "
           "absorption and (single) scattering within a cuboid shaped 'gauge "
           "volume' of a generic sample. The sample shape can be defined by, "
           "e.g., the CreateSampleShape algorithm.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }

private:
  std::string sampleXML() override;
  void initialiseCachedDistances() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_*/
