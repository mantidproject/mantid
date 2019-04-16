// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FindFilesWorker.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/File.h>
#include <QApplication>
#include <boost/algorithm/string.hpp>

#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;

/**
 * Constructor.
 *
 * @param parameters :: a struct representing the parameters of the file search
 */
FindFilesWorker::FindFilesWorker(const FindFilesSearchParameters &parameters)
    : QRunnable(), m_parameters(parameters) {
  qRegisterMetaType<MantidQt::API::FindFilesSearchResults>(
      "FindFilesSearchResults");
}

/**
 * Called when the thread is ran via start().  Tries to find the files, and
 * populates the error and filenames member variables with the result of the
 *search.
 *
 * At present, there are two possible use cases:
 *
 * 1. Files are found directly by the FileFinder.  This is the default case.
 * 2. Files are found using the specified algorithm property.  In this case, a
 *    class user must have specified the algorithm and property via
 *    MWRunFiles::setAlgorithmProperty().
 */
void FindFilesWorker::run() {
  // Reset result member vars.
  std::string error;
  std::vector<std::string> filenames;
  QString valueForProperty;

  if (m_parameters.searchText.empty()) {
    if (m_parameters.isOptional)
      error = "";
    else
      error = "No files specified.";

    const auto result = createFindFilesSearchResult(
        error, filenames, valueForProperty.toStdString());
    finishSearching(result);
    return;
  }

  Mantid::API::FileFinderImpl &fileSearcher =
      Mantid::API::FileFinder::Instance();

  try {
    // Use the property of the algorithm to find files, if one has been
    // specified.
    if (m_parameters.algorithmName.length() != 0 &&
        m_parameters.algorithmProperty.length() != 0) {
      auto searchResult = getFilesFromAlgorithm();
      filenames = std::get<0>(searchResult);
      valueForProperty = QString::fromStdString(std::get<1>(searchResult));
    }
    // Else if we are loading run files, then use findRuns.
    else if (m_parameters.isForRunFiles) {
      filenames = fileSearcher.findRuns(m_parameters.searchText);
      valueForProperty = "";
      for (auto &filename : filenames) {
        valueForProperty += QString::fromStdString(filename) + ",";
      }
      valueForProperty.chop(1);
    }
    // Else try to run a simple parsing on the string, and find the full paths
    // individually.
    else {
      // Tokenise on ","
      std::vector<std::string> filestext;
      filestext = boost::split(filestext, m_parameters.searchText,
                               boost::is_any_of(","));

      // Iterate over tokens.
      auto it = filestext.begin();
      for (; it != filestext.end(); ++it) {
        boost::algorithm::trim(*it);
        std::string result = fileSearcher.getFullPath(*it);
        Poco::File test(result);
        if ((!result.empty()) && test.exists()) {
          filenames.push_back(*it);
          valueForProperty += QString::fromStdString(*it) + ",";
        } else {
          throw std::invalid_argument("File \"" + (*it) + "\" not found");
        }
      }
      valueForProperty.chop(1);
    }
  } catch (std::exception &exc) {
    error = exc.what();
    filenames.clear();
  } catch (...) {
    error = "An unknown error occurred while trying to locate the file(s). "
            "Please contact the development team";
    filenames.clear();
  }

  auto result = createFindFilesSearchResult(error, filenames,
                                            valueForProperty.toStdString());
  finishSearching(result);
}

/**
 * Create a list of files from the given algorithm property.
 *
 * @return a tuple with a vector of filenames and a string for the property
 * value
 */
std::pair<std::vector<std::string>, std::string>
FindFilesWorker::getFilesFromAlgorithm() {
  std::vector<std::string> filenames;
  Mantid::API::IAlgorithm_sptr algorithm =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(
          m_parameters.algorithmName);

  if (!algorithm)
    throw std::invalid_argument("Cannot create algorithm " +
                                m_parameters.algorithmName + ".");

  algorithm->initialize();
  const std::string propName = m_parameters.algorithmProperty;
  algorithm->setProperty(propName, m_parameters.searchText);

  Property *prop = algorithm->getProperty(propName);
  std::string valueForProperty = prop->value();

  FileProperty *fileProp = dynamic_cast<FileProperty *>(prop);
  MultipleFileProperty *multiFileProp =
      dynamic_cast<MultipleFileProperty *>(prop);

  if (fileProp) {
    filenames.push_back(fileProp->value());
  } else if (multiFileProp) {
    // This flattens any summed files to a set of single files so that you lose
    // the information about
    // what was summed
    std::vector<std::vector<std::string>> propertyFilenames =
        algorithm->getProperty(propName);
    filenames = VectorHelper::flattenVector(propertyFilenames);
  }

  return std::make_pair(filenames, valueForProperty);
}

/** Finish searching by emitting the results
 *
 * @param result :: the result to return to the main thread.
 */
void FindFilesWorker::finishSearching(const FindFilesSearchResults &result) {
  // Before emitting our search results, we need to check if we have recieved
  // a signal to disconnect as this search is no longer relevent. By calling
  // process events here we ensure that if the result is no longer needed the
  // emitted signal is just ignored.
  QCoreApplication::processEvents();
  emit finished(result);
  QCoreApplication::sendPostedEvents();
}

/** Create a struct containing the results of the search.
 *
 * This will contain a string with an error (empty string if no error occured),
 * a vector of filenames, and a value to display on the property of an algorithm
 *
 * @param error :: A user friendly error message or the empty string
 * @param filenames :: a vector of paths to files found by the search
 * @param valueForProperty :: a message to display on the algorithm's property
 * @return A struct holding the results of this search.
 */
FindFilesSearchResults FindFilesWorker::createFindFilesSearchResult(
    const std::string &error, const std::vector<std::string> &filenames,
    const std::string &valueForProperty) {
  FindFilesSearchResults results;
  results.error = error;
  results.filenames = filenames;
  results.valueForProperty = valueForProperty;
  return results;
}

void FindFilesWorker::disconnectWorker() { this->disconnect(); }
