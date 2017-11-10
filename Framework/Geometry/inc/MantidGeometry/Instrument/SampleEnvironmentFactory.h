#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORY_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORY_H_
/**
  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/SampleEnvironmentSpec.h"

namespace Mantid {
namespace Geometry {

/**
 * Interface for a class responsible for finding a specification based on a
 * name.
 */
class MANTID_GEOMETRY_DLL ISampleEnvironmentSpecFinder {
public:
  virtual ~ISampleEnvironmentSpecFinder() = default;
  virtual SampleEnvironmentSpec_uptr find(const std::string &facility,
                                          const std::string &instrument,
                                          const std::string &name) const = 0;
};
// Typedef for a unique_ptr
using ISampleEnvironmentSpecFinder_uptr =
    std::unique_ptr<ISampleEnvironmentSpecFinder>;

/**
  Create a single instance of a SampleEnvironment. It requires the name
  of a sample environment specification and a can name.

  The specifications are cached in a static lookup. The class is implemented
  using the monostate pattern
*/
class MANTID_GEOMETRY_DLL SampleEnvironmentFactory {
public:
  SampleEnvironmentFactory() = default;
  SampleEnvironmentFactory(ISampleEnvironmentSpecFinder_uptr specFinder);

  SampleEnvironment_uptr create(const std::string &facility,
                                const std::string &instrument,
                                const std::string &specName,
                                const std::string &canName);

  size_t cacheSize() const;
  void clearCache();

private:
  ISampleEnvironmentSpecFinder_uptr m_finder;
};

//------------------------------------------------------------------------------
// SampleEnvironmentSpecFileFinder
//------------------------------------------------------------------------------
/**
 * Class responsible for finding a specifications on disk.
 */
class MANTID_GEOMETRY_DLL SampleEnvironmentSpecFileFinder final
    : public ISampleEnvironmentSpecFinder {
public:
  SampleEnvironmentSpecFileFinder(const std::vector<std::string> &directories);

  SampleEnvironmentSpec_uptr find(const std::string &facility,
                                  const std::string &instrument,
                                  const std::string &name) const override;

private:
  SampleEnvironmentSpec_uptr parseSpec(const std::string &name,
                                       const std::string &filename) const;

  const std::string m_fileext = ".xml";
  const std::vector<std::string> m_rootDirs;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTFACTORY_H_ */
