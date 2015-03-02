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
  FileBackedExperimentInfo(::NeXus::File *file, const std::string &path);

  ExperimentInfo *cloneExperimentInfo() const;

  const std::string toString() const;

  Geometry::Instrument_const_sptr getInstrument() const;

  const Geometry::ParameterMap &instrumentParameters() const;

  Geometry::ParameterMap &instrumentParameters();

  const Geometry::ParameterMap &constInstrumentParameters() const;

  void populateInstrumentParameters();

  void replaceInstrumentParameters(const Geometry::ParameterMap &pmap);

  void swapInstrumentParameters(Geometry::ParameterMap &pmap);

  void cacheDetectorGroupings(const det2group_map &mapping);

  const std::vector<detid_t> &getGroupMembers(const detid_t detID) const;

  Geometry::IDetector_const_sptr getDetectorByID(const detid_t detID) const;

  void setModeratorModel(ModeratorModel *source);

  ModeratorModel &moderatorModel() const;

  void setChopperModel(ChopperModel *chopper, const size_t index = 0);

  ChopperModel &chopperModel(const size_t index = 0) const;

  const Sample &sample() const;

  Sample &mutableSample();

  const Run &run() const;

  Run &mutableRun();

  Kernel::Property *getLog(const std::string &log) const;

  double getLogAsSingleValue(const std::string &log) const;

  int getRunNumber() const;

  Kernel::DeltaEMode::Type getEMode() const;

  double getEFixed(const detid_t detID) const;

  double getEFixed(const Geometry::IDetector_const_sptr detector =
                       Geometry::IDetector_const_sptr()) const;

  void setEFixed(const detid_t detID, const double value);

private:
  void populateIfNotLoaded() const;
  void populateFromFile() const;

  mutable bool m_loaded;
  ::NeXus::File *m_file;
  std::string m_path;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
