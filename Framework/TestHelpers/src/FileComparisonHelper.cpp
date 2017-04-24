#include "MantidTestHelpers/FileComparisonHelper.h"

#include "MantidAPI/FileFinder.h"

#include <fstream>
#include <string>

namespace FileComparisonHelper {
/**
* Takes a pair of iterators and checks that all the values
* those iterators point to are identical. If they differ in size
* or value it will return false. If they are identical it will
* return true.
*
* @param firstIter1:: The starting position of the first iterator to check
* @param firstIter2:: The starting position of the second iterator to check
* @param lastIter1:: The final position of the first iterator to check
* @param lastIter2:: The final position of the second iterator to check
*
* @return True if iterators are identical in value and length else false
*/
template <typename iter1, typename iter2>
bool areFileStreamsEqual(iter1 firstIter1, iter2 firstIter2, iter1 lastIter1,
                         iter2 lastIter2) {

  while (firstIter1 != lastIter1 && firstIter2 != lastIter2) {
    // Check individual values of iterators
    if (*firstIter1 != *firstIter2) {
      return false;
    }

    firstIter1++;
    firstIter2++;
  }
  // Check that both iterators were the same length
  return (firstIter1 == lastIter1 && firstIter2 == lastIter2);
}

/**
* Checks the two files at the specified paths are equal in both
* length and content. This does not take into account line endings
* i.e. CRLF and LF between Windows and Unix. If the files cannot
* be opened it will throw. a runtime error.
*
* @param referenceFileFullPath:: The full path to the reference file
* @param outFileFullPath:: The full path to the output file to check
*
* @return True if files are equal in content and length, else false.
*/
bool areFilesEqual(const std::string &referenceFileFullPath,
                   const std::string &outFileFullPath) {
  std::ifstream refFileStream(referenceFileFullPath, std::ifstream::binary);
  std::ifstream outFileStream(outFileFullPath, std::ifstream::binary);

  if (!refFileStream.is_open()) {
    throw std::runtime_error("Could not open reference file at specified path");
  }

  if (!outFileStream.is_open()) {
    throw std::runtime_error("Could not open output file at specified path");
  }

  return areFileStreamsEqual(refFileStream, outFileStream);
}

/**
* Compares two file streams for equality and returns
* true if these are equal, false if they differ in any way.
* This does not account for line endings i.e. CRLF and LF
* on Windows and Unix files would return false
*
* @param referenceFileStream :: The ifstream to the reference file
* @param fileToCheck :: The ifstream to the file to check
*
* @return True if files are identical else false
*/
bool areFileStreamsEqual(std::ifstream &referenceFileStream,
                         std::ifstream &fileToCheck) {
  // Open iterators for templated function to run on
  std::istreambuf_iterator<char> refIter(referenceFileStream);
  std::istreambuf_iterator<char> checkIter(fileToCheck);
  // Last iterator in istream is equivalent of uninitialized iterator
  std::istreambuf_iterator<char> end;

  return areFileStreamsEqual(refIter, checkIter, end, end);
}

/**
  * Attempts to find a reference file with the given name using
  * the FileFinder. If it cannot be found it will throw a
  * std::invalid_argument. If the file is found it will compare
  * the two specified files are equal in length and content
  * whilst ignoring EOL differences
  *
  * @param referenceFileName :: The filename of the reference file
  * @param outFileFullPath :: The path to the file written by the test to
  *compare
  *
  * @throws :: If the reference file could not be found throws
  *std::invalid_argument
  *
  * @return :: True if files are equal length and content (ignoring EOL) else
  *false.
  */
bool isEqualToReferenceFile(const std::string &referenceFileName,
                            const std::string &outFileFullPath) {
  const std::string referenceFilePath =
      Mantid::API::FileFinder::Instance().getFullPath(referenceFileName);

  if (referenceFilePath == "") {
    throw std::invalid_argument("No file with the name: " + referenceFileName +
                                " could be found by Mantid");
  }
  return areFilesEqual(referenceFilePath, outFileFullPath);
}

} // Namespace FileComparisonHelper
