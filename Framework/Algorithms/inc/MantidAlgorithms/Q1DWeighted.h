// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_Q1DWEIGHTED_H_
#define MANTID_ALGORITHMS_Q1DWEIGHTED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/**
    Part of data reduction for SANS. Transforms a 2D detector array into I(Q) by
   assigning each
    2D pixel to a Q bin. The contribution of each pixel is weighted by either 1
   or a function of the error
    on the signal in that pixel.

    The weighting of the pixel by the error follows the HFIR/SANS reduction code
   implemented in IGOR
    by Ken Littrell, ORNL.

    Choosing to weight each pixel by 1 gives I(q) where each bin is the average
   of all pixels contributing to
    that bin.

    Each pixel is sub-divided in NPixelDivision*NPixelDivision sub-pixels when
   averaging.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The (partly) corrected data in units of wavelength.
   </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result
   histogram. </LI>
    <LI> OutputBinning     - The bin parameters to use for the final result.
   </LI>
    <LI> NPixelDivision    - The number of pixel sub-divisions in each direction
   used for calculation. </LI>
    <LI> PixelSizeX        - The pixel size, in meter, in the X direction.</LI>
    <LI> PixelSizeY        - The pixel size, in meter, in the Y direction. </LI>
    <LI> ErrorWeighting    - Whether to weight pixel contribution by their error
   (default: false). </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Q1DWeighted : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Q1DWeighted"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs azimuthal averaging on a 2D SANS data to produce I(Q).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Q1D"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Create an output workspace
  API::MatrixWorkspace_sptr
  createOutputWorkspace(API::MatrixWorkspace_const_sptr, const size_t,
                        const std::vector<double> &);

  void bootstrap(API::MatrixWorkspace_const_sptr);
  void calculate(API::MatrixWorkspace_const_sptr);
  void finalize(API::MatrixWorkspace_const_sptr);

  std::vector<std::vector<std::vector<double>>> m_intensities;
  std::vector<std::vector<std::vector<double>>> m_errors;
  std::vector<std::vector<std::vector<double>>> m_normalisation;
  std::vector<double> m_qBinEdges;
  size_t m_nQ;
  size_t m_nLambda;
  size_t m_nWedges;
  size_t m_nSpec;
  int m_nSubPixels;
  double m_wedgeFullAngle;
  double m_wedgeOffset;
  double m_wedgeAngle;
  double m_pixelSizeY;
  double m_pixelSizeX;
  bool m_asymmWedges;
  bool m_errorWeighting;
  bool m_correctGravity;

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1DWEIGHTED_H_*/
