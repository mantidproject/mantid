#ifndef MANTID_ALGORITHMS_RINGPROFILE_H_
#define MANTID_ALGORITHMS_RINGPROFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"
#include "MantidAPI/ISpectrum.h"
#include <vector>

namespace Mantid {
namespace Algorithms {

/**
  Calculates the sum of the counts against a circular ring.

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
class DLLExport RingProfile : public API::Algorithm {
public:
  RingProfile();
  virtual ~RingProfile();

  virtual const std::string name() const { return "RingProfile"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculates the sum of the counts against a circular ring.";
  }

  virtual int version() const { return 1; };
  virtual const std::string category() const { return "Transforms"; };

protected:
  boost::shared_ptr<API::Progress> m_progress;

private:
  void init();
  void exec();
  /// get the bin position for the given angle
  int fromAngleToBin(double angle, bool degree = true);
  /// validate the inputs of the algorithm for instrument based workspace
  void checkInputsForSpectraWorkspace(const API::MatrixWorkspace_sptr);
  /// validate the inputs of the algorithm for 2d matrix based instrument
  void checkInputsForNumericWorkspace(const API::MatrixWorkspace_sptr);
  /// process ring profile for instrument based workspace
  void processInstrumentRingProfile(const API::MatrixWorkspace_sptr inputWS,
                                    std::vector<double> &output_bins);
  /// process ring profile for image based workspace
  void processNumericImageRingProfile(const API::MatrixWorkspace_sptr inputWS,
                                      std::vector<double> &output_bins);
  /// identify the bin position for the given pixel in the image based workspace
  void getBinForPixel(const API::MatrixWorkspace_sptr, int, std::vector<int> &);
  //// identify the bin position for the pixel related to the given detector
  int getBinForPixel(Mantid::Geometry::IDetector_const_sptr);
  /// copy of the minRadius input
  double min_radius;
  /// copy of the maxRadius input
  double max_radius;
  /// copy of the StartAngle input
  double start_angle;
  /// flag that indicate if Sense input was configure to clockwise
  bool clockwise;
  /// copy of NumBins input
  int num_bins;
  /// copy of centre for axis(0)
  double centre_x;
  /// copy of centre for axis(1)
  double centre_y;
  /// copy of centre for axis(2)
  double centre_z;
  /// The size of the bins in angle that is equal to 360 / NumBins
  double bin_size;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RINGPROFILE_H_ */
