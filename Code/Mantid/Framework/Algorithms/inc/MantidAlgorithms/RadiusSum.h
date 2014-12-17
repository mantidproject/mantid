#ifndef MANTID_ALGORITHMS_RADIUSSUM_H_
#define MANTID_ALGORITHMS_RADIUSSUM_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** RadiusSum :

  Sum of all the counts inside a ring against the scattering angle for each
  Radius.

  @author Gesner Passos, ISIS

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport RadiusSum : public API::Algorithm {
public:
  RadiusSum();
  virtual ~RadiusSum();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Sum of all the counts inside a ring against the scattering angle "
           "for each Radius.";
  }

  virtual int version() const;
  virtual const std::string category() const;

  static bool inputWorkspaceHasInstrumentAssociated(API::MatrixWorkspace_sptr);

  static std::vector<double>
      getBoundariesOfNumericImage(API::MatrixWorkspace_sptr);

  static std::vector<double>
      getBoundariesOfInstrument(API::MatrixWorkspace_sptr);

  static void centerIsInsideLimits(const std::vector<double> &centre,
                                   const std::vector<double> &boundaries);

private:
  void init();
  void exec();

  std::vector<double> processInstrumentRadiusSum();
  std::vector<double> processNumericImageRadiusSum();

  void cacheInputPropertyValues();
  void inputValidationSanityCheck();
  void numBinsIsReasonable();
  std::vector<double> getBoundariesOfInputWorkspace();

  double getMaxDistance(const Kernel::V3D &centre,
                        const std::vector<double> &boundary_limits);

  void setUpOutputWorkspace(std::vector<double> &output);

  int getBinForPixelPos(const Kernel::V3D &pos);

  Kernel::V3D centre;
  int num_bins;

  API::MatrixWorkspace_sptr inputWS;
  double min_radius, max_radius;

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

  void normalizeOutputByRadius(std::vector<double> &output, double exp_power);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RADIUSSUM_H_ */
