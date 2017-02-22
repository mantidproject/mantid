#ifndef FILECOMPARISONHELPER_H_
#define FILECOMPARISONHELPER_H_

#include <iostream>

namespace FileComparisonHelper {
/**
* Takes a pair of iterators and checks that all the values
* those iterators point to are identical. If they differ in size
* or value it will return false. If they are identical it will
* return true.
*
* @param firstIter1:: The starting position of the first iterator to check
* @param lastIter1:: The final position of the first iterator to check
* @param firstIter2:: The starting position of the second iterator to check
* @param lastIter2:: The final position of the second iterator to check
*
* @return True if iterators are identical in value and length else false
*/
template <typename iter1, typename iter2>
bool areIteratorsEqual(iter1 firstIter1, iter1 lastIter1, iter2 firstIter2,
                       iter2 lastIter2) {

  while (firstIter1 != lastIter1 && firstIter2 != lastIter2) {
    // Check individual values of iterators
    if (*firstIter1 != *firstIter2) {
      // Check if we need to account for EOL differences
      if (*firstIter1 == '\r') {
        std::advance(firstIter1, 2); // Consume CRLF
        firstIter2++;
      } else if (*firstIter2 == '\r') {
        std::advance(firstIter2, 2); // Consume CRLF
        firstIter1++;
      } else {
        return false;
      }
    }

    firstIter1++;
    firstIter2++;
  }
  // Check that both iterators were the same length
  return (firstIter1 == lastIter1 && firstIter2 == lastIter2);
}

/**
  * Compares two file streams for equality and returns
  * true if these are equal, false if they differ in any way.
  * This does account for line endings i.e. CRLF and LF
  * will not be counted as a mismatch
  *
  * @param referenceFileStream :: The ifstream to the reference file
  * @param fileToCheck :: The ifstream to the file to check
  *
  * @return True if files are identical else false
  */
bool compareFileStreams(std::ifstream &referenceFileStream,
                        std::ifstream &fileToCheck) {
  // Open iterators for templated function to run on
  std::istreambuf_iterator<char> refIter(referenceFileStream);
  std::istreambuf_iterator<char> checkIter(fileToCheck);
  // Last iterator in istream is equivalent of uninitialized iterator
  std::istreambuf_iterator<char> end;

  return areIteratorsEqual(refIter, end, checkIter, end);
}

/**
  * Checks the two files at the specified paths are equal in both
  * length and content. This does take into account line endings
  * i.e. CRLF and LF between Windows and Unix is OK.
  * If the files cannot be opened it will throw. a runtime error.
  *
  * @param referenceFileFullPath:: The full path to the reference file
  * @param outFileFullPath:: The full path to the output file to check
  *
  * @return True if files are equal in content and length, else false.
  */
bool checkFilesAreEqual(const std::string &referenceFileFullPath,
                        const std::string &outFileFullPath) {
  std::ifstream refFileStream(referenceFileFullPath, std::ifstream::binary);
  std::ifstream outFileStream(outFileFullPath, std::ifstream::binary);

  if (!refFileStream.is_open()) {
    throw std::runtime_error("Could not open reference file at specified path");
  }

  if (!outFileStream.is_open()) {
    throw std::runtime_error("Could not open output file at specified path");
  }

  return compareFileStreams(refFileStream, outFileStream);
}
}

#endif // FILECOMPARISONHELPER_H_