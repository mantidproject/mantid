#ifndef MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventInserter.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertCWSDExpToMomentum : TODO: DESCRIPTION

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
class DLLExport ConvertCWSDExpToMomentum : API::Algorithm {
public:
  ConvertCWSDExpToMomentum();
  virtual ~ConvertCWSDExpToMomentum();

private:
  void init();
  void exec();

  void addMDEvents();

  void convertSpiceMatrixToMomentumMDEvents(API::MatrixWorkspace_sptr dataws,
                                            const detid_t &startdetid,
                                            const int runnumber);

  API::IMDEventWorkspace_sptr createExperimentMDWorkspace();

  bool getInputs(std::string &errmsg);

  API::MatrixWorkspace_sptr loadSpiceData(const std::string &filename,
                                          bool &loaded, std::string &errmsg);

  void parseDetectorTable(std::vector<Kernel::V3D> &vec_detpos,
                          std::vector<detid_t> &vec_detid);

  API::ITableWorkspace_sptr m_expDataTableWS;
  API::ITableWorkspace_sptr m_detectorListTableWS;
  API::IMDEventWorkspace_sptr m_outputWS;

  Kernel::V3D m_samplePos;
  Kernel::V3D m_sourcePos;

  size_t m_iColFilename;
  size_t m_iColStartDetID;

  size_t m_numBins;

  std::vector<double> m_extentMins;
  std::vector<double> m_extentMaxs;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_ */
