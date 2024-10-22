// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <fstream>
#include <string>

#include "MantidKernel/ConfigService.h"

#include <Poco/Path.h>

namespace ScopedFileHelper {
/** File object type. Provides exception save file creation/destruction.

This is a resource management type. Objects of this type are limited to stack
allocation.
*/
class ScopedFile {
public:
  ScopedFile(const std::string &fileContents, const std::string &fileName);
  ScopedFile(const std::string &fileContents, const std::string &fileName, const std::string &fileDirectory);
  ScopedFile &operator=(const ScopedFile &other);
  ScopedFile(const ScopedFile &other);
  void release() const;
  std::string getFileName() const;
  ~ScopedFile();

private:
  void doCreateFile(const std::string &fileContents, const Poco::Path &fileNameAndPath);

  mutable std::string m_filename;
  std::ofstream m_file;
  // Following methods keeps us from being able to put objects of this type on
  // the heap.
  void *operator new(size_t);
  void operator delete(void *);
  void *operator new[](size_t);
  void operator delete[](void *);
};
} // namespace ScopedFileHelper
