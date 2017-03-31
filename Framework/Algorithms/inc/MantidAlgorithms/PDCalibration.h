#ifndef MANTID_ALGORITHMS_PDCALIBRATION_H_
#define MANTID_ALGORITHMS_PDCALIBRATION_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include <map>

namespace Mantid {
namespace Algorithms {

/** PDCalibration : TODO: DESCRIPTION

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
class MANTID_ALGORITHMS_DLL PDCalibration : public API::Algorithm {
public:
  PDCalibration();
  ~PDCalibration();

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  class FittedPeaks; // forward declare of private inner class

  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  API::MatrixWorkspace_sptr loadAndBin();
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr wksp);
  API::MatrixWorkspace_sptr load(const std::string filename);
  void loadOldCalibration();
  void createNewCalTable();
  std::function<double(double)> getDSpacingToTof(const detid_t detid);
  std::vector<double> dSpacingWindows(const std::vector<double> &centres,
                                      const double widthMax);
  std::vector<double> getTOFminmax(const double difc, const double difa,
                                   const double tzero);
  void setCalibrationValues(const detid_t detid, const double difc,
                            const double difa, const double tzero);
  void fitDIFCtZeroDIFA(const std::vector<double> &d,
                        const std::vector<double> &tof, double &difc,
                        double &t0, double &difa);
  API::MatrixWorkspace_sptr m_uncalibratedWS;
  API::ITableWorkspace_sptr m_calibrationTable;
  std::vector<double> m_peaksInDspacing;
  std::string calParams;
  std::map<detid_t, size_t> m_detidToRow;
  double m_tofMin{0.};
  double m_tofMax{0.};
  double m_tzeroMin{0.};
  double m_tzeroMax{0.};
  double m_difaMin{0.};
  double m_difaMax{0.};
  bool m_hasDasIds{false};
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PDCALIBRATION_H_ */
