#ifndef MANTID_ALGORITHMS_PDCALIBRATION_H_
#define MANTID_ALGORITHMS_PDCALIBRATION_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
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
  virtual ~PDCalibration();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();
  void loadAndBin();
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr wksp);
  API::MatrixWorkspace_sptr load(const std::string filename);
  void loadOldCalibration();
  std::vector<double> dSpacingToTof(const std::vector<double> &dSpacing,
                                    const detid_t id);

  API::MatrixWorkspace_sptr m_uncalibratedWS;
  API::ITableWorkspace_sptr m_calibrationTableNew;
  API::ITableWorkspace_sptr m_calibrationTableOld;
  std::vector<double> m_peaksInDspacing;
  std::map<detid_t, size_t> m_detidToRow;
  double m_tofMin;
  double m_tofMax;
  bool m_hasDasIds;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PDCALIBRATION_H_ */
