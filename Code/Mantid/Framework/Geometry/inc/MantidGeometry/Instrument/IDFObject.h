#ifndef MANTID_GEOMETRY_IDFOBJECT_H_
#define MANTID_GEOMETRY_IDFOBJECT_H_

#include "MantidKernel/System.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <Poco/Timestamp.h>
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
  AbstractIDFObject() {}
  static const std::string expectedExtension();
  virtual const Poco::Path &getParentDirectory() const = 0;
  virtual const Poco::Path &getFileFullPath() const = 0;
  virtual const std::string &getFileFullPathStr() const = 0;
  virtual std::string getFileNameOnly() const = 0;
  virtual std::string getExtension() const = 0;
  virtual Poco::Timestamp getLastModified() const = 0;
  virtual std::string getFormattedLastModified() const = 0;
  virtual std::string getMangledName() const = 0;
  virtual bool exists() const = 0;
  virtual ~AbstractIDFObject(){};

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
  virtual const Poco::Path &getParentDirectory() const;
  virtual const Poco::Path &getFileFullPath() const;
  virtual const std::string &getFileFullPathStr() const;
  virtual std::string getFileNameOnly() const;
  virtual std::string getExtension() const;
  virtual Poco::Timestamp getLastModified() const;
  virtual std::string getFormattedLastModified() const;
  virtual std::string getMangledName() const;
  virtual bool exists() const;
  virtual ~IDFObject();

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
  virtual const Poco::Path &getParentDirectory() const {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  virtual const Poco::Path &getFileFullPath() const {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  virtual const std::string &getFileFullPathStr() const {
    return m_emptyResponse;
  }
  virtual std::string getFileNameOnly() const { return m_emptyResponse; }
  virtual std::string getExtension() const { return m_emptyResponse; }
  virtual Poco::Timestamp getLastModified() const {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  virtual std::string getFormattedLastModified() const {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  virtual std::string getMangledName() const {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  virtual bool exists() const { return false; }
  virtual ~NullIDFObject(){};
};

typedef boost::shared_ptr<AbstractIDFObject> IDFObject_sptr;
typedef boost::shared_ptr<const AbstractIDFObject> IDFObject_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_IDFOBJECT_H_ */
