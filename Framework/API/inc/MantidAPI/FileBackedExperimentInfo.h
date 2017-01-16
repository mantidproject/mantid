#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFO_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFO_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

namespace Mantid {
namespace API {

/**
    Implements a lazy-loading mechanism for the experimental information
    stored in a NeXus file.

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
class MANTID_API_DLL FileBackedExperimentInfo : public ExperimentInfo {
public:
  /// Constructor
  FileBackedExperimentInfo(const std::string &filename,
                           const std::string &nxpath);

  ExperimentInfo *cloneExperimentInfo() const override;

  const std::string toString() const override;

  Geometry::Instrument_const_sptr getInstrument() const override;

  const Geometry::ParameterMap &instrumentParameters() const override;

  Geometry::ParameterMap &instrumentParameters() override;

  const Geometry::ParameterMap &constInstrumentParameters() const override;

  void populateInstrumentParameters() override;

  void replaceInstrumentParameters(const Geometry::ParameterMap &pmap) override;

  void swapInstrumentParameters(Geometry::ParameterMap &pmap) override;

  void cacheDetectorGroupings(const det2group_map &mapping) override;

  const std::set<detid_t> &getGroupMembers(const detid_t detID) const override;

  Geometry::IDetector_const_sptr
  getDetectorByID(const detid_t detID) const override;

  void setModeratorModel(ModeratorModel *source) override;

  ModeratorModel &moderatorModel() const override;

  void setChopperModel(ChopperModel *chopper, const size_t index = 0) override;

  ChopperModel &chopperModel(const size_t index = 0) const override;

  const Sample &sample() const override;

  Sample &mutableSample() override;

  const Run &run() const override;

  Run &mutableRun() override;

  Kernel::Property *getLog(const std::string &log) const override;

  double getLogAsSingleValue(const std::string &log) const override;

  int getRunNumber() const override;

  Kernel::DeltaEMode::Type getEMode() const override;

  double getEFixed(const detid_t detID) const override;

  double getEFixed(const Geometry::IDetector_const_sptr detector =
                       Geometry::IDetector_const_sptr()) const override;

  void setEFixed(const detid_t detID, const double value) override;

  size_t numberOfDetectorGroups() const override;
  const std::set<detid_t> &
  detectorIDsInGroup(const size_t index) const override;

private:
  void populateIfNotLoaded() const;
  void populateFromFile() const;

  mutable bool m_loaded;
  std::string m_filename;
  std::string m_nxpath;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
