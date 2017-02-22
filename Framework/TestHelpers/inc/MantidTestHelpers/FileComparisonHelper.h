#ifndef FILECOMPARISONHELPER_H_
#define FILECOMPARISONHELPER_H_

#include <iosfwd>
#include <string>

namespace FileComparisonHelper {

/// Compares the length and content of two iterators and returns if they are
/// equal
template <typename iter1, typename iter2>
bool areIteratorsEqual(iter1 firstIter1, iter1 lastIter1, iter2 firstIter2, iter2 lastIter2);

/// Checks if two files are equal in content and length at the specified path.
/// Accounts for EOL differences if they exist
bool checkFilesAreEqual(const std::string &referenceFileFullPath,
                        const std::string &outFileFullPath);

/// Checks if two file streams are equal in content and length. Accounts for EOL
/// differences.
bool compareFileStreams(std::ifstream &referenceFileStream,
                        std::ifstream &fileToCheck);
}

#endif // FILECOMPARISONHELPER_H_
