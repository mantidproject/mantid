#ifndef MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

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

  /// Algorithm's name
  const std::string name() const override { return "ConvertCWSDExpToMomentum"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load and convert a set of files in an HB3A experiment.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\ConstantWavelength;DataHandling\\Text";
  }

private:
  void init() override;
  void exec() override;

  void addMDEvents(bool usevirtual);

  void convertSpiceMatrixToMomentumMDEvents(
      API::MatrixWorkspace_sptr dataws, bool usevirtual,
      const detid_t &startdetid, const int scannumber, const int runnumber,
      double measuretime, int monitor_counts);

  /// Convert |Q| with detector position to Q_sample
  Kernel::V3D convertToQSample(const Kernel::V3D &samplePos,
                               const Kernel::V3D &ki, const Kernel::V3D &detPos,
                               const double &momentum,
                               std::vector<Mantid::coord_t> &qSample,
                               const Kernel::DblMatrix &rotationMatrix);

  API::IMDEventWorkspace_sptr createExperimentMDWorkspace();

  bool getInputs(bool virtualinstrument, std::string &errmsg);

  API::MatrixWorkspace_sptr loadSpiceData(const std::string &filename,
                                          bool &loaded, std::string &errmsg);

  void parseDetectorTable(std::vector<Kernel::V3D> &vec_detpos,
                          std::vector<detid_t> &vec_detid);

  void setupTransferMatrix(API::MatrixWorkspace_sptr dataws,
                           Kernel::DblMatrix &rotationMatrix);

  void createVirtualInstrument();

  void updateQRange(const std::vector<Mantid::coord_t> &vec_q);

  /// Remove background from
  void removeBackground(API::MatrixWorkspace_sptr dataws);

  API::ITableWorkspace_sptr m_expDataTableWS;
  API::ITableWorkspace_sptr m_detectorListTableWS;
  API::IMDEventWorkspace_sptr m_outputWS;
  Geometry::Instrument_sptr m_virtualInstrument;

  /// Shifts in detector position set from user (calibration): all in the unit
  /// as meter
  double m_detSampleDistanceShift;
  double m_detXShift;
  double m_detYShift;

  Kernel::V3D m_samplePos;
  Kernel::V3D m_sourcePos;

  size_t m_iColScan;
  size_t m_iColPt;
  size_t m_iColFilename;
  size_t m_iColStartDetID;
  size_t m_iMonitorCounts;
  size_t m_iTime;

  std::vector<double> m_extentMins;
  std::vector<double> m_extentMaxs;
  std::vector<size_t> m_numBins;

  std::vector<coord_t> m_minQVec;
  std::vector<coord_t> m_maxQVec;
  bool m_setQRange;

  /// Data directory
  std::string m_dataDir;
  /// Flag to use m_dataDir
  bool m_isBaseName;
  /// Background workspace
  bool m_removeBackground;
  API::MatrixWorkspace_const_sptr m_backgroundWS;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUM_H_ */
