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
  /// Virtual destructor
  virtual ~TOFSANSResolutionByPixel() {}
  /// Algorithm's name
  virtual const std::string name() const { return "TOFSANSResolutionByPixel"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the Q resolution for TOF SANS data for each pixel.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
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
  getModeratorWorkspace(Mantid::API::MatrixWorkspace_sptr inWS);
  /// Create an output workspace
  Mantid::API::MatrixWorkspace_sptr
  setupOutputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);
  /// Wavelength resolution (constant for all wavelengths)
  double m_wl_resolution;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_*/
