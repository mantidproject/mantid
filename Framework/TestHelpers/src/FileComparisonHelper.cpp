#include "MantidTestHelpers/FileComparisonHelper.h"

#include "MantidAPI/FileFinder.h"
#include "MantidKernel/Logger.h"

#include <fstream>
#include <string>

namespace { // Anonymous namespace
// Bring typedef back into anonymous namespace
using streamCharIter = std::istreambuf_iterator<char>;

// Returns if the difference was due to EOL and fixes the position of the
// stream if it was due to EOL
bool isEolDifference(streamCharIter &streamOne, streamCharIter &streamTwo) {

  // Check which is the Windows file stream (CR in CRLF)
  // and advance it by a character so we are back to LF on both
  if (*streamOne == '\r' && *streamTwo == '\n') {

    ++streamOne;
  } else if (*streamOne == '\n' && *streamTwo == '\r') {
    ++streamTwo;
  } else {
    // Was not a different EOL so indicate false to that
    return false;
  }
  // We sorted EOL issues - return that this was the issue
  return true;
}

// Checks if two char streams are identical whilst accounting for
// EOL differences
bool checkCharactersAreIdentical(streamCharIter streamOne,
                                 streamCharIter streamTwo) {
  const char charOne = *streamOne;
  const char charTwo = *streamTwo;

  // Do comparison first so we do the least ops on good path
  if (charOne == charTwo) {
    return true;
  }

  // Have the handle EOL differences
  return isEolDifference(streamOne, streamTwo);
}

void logDifferenceError(char refChar, char testChar, size_t numNewLines,
                        const std::string &seenChars) {
  const std::string lineNumber = std::to_string(numNewLines);

  // Build our output string
  std::string outError;
  (outError += "At line number: ") += lineNumber;
  (outError += ". Character number: ") += std::to_string(seenChars.size() + 1);
  (outError += " expected : '") += refChar;
  (outError += "' found: '") += testChar;
  ((outError += "\nReference output:\n") += seenChars) += refChar;
  ((outError += "\nTest output:\n") += seenChars) += testChar;

  Mantid::Kernel::Logger g_log("FileComparisonHelper");
  g_log.error(outError);
}

} // End of anonymous namespace

namespace FileComparisonHelper {
/**
* Takes a pair of iterators and checks that all the values
* those iterators point to are identical. If they differ in size
* or value it will return false. If they are identical it will
* return true.
*
* @param refStream:: The starting position of the reference stream to check
* @param testStream:: The starting position of the test stream to check
* @param refStreamEnd:: The final position of the reference stream to check
* @param testStreamEnd:: The final position of the second stream to check
*
* @return True if iterators are identical in value and length else false
*/
bool areIteratorsEqual(streamCharIter refStream, streamCharIter testStream,
                       streamCharIter refStreamEnd,
                       streamCharIter testStreamEnd) {
  // Used if we need to display to the user something did not match
  size_t numNewLines = 0;
  std::string seenChars;

  while (refStream != refStreamEnd && testStream != testStreamEnd) {
    // Check individual values of iterators
    if (!checkCharactersAreIdentical(refStream, testStream)) {
      logDifferenceError(*refStream, *testStream, numNewLines, seenChars);
      return false;
    }

    // Keep track of where the previous EOL is in case we need to log an error
    if (*refStream == '\n') {
      seenChars.clear();
      numNewLines++;
    } else {
      seenChars += *refStream;
    }
    // Move iterators alone to compare next char
    refStream++;
    testStream++;
  }

  bool areStreamsEqual = true;

  // Lastly check both iterators were the same length as they should both
  // point to the end value
  if (refStream != refStreamEnd || testStream != testStreamEnd) {
    Mantid::Kernel::Logger g_log("FileComparisonHelper");
    g_log.error("Length of both files were not identical");
    areStreamsEqual = false;
  } else if (numNewLines == 0 && seenChars.empty()) {
    Mantid::Kernel::Logger g_log("FileComparisonHelper");
    g_log.error("No characters checked in FileComparisonHelper");
    areStreamsEqual = false;
  }

  return areStreamsEqual;
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

  if (refFileStream.fail()) {
    throw std::runtime_error("Could not open reference file at specified path");
  }

  if (outFileStream.fail()) {
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
  // Open iterators for function to run on
  streamCharIter refIter(referenceFileStream), checkIter(fileToCheck);
  // Last iterator in istream is equivalent of uninitialized iterator
  streamCharIter end;

  return areIteratorsEqual(refIter, checkIter, end, end);
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

  if (referenceFilePath.empty()) {
    throw std::invalid_argument("No reference file with the name: " +
                                referenceFileName +
                                " could be found by FileComparisonHelper");
  }
  return areFilesEqual(referenceFilePath, outFileFullPath);
}

} // Namespace FileComparisonHelper
