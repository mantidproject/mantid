// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <iosfwd>
#include <iterator>
#include <string>

namespace FileComparisonHelper {
/**
  FileComparisonHelper provides several helper functions to compare
  files or file-streams within unit tests. It accounts for EOL
  differences between Unix and Windows and will emit an error in
  the Mantid logging system as to how the files differed

  @author David Fairbrother
  @date 2017


*/

/// Typedef buffered stream iterator (char) to a shorter name
using streamCharIter = std::istreambuf_iterator<char>;

/// Compares the length and content of std::streams iterators
///	and returns if they are equal
bool areIteratorsEqual(streamCharIter refStream, streamCharIter testStream,
                       streamCharIter refStreamEnd = streamCharIter(), streamCharIter testStreamEnd = streamCharIter());

/// Checks if two files are equal in content and length at the specified path.
/// Accounts for EOL differences if they exist
bool areFilesEqual(const std::string &referenceFileFullPath, const std::string &outFileFullPath);

/// Checks if two file streams are equal in content and length. Accounts for EOL
/// differences.
bool areFileStreamsEqual(std::ifstream &referenceFileStream, std::ifstream &fileToCheck);

/// Attempts to find a reference file with the given name using Mantid
/// then compares content and length of files (ignoring EOL differences).
bool isEqualToReferenceFile(const std::string &referenceFileName, const std::string &outFileFullPath);
} // Namespace FileComparisonHelper
