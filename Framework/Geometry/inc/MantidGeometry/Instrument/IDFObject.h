// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#ifndef Q_MOC_RUN
#include <memory>
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
  const Poco::Path getParentDirectory() const override { throw std::runtime_error("Not implemented on NullIDFObject"); }
  const Poco::Path &getFileFullPath() const override { throw std::runtime_error("Not implemented on NullIDFObject"); }
  const std::string &getFileFullPathStr() const override { return m_emptyResponse; }
  std::string getFileNameOnly() const override { return m_emptyResponse; }
  std::string getExtension() const override { return m_emptyResponse; }
  std::string getMangledName() const override { throw std::runtime_error("Not implemented on NullIDFObject"); }
  bool exists() const override { return false; }
};

using IDFObject_sptr = std::shared_ptr<AbstractIDFObject>;
using IDFObject_const_sptr = std::shared_ptr<const AbstractIDFObject>;

} // namespace Geometry
} // namespace Mantid
