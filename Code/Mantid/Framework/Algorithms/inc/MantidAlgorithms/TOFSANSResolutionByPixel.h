#ifndef MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_
#define MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

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
  /// (Empty) Constructor
  TOFSANSResolutionByPixel() : API::Algorithm() {}
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
  /// Wavelength resolution (constant for all wavelengths)
  double wl_resolution;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TOFSANSRESOLUTIONBYPIXEL_H_*/
