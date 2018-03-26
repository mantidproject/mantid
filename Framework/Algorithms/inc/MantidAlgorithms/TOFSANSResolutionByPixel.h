#ifndef MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_
#define MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/**
    Calculates the TOF-SANS Q-resolution for each wavelenght and pixel using
   formula
    by Mildner and Carpenter.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport TOFSANSResolutionByPixel : public API::Algorithm {
public:
  /// Default constructor
  TOFSANSResolutionByPixel();
  /// Algorithm's name
  const std::string name() const override { return "TOFSANSResolutionByPixel"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the Q resolution for TOF SANS data for each pixel.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"TOFSANSResolution"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Return the TOF resolution for a particular wavelength
  virtual double getTOFResolution(double wl);
  /// Get the collimation length when we evaluate it using 5 Guards
  double
  getCollimationLengthWithGuard(Mantid::API::MatrixWorkspace_sptr inWS,
                                const double L1,
                                const double collimationLengthCorrection) const;
  /// Return the default collimation length
  double provideDefaultLCollimationLength(
      Mantid::API::MatrixWorkspace_sptr inWS) const;
  /// Check input
  void checkInput(Mantid::API::MatrixWorkspace_sptr inWS);
  /// Get the moderator workspace
  Mantid::API::MatrixWorkspace_sptr
  getModeratorWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);
  /// Create an output workspace
  Mantid::API::MatrixWorkspace_sptr
  setupOutputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);
  /// Wavelength resolution (constant for all wavelengths)
  double m_wl_resolution;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_*/
