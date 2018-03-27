#ifndef MANTID_GEOMETRY_IDFOBJECT_H_
#define MANTID_GEOMETRY_IDFOBJECT_H_

#include "MantidKernel/System.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <Poco/File.h>
#include <Poco/Path.h>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

/** IDFObject : File object wrapper over an IDF file.

This is essentially an adapter for a Poco::File, with some extra convenience
methods allowing easy access to the parent directory path.
This type has the last modified date and the exists method as a virtual methods
to facilite testing.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/**
 * Abstract IDF Object
 */
class DLLExport AbstractIDFObject {
public:
  AbstractIDFObject() = default;
  static const std::string expectedExtension();
  virtual const Poco::Path getParentDirectory() const = 0;
  virtual const Poco::Path &getFileFullPath() const = 0;
  virtual const std::string &getFileFullPathStr() const = 0;
  virtual std::string getFileNameOnly() const = 0;
  virtual std::string getExtension() const = 0;
  virtual std::string getMangledName() const = 0;
  virtual bool exists() const = 0;
  virtual ~AbstractIDFObject() = default;

private:
  AbstractIDFObject(const AbstractIDFObject &);
  AbstractIDFObject &operator=(const AbstractIDFObject &);
};

/**
 * Concrete IDF Object.
 */
class DLLExport IDFObject : public AbstractIDFObject {
public:
  IDFObject(const std::string &fileName);
  const Poco::Path getParentDirectory() const override;
  const Poco::Path &getFileFullPath() const override;
  const std::string &getFileFullPathStr() const override;
  std::string getFileNameOnly() const override;
  std::string getExtension() const override;
  std::string getMangledName() const override;
  bool exists() const override;

private:
  IDFObject(const IDFObject &);
  IDFObject &operator=(const IDFObject &);
  const Poco::File m_defFile;
  const bool m_hasFileName;
  const Poco::Path m_cachePath;
  const Poco::Path m_cacheParentDirectory;
  const std::string m_cachePathStr;
};

/*
 * NULL IDFObject
 */
class DLLExport NullIDFObject : public AbstractIDFObject {
private:
  std::string m_emptyResponse;

public:
  NullIDFObject() : m_emptyResponse("") {}
  const Poco::Path getParentDirectory() const override {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  const Poco::Path &getFileFullPath() const override {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  const std::string &getFileFullPathStr() const override {
    return m_emptyResponse;
  }
  std::string getFileNameOnly() const override { return m_emptyResponse; }
  std::string getExtension() const override { return m_emptyResponse; }
  std::string getMangledName() const override {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  bool exists() const override { return false; }
};

using IDFObject_sptr = boost::shared_ptr<AbstractIDFObject>;
using IDFObject_const_sptr = boost::shared_ptr<const AbstractIDFObject>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_IDFOBJECT_H_ */
