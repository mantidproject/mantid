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
  FileBackedExperimentInfo(::NeXus::File *file, const std::string & path);

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

private:

  void checkAndPopulate() const;
  void populateFromFile() const;

  mutable bool m_empty;
  ::NeXus::File *m_file;
  std::string m_path;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
