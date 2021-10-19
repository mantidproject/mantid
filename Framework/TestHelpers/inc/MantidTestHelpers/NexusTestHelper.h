// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This file MAY NOT be modified to use anything from a package other than
 *Kernel.
 *********************************************************************************/
#pragma once

#include <memory>
#include <string>

namespace NeXus {
class File;
}

/** A Helper class for easily writing nexus saving/loading tests.
@author Janik Zikovsky
@date 2011-09-07
*/
class NexusTestHelper {
public:
  NexusTestHelper(bool deleteFile = true);
  virtual ~NexusTestHelper();

  void createFile(const std::string &barefilename);
  void reopenFile();

  /// Nexus file handle
  std::unique_ptr<::NeXus::File> file;

  /// Created filename (full path)
  std::string filename;

  /// Do you delete when finished?
  bool deleteFile;
};
