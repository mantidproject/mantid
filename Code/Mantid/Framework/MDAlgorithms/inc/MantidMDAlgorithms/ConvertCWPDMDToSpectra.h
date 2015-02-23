#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

//----------------------------------------------------------------------------------------------
/** Calculate d-space value from detector's position (2theta/theta) and
 * wavelength
 * @brief calculateD
 * @param theta
 * @param wavelength
 * @return
 */
inline double calculateDspaceFrom2Theta(const double &twotheta,
                                        const double &wavelength) {
  return (0.5 * wavelength / sin(twotheta * 0.5 * M_PI / 180.));
}

//----------------------------------------------------------------------------------------------
/** Calcualte detector's 2theta value from d-spacing value
 * 2theta = 2*asin(lamba/2/d)
 * @brief calculate2ThetaFromD
 * @param dspace
 * @param wavelengh
 * @return
 */
inline double calculate2ThetaFromD(const double &dspace,
                                   const double &wavelength) {
  return (2.0 * asin(0.5 * wavelength / dspace) * 180. / M_PI);
}

//----------------------------------------------------------------------------------------------
/** Calculate Q value from detector's positin (2theta/theta) and wavelength
 * q = 2 k \sin(\theta) = \frac{4 \pi}{\lambda} \sin(\theta).
 * @brief calculateQ
 * @param theta
 * @param wavelength
 * @return
 */
inline double calculateQFrom2Theta(const double &twotheta,
                                   const double &wavelength) {
  return (4. * M_PI * sin(twotheta * 0.5 * M_PI / 180.) / wavelength);
}

//----------------------------------------------------------------------------------------------
/** Calculate detector's 2theta value from Q
 * 2theta = 2*asin(lambda*Q/(4pi))
 * @brief calcualte2ThetaFromQ
 * @param q
 * @param wavelength
 * @return
 */
inline double calcualte2ThetaFromQ(const double &q, const double &wavelength) {
  return (2.0 * asin(q * wavelength * 0.25 / M_PI) * 180. / M_PI);
}

//----------------------------------------------------------------------------------------------
/** Calculate 2theta value of detector postion from sample position
 * @brief calculate2Theta
 * @param detpos
 * @param samplepos
 * @return
 */
inline double calculate2Theta(const Kernel::V3D &detpos,
                              const Kernel::V3D &samplepos) {
  return detpos.angle(samplepos);
}

/** ConvertCWPDMDToSpectra : Convert one MDWorkspaces containing reactor-source
  powder diffractometer's data to single spectrum matrix workspace
  by merging and binning the detectors' counts by their 2theta value.


  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertCWPDMDToSpectra : public API::Algorithm {
public:
  ConvertCWPDMDToSpectra();

  virtual ~ConvertCWPDMDToSpectra();

  /// Algorithm's name
  virtual const std::string name() const { return "ConvertCWPDMDToSpectra"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Binning the constant wavelength powder diffracton data stored in "
           "MDWorkspaces to single spectrum.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;MDAlgorithms";
  }

private:
  /// Initialisation code
  void init();

  /// Execution code
  void exec();

  /// Main algorithm to reduce powder diffraction data
  API::MatrixWorkspace_sptr reducePowderData(
      API::IMDEventWorkspace_const_sptr dataws,
      API::IMDEventWorkspace_const_sptr monitorws, const std::string targetunit,
      const std::map<int, double> &map_runwavelength, const double xmin,
      const double xmax, const double binsize, bool dolinearinterpolation);

  /// Bin signals to its 2theta position
  void binMD(API::IMDEventWorkspace_const_sptr mdws, const char &unitbit,
             const std::map<int, double> &map_runlambda,
             const std::vector<double> &vecx, std::vector<double> &vecy);

  /// Do linear interpolation to zero counts if bin is too small
  void linearInterpolation(API::MatrixWorkspace_sptr matrixws,
                           const double &infinitesimal);

  /// Set up sample logs
  void setupSampleLogs(API::MatrixWorkspace_sptr matrixws,
                       API::IMDEventWorkspace_const_sptr inputmdws);

  /// Scale reduced data
  void scaleMatrixWorkspace(API::MatrixWorkspace_sptr matrixws,
                            const double &scalefactor,
                            const double &infinitesimal);

  /// Convert units from 2theta to d-spacing or Q
  void convertUnits(API::MatrixWorkspace_sptr matrixws,
                    const std::string &targetunit, const double &wavelength);

  /// Infinitesimal value
  double m_infitesimal;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_ */
