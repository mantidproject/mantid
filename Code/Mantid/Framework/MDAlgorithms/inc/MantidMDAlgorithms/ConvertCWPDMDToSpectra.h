#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertCWPDMDToSpectra : TODO: DESCRIPTION

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

  API::MatrixWorkspace_sptr
  reducePowderData(API::IMDEventWorkspace_const_sptr dataws,
                   API::IMDEventWorkspace_const_sptr monitorws,
                   const double min2theta, const double max2theta,
                   const double binsize, bool dolinearinterpolation);

  void binMD(API::IMDEventWorkspace_const_sptr mdws,
             const std::vector<double> &vecx, std::vector<double> &vecy);

  void linearInterpolation(API::MatrixWorkspace_sptr matrixws,
                           std::vector<bool> &vec0count);

  void setupSampleLogs(API::MatrixWorkspace_sptr matrixws,
                       API::IMDEventWorkspace_const_sptr inputmdws);

  void scaleMatrixWorkspace(API::MatrixWorkspace_sptr matrixws,
                            const double &scalefactor);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRA_H_ */
