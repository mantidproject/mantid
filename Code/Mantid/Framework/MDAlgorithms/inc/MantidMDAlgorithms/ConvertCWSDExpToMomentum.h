#ifndef MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidKernel/Matrix.h"

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
class DLLExport ConvertCWSDExpToMomentum : public API::Algorithm {
public:
  ConvertCWSDExpToMomentum();
  virtual ~ConvertCWSDExpToMomentum();

  /// Algorithm's name
  virtual const std::string name() const { return "ConvertCWSDExpToMomentum"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load and convert a set of files in an HB3A experiment.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;DataHandling\\Text";
  }

private:
  void init();
  void exec();

  void addMDEvents();

  void convertSpiceMatrixToMomentumMDEvents(API::MatrixWorkspace_sptr dataws,
                                            const detid_t &startdetid,
                                            const int runnumber);

  /// Convert |Q| with detector position to Q_sample
  Kernel::V3D convertToQSample(const Kernel::V3D &samplePos,
                               const Kernel::V3D &ki, const Kernel::V3D &detPos,
                               const double &momentum,
                               std::vector<Mantid::coord_t> &qSample,
                               const Kernel::DblMatrix &rotationMatrix);

  API::IMDEventWorkspace_sptr createExperimentMDWorkspace();

  bool getInputs(std::string &errmsg);

  API::MatrixWorkspace_sptr loadSpiceData(const std::string &filename,
                                          bool &loaded, std::string &errmsg);

  void parseDetectorTable(std::vector<Kernel::V3D> &vec_detpos,
                          std::vector<detid_t> &vec_detid);

  void setupTransferMatrix(API::MatrixWorkspace_sptr dataws,
                           Kernel::DblMatrix &rotationMatrix);

  API::ITableWorkspace_sptr m_expDataTableWS;
  API::ITableWorkspace_sptr m_detectorListTableWS;
  API::IMDEventWorkspace_sptr m_outputWS;
  Geometry::Instrument_sptr m_virtualInstrument;

  Kernel::V3D m_samplePos;
  Kernel::V3D m_sourcePos;

  size_t m_iColFilename;
  size_t m_iColStartDetID;

  std::vector<double> m_extentMins;
  std::vector<double> m_extentMaxs;
  std::vector<size_t> m_numBins;

  /// Data directory
  std::string m_dataDir;
  /// Flag to use m_dataDir
  bool m_isBaseName;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_ */
