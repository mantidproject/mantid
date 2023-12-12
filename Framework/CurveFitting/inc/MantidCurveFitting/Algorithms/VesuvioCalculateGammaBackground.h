// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDTypes.h"

#include <map>

namespace Mantid {

namespace Kernel {
class V3D;
}

namespace CurveFitting {
namespace Functions {
struct ResolutionParams;
}
namespace Algorithms {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
struct DetectorParams;

class MANTID_CURVEFITTING_DLL VesuvioCalculateGammaBackground final : public API::Algorithm {
public:
  VesuvioCalculateGammaBackground();
  ~VesuvioCalculateGammaBackground() override;

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the background due to gamma rays produced when neutrons "
           "are absorbed by shielding.";
  }
  const std::vector<std::string> seeAlso() const override { return {"VesuvioCorrections"}; }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// Hold information about a single foil
  struct FoilInfo {
    double thetaMin;
    double thetaMax;
    double lorentzWidth;
    double gaussWidth;
  };

  /// Avoid nested try catch in openmp loop by returning boolean if spectrum was
  /// corrceted
  bool calculateBackground(size_t inputIndex, size_t outputIndex);
  /// Calculate & correct the given index of the input workspace
  void applyCorrection(const size_t inputIndex, const size_t outputIndex);
  /// Compute the expected spectrum from a given detector
  void calculateSpectrumFromDetector(const size_t inputIndex, const size_t outputIndex);
  /// Compute the expected background from the foils
  void calculateBackgroundFromFoils(const size_t inputIndex, const size_t outputIndex);
  /// Compute expected background from single foil for spectrum at wsIndex
  void calculateBackgroundSingleFoil(std::vector<double> &ctfoil, const size_t wsIndex, const FoilInfo &foilInfo,
                                     const Kernel::V3D &detPos, const DetectorParams &detPar,
                                     const CurveFitting::Functions::ResolutionParams &detRes);
  /// Compute a TOF spectrum for the given inputs & spectrum
  std::vector<double> calculateTofSpectrum(const std::vector<double> &inSpectrum, std::vector<double> &tmpWork,
                                           const size_t wsIndex, const DetectorParams &detpar,
                                           const CurveFitting::Functions::ResolutionParams &respar);

  /// Check and store appropriate input data
  void retrieveInputs();
  /// Create the output workspaces
  void createOutputWorkspaces();
  /// Compute & store the parameters that are fixed during the correction
  void cacheInstrumentGeometry();
  /// Compute the theta range for a given foil
  std::pair<double, double> calculateThetaRange(const Geometry::IComponent_const_sptr &foilComp, const double radius,
                                                const unsigned int horizDir) const;

  /// Input TOF data
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// Sorted indices to correct
  std::map<size_t, size_t> m_indices;
  /// Function that defines the mass profile
  std::string m_profileFunction;
  /// The number of peaks in spectrum
  size_t m_npeaks;
  /// List of spectra numbers whose background sum is to be reversed
  std::set<specnum_t> m_reversed;

  /// Sample position
  Kernel::V3D m_samplePos;
  /// Source to sample distance
  double m_l1;
  /// Radius of (imaginary) circle that foils sit on
  double m_foilRadius;
  /// Minimum in up dir to start integration over foil volume
  double m_foilUpMin;
  /// Minimum in up dir to stop integration over foil volume
  double m_foilUpMax;

  /// Description of foils in the position 0
  std::vector<FoilInfo> m_foils0;
  /// Description of foils in the position 0
  std::vector<FoilInfo> m_foils1;
  /// Stores the value of the calculated background
  API::MatrixWorkspace_sptr m_backgroundWS;
  /// Stores the corrected data
  API::MatrixWorkspace_sptr m_correctedWS;

  /// Pointer to progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
