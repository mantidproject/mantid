// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RADIUSSUM_H_
#define MANTID_ALGORITHMS_RADIUSSUM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

/** RadiusSum :

  Sum of all the counts inside a ring against the scattering angle for each
  Radius.

  @author Gesner Passos, ISIS
*/
class DLLExport RadiusSum : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sum of all the counts inside a ring against the scattering angle "
           "for each Radius.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RingProfile", "RadiusSum"};
  }
  const std::string category() const override;

  static bool inputWorkspaceHasInstrumentAssociated(API::MatrixWorkspace_sptr);

  static std::vector<double>
      getBoundariesOfNumericImage(API::MatrixWorkspace_sptr);

  static std::vector<double>
      getBoundariesOfInstrument(API::MatrixWorkspace_sptr);

  static void centerIsInsideLimits(const std::vector<double> &centre,
                                   const std::vector<double> &boundaries);

private:
  void init() override;
  void exec() override;

  std::vector<double> processInstrumentRadiusSum();
  std::vector<double> processNumericImageRadiusSum();

  void cacheInputPropertyValues();
  void inputValidationSanityCheck();
  void numBinsIsReasonable();
  std::vector<double> getBoundariesOfInputWorkspace();

  double getMaxDistance(const Kernel::V3D &centre,
                        const std::vector<double> &boundary_limits);

  void setUpOutputWorkspace(const std::vector<double> &values);

  int getBinForPixelPos(const Kernel::V3D &pos);

  Kernel::V3D centre;
  int num_bins = 0;

  API::MatrixWorkspace_sptr inputWS;
  double min_radius = 0.0, max_radius = 0.0;

  double getMinBinSizeForInstrument(API::MatrixWorkspace_sptr);
  double getMinBinSizeForNumericImage(API::MatrixWorkspace_sptr);

  /** Return the bin position for a given distance
   *  From the input, it is defined the limits of distances as:
   *  Dominium => [min_radius, max_radius]
   *
   *  Besides, it is defined that this dominium is splited in num_bins.
   *
   *  Hence, each bin will follow this rule:
   *
   *  Bins(n) = [min_radius + n bin_size, min_radius + (n+1) bin_size[
   *
   *  for bin_size = (max_radius - min_radius ) / num_bins
   *
   *  For a distance given as x, its position (n) is such as:
   *
   *  min_radius + n bin_size < x < min_radius + (n+1) bin_size
   *
   *  n < (x - min_radius)/bin_size < n+1
   *
   * Hence, n = truncate ( (x- min_radius)/bin_size)
   *
   * @note Made inline for performance
   * return bin position.
   */
  int fromDistanceToBin(double distance) {
    return static_cast<int>(((distance - min_radius) * num_bins) /
                            (max_radius - min_radius));
  }

  void normalizeOutputByRadius(std::vector<double> &values, double exp_power);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RADIUSSUM_H_ */
