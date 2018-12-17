// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_
#define MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

// forward declarations
namespace HistogramData {
class Points;
}
namespace Geometry {
class IDetector;
class IObject;
class ParameterMap;
} // namespace Geometry

namespace Algorithms {
/**
    Corrects the input workspace for helium3 tube efficiency based on an
    exponential parameterization. The algorithm expects the input workspace
    units to be wavelength. The formula for the efficiency is given here.

    \f[
    \epsilon = \frac{A}{1-e^{\frac{-\alpha P (L - 2W) \lambda}{T sin(\theta)}}}
    \f]

    where \f$A\f$ is a dimensionless scaling factor, \f$\alpha\f$ is a constant
    with units \f$(Kelvin / (metres\: \mbox{\AA}\: atm))\f$, \f$P\f$ is pressure
   in
    units of \f$atm\f$, \f$L\f$ is the tube diameter in units of \f$metres\f$,
    \f$W\f$ is the tube thickness in units of \f$metres\f$, \f$T\f$ is the
    temperature in units of \f$Kelvin\f$, \f$sin(\theta)\f$ is the angle of
    the neutron trajectory with respect to the long axis of the He3 tube and
    \f$\lambda\f$ is in units of \f$\mbox{\AA}\f$.

    @author Michael Reuter
    @date 30/09/2010
*/
class DLLExport He3TubeEfficiency : public API::Algorithm {
public:
  /// Default constructor
  He3TubeEfficiency();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "He3TubeEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "He3 tube efficiency correction.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"DetectorEfficiencyCor"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  void execEvent();

  /// Calculates the efficiency correction from the points
  void computeEfficiencyCorrection(std::vector<double> &effCorrection,
                                   const HistogramData::Points &wavelength,
                                   const double expConstant,
                                   const double scale) const;
  /// Correct the given spectra index for efficiency
  void correctForEfficiency(std::size_t spectraIndex,
                            const API::SpectrumInfo &spectrumInfo);
  /// Sets the detector geometry cache if necessary
  void getDetectorGeometry(const Geometry::IDetector &det, double &detRadius,
                           Kernel::V3D &detAxis);
  /// Computes the distance to the given shape from a starting point
  double distToSurface(const Kernel::V3D start,
                       const Geometry::IObject *shape) const;
  /// Calculate the detector efficiency
  double detectorEfficiency(const double alpha,
                            const double scale_factor = 1.0) const;
  /// Log any errors with spectra that occurred
  void logErrors() const;
  /// Retrieve the detector parameters from workspace or detector properties
  double getParameter(std::string wsPropName, std::size_t currentIndex,
                      std::string detPropName, const Geometry::IDetector &idet);
  /// Helper for event handling
  template <class T> void eventHelper(std::vector<T> &events, double expval);
  /// Function to calculate exponential contribution
  double calculateExponential(std::size_t spectraIndex,
                              const Geometry::IDetector &idet);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// Map that stores additional properties for detectors
  const Geometry::ParameterMap *m_paraMap;
  /// A lookup of previously seen shape objects used to save calculation time as
  /// most detectors have the same shape
  std::map<const Geometry::IObject *, std::pair<double, Kernel::V3D>>
      m_shapeCache;
  /// Sample position
  Kernel::V3D m_samplePos;
  /// The spectra numbers that were skipped
  std::vector<specnum_t> m_spectraSkipped;
  /// Algorithm progress keeper
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_ */
