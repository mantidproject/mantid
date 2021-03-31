// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace MDAlgorithms {

//----------------------------------------------------------------------------------------------
/** Calculate d-space value from detector's position (2theta/theta) and
 * wavelength
 * @brief calculateDspaceFrom2Theta
 * @param twotheta
 * @param wavelength
 * @return
 */
inline double calculateDspaceFrom2Theta(const double &twotheta, const double &wavelength) {
  return (0.5 * wavelength / sin(twotheta * 0.5 * M_PI / 180.));
}

//----------------------------------------------------------------------------------------------
/** Calcualte detector's 2theta value from d-spacing value
 * 2theta = 2*asin(lamba/2/d)
 * @brief calculate2ThetaFromD
 * @param dspace
 * @param wavelength
 * @return
 */
inline double calculate2ThetaFromD(const double &dspace, const double &wavelength) {
  return (2.0 * asin(0.5 * wavelength / dspace) * 180. / M_PI);
}

//----------------------------------------------------------------------------------------------
/** Calculate Q value from detector's positin (2theta/theta) and wavelength
 * q = 2 k sin(theta) = (4 pi)/(lambda) * sin(theta)
 * @brief calculateQ
 * @param twotheta
 * @param wavelength
 * @return
 */
inline double calculateQFrom2Theta(const double &twotheta, const double &wavelength) {
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
inline double calculate2Theta(const Kernel::V3D &detpos, const Kernel::V3D &samplepos) {
  return detpos.angle(samplepos);
}

/** ConvertCWPDMDToSpectra : Convert one MDWorkspaces containing reactor-source
  powder diffractometer's data to single spectrum matrix workspace
  by merging and binning the detectors' counts by their 2theta value.
*/
class DLLExport ConvertCWPDMDToSpectra : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertCWPDMDToSpectra"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Convert constant wavelength (CW) powder diffraction (PD) data in "
           "MDEventWorksapces "
           " to a single-spectrum MatrixWorkspace, i.e., binning the "
           "diffraction data "
           " to single spectrum according to neutron's scattering angle, "
           "d-spacing or Q. ";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\ConstantWavelength;Diffraction\\Focussing"; }

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Main algorithm to reduce powder diffraction data
  API::MatrixWorkspace_sptr reducePowderData(const API::IMDEventWorkspace_const_sptr &dataws,
                                             const API::IMDEventWorkspace_const_sptr &monitorws,
                                             const std::string &targetunit,
                                             const std::map<int, double> &map_runwavelength, const double xmin,
                                             const double xmax, const double binsize, bool dolinearinterpolation,
                                             const std::vector<detid_t> &vec_excludeddets);

  /// Find the binning boundary according to detectors' positions
  void findXBoundary(const API::IMDEventWorkspace_const_sptr &dataws, const std::string &targetunit,
                     const std::map<int, double> &map_runwavelength, double &xmin, double &xmax);

  /// Bin signals to its 2theta position
  void binMD(const API::IMDEventWorkspace_const_sptr &mdws, const char &unitbit,
             const std::map<int, double> &map_runlambda, const std::vector<double> &vecx, std::vector<double> &vecy,
             const std::vector<detid_t> &vec_excludedet);

  /// Do linear interpolation to zero counts if bin is too small
  void linearInterpolation(const API::MatrixWorkspace_sptr &matrixws, const double &infinitesimal);

  /// Set up sample logs
  void setupSampleLogs(const API::MatrixWorkspace_sptr &matrixws, const API::IMDEventWorkspace_const_sptr &inputmdws);

  /// Scale reduced data
  void scaleMatrixWorkspace(const API::MatrixWorkspace_sptr &matrixws, const double &scalefactor,
                            const double &infinitesimal);

  /// Convert units from 2theta to d-spacing or Q
  void convertUnits(API::MatrixWorkspace_sptr matrixws, const std::string &targetunit, const double &wavelength);

  bool isExcluded(const std::vector<detid_t> &vec_excludedet, const detid_t detid);

  /// Infinitesimal value
  double m_infitesimal = 1.0E-10;
};

} // namespace MDAlgorithms
} // namespace Mantid
