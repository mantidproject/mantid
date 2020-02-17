// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAMPLEENVIRONMENTFACTORY_H_
#define MANTID_DATAHANDLING_SAMPLEENVIRONMENTFACTORY_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/SampleEnvironmentSpec.h"

namespace Mantid {
namespace DataHandling {

/**
 * Interface for a class responsible for finding a specification based on a
 * name.
 */
class MANTID_DATAHANDLING_DLL ISampleEnvironmentSpecFinder {
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
class MANTID_DATAHANDLING_DLL SampleEnvironmentFactory {
public:
  SampleEnvironmentFactory() = default;
  SampleEnvironmentFactory(ISampleEnvironmentSpecFinder_uptr specFinder);

  Geometry::SampleEnvironment_uptr create(const std::string &facility,
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
class MANTID_DATAHANDLING_DLL SampleEnvironmentSpecFileFinder final
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

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAMPLEENVIRONMENTFACTORY_H_ */
