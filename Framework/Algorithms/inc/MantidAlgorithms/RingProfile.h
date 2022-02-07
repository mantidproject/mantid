// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDetector.h"
#include <vector>

namespace Mantid {
namespace Algorithms {

/**
  Calculates the sum of the counts against a circular ring.

  @author Gesner Passos, ISIS
*/
class MANTID_ALGORITHMS_DLL RingProfile : public API::Algorithm {
public:
  RingProfile();

  const std::string name() const override { return "RingProfile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the sum of the counts against a circular ring."; }

  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LineProfile"}; }
  const std::string category() const override { return "Transforms\\Grouping"; };

protected:
  std::shared_ptr<API::Progress> m_progress;

private:
  void init() override;
  void exec() override;
  /// get the bin position for the given angle
  int fromAngleToBin(double angle, bool degree = true);
  /// validate the inputs of the algorithm for instrument based workspace
  void checkInputsForSpectraWorkspace(const API::MatrixWorkspace_sptr &);
  /// validate the inputs of the algorithm for 2d matrix based instrument
  void checkInputsForNumericWorkspace(const API::MatrixWorkspace_sptr &);
  /// process ring profile for instrument based workspace
  void processInstrumentRingProfile(const API::MatrixWorkspace_sptr &inputWS, std::vector<double> &output_bins);
  /// process ring profile for image based workspace
  void processNumericImageRingProfile(const API::MatrixWorkspace_sptr &inputWS, std::vector<double> &output_bins);
  /// identify the bin position for the given pixel in the image based workspace
  void getBinForPixel(const API::MatrixWorkspace_sptr &, int, std::vector<int> &);
  //// identify the bin position for the pixel related to the given detector
  int getBinForPixel(const Kernel::V3D &position);
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
