// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif
#include <filesystem>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

/** IDFObject : File object wrapper over an IDF file.

This is essentially a file path wrapper with some extra convenience
methods allowing easy access to the parent directory path.
This type has the exists method as a virtual method to facilitate testing.
*/

/**
 * Abstract IDF Object
 */
class MANTID_GEOMETRY_DLL AbstractIDFObject {
public:
  AbstractIDFObject() = default;
  static const std::string expectedExtension();
  virtual std::filesystem::path getParentDirectory() const = 0;
  virtual const std::filesystem::path &getFileFullPath() const = 0;
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
class MANTID_GEOMETRY_DLL IDFObject : public AbstractIDFObject {
public:
  IDFObject(const std::string &fileName);
  std::filesystem::path getParentDirectory() const override;
  const std::filesystem::path &getFileFullPath() const override;
  const std::string &getFileFullPathStr() const override;
  std::string getFileNameOnly() const override;
  std::string getExtension() const override;
  std::string getMangledName() const override;
  bool exists() const override;

private:
  IDFObject(const IDFObject &);
  IDFObject &operator=(const IDFObject &);
  const bool m_hasFileName;
  const std::filesystem::path m_cachePath;
  const std::filesystem::path m_cacheParentDirectory;
  const std::string m_cachePathStr;
};

/*
 * NULL IDFObject
 */
class MANTID_GEOMETRY_DLL NullIDFObject : public AbstractIDFObject {
private:
  std::string m_emptyResponse;
  std::filesystem::path m_emptyPath;

public:
  NullIDFObject() : m_emptyResponse(""), m_emptyPath("") {}
  std::filesystem::path getParentDirectory() const override {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
  const std::filesystem::path &getFileFullPath() const override {
    throw std::runtime_error("Not implemented on NullIDFObject");
  }
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
