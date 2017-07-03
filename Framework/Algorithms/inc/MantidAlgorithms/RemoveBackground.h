#ifndef MANTID_ALGORITHMS_REMOVEBACKGROUND_H_
#define MANTID_ALGORITHMS_REMOVEBACKGROUND_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
}
// forward declarations from other Mantid modules
namespace Kernel {
class Unit;
}

namespace API {
class SpectrumInfo;
}

namespace Algorithms {
/** Performs removal of constant (and possibly non-constant after simple
modification) background calculated in TOF units
from a matrix workspace, expressed in units, different from TOF.

@date 26/10/2014


Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

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

/**Class actually performing background removal from a workspace spectra */
class DLLExport BackgroundHelper {
public:
  BackgroundHelper();
  ~BackgroundHelper();

  void initialize(const API::MatrixWorkspace_const_sptr &bkgWS,
                  const API::MatrixWorkspace_sptr &sourceWS, int emode,
                  Kernel::Logger *pLog = nullptr, int nThreads = 1,
                  bool inPlace = true, bool nullifyNegative = false);
  void removeBackground(int nHist, HistogramData::HistogramX &x_data,
                        HistogramData::HistogramY &y_data,
                        HistogramData::HistogramE &e_data,
                        int threadNum = 0) const;

private:
  // vector of pointers to the units conversion class for the working workspace;
  std::vector<Kernel::Unit *> m_WSUnit;

  // shared pointer to the workspace containing background
  API::MatrixWorkspace_const_sptr m_bgWs;
  // shared pointer to the workspace where background should be removed
  API::MatrixWorkspace_const_sptr m_wkWS;

  const API::SpectrumInfo *m_spectrumInfo;

  // logger from the hosting algorithm
  Kernel::Logger *m_pgLog;
  // perform background removal in-place
  bool m_inPlace;

  // if the background workspace is single value workspace
  bool m_singleValueBackground;
  // average number of counts at background for first spectra of a background
  // workspace
  double m_NBg;
  // time interval for measuring the background
  double m_dtBg;
  // Squared error of the background for first spectra of the background
  // workspace
  double m_ErrSq;
  // energy conversion mode
  int m_Emode;
  // incident for direct or analysis for indirect energy for units conversion
  double m_Efix;
  // if true, negative signals are nullified
  bool m_nullifyNegative;
  // removing negative values from ws with background removed previously.
  bool m_previouslyRemovedBkgMode;

  // get Ei attached to direct or indirect instrument workspace
  double getEi(const API::MatrixWorkspace_const_sptr &inputWS) const;
  // the procedure user to delete existing unit converter pointers
  void deleteUnitsConverters();
};

class DLLExport RemoveBackground : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveBackground"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes background (constant for now) calculated in TOF units "
           "from a matrix workspace, expressed in units, different from TOF";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "CorrectionFunctions\\BackgroundCorrections";
  }

protected:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

private:
  // class responsible for background removal
  BackgroundHelper m_BackgroundHelper;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_REBIN_H_*/
