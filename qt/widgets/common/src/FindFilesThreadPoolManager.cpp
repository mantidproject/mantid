#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;

////////////////////////////////////////////////////////////////////
// FindFilesThread
////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 *
 * @param parent :: pointer to the parent QObject.
 */
FindFilesThread::FindFilesThread()
    : QRunnable(), m_error(), m_filenames(), m_valueForProperty(), m_text(),
      m_algorithm(), m_property(), m_isForRunFiles(), m_isOptional() {}

/**
 * Set the values needed for the thread to run.
 *
 * @param text              :: the text containing the file names, typed in by
 *the user
 * @param isForRunFiles     :: whether or not we are finding run files.
 * @param isOptional        :: whether or not the files are optional.
 * @param algorithmProperty :: the algorithm and property to use as an
 *alternative to FileFinder.  Optional.
 */
void FindFilesThread::set(const FindFilesSearchParameters &parameters) {
  m_text = parameters.searchText.trimmed().toStdString();
  m_isForRunFiles = parameters.isForRunFiles;
  m_isOptional = parameters.isOptional;
  m_algorithm = parameters.algorithmName;
  m_property = parameters.algorithmProperty;
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
 *class user must have
 *    specified the algorithm and property via
 *MWRunFiles::setAlgorithmProperty().
 */
void FindFilesThread::run() {
  // Reset result member vars.
  m_error.clear();
  m_filenames.clear();
  m_valueForProperty.clear();

  if (m_text.empty()) {
    if (m_isOptional)
      m_error = "";
    else
      m_error = "No files specified.";

    const auto result = createFindFilesSearchResult();
    emit finished(result);
    return;
  }

  Mantid::API::FileFinderImpl &fileSearcher =
      Mantid::API::FileFinder::Instance();

  try {
    // Use the property of the algorithm to find files, if one has been
    // specified.
    if (m_algorithm.length() != 0 && m_property.length() != 0) {
      getFilesFromAlgorithm();
    }
    // Else if we are loading run files, then use findRuns.
    else if (m_isForRunFiles) {
      m_filenames = fileSearcher.findRuns(m_text);
      m_valueForProperty = "";
      for (auto cit = m_filenames.begin(); cit != m_filenames.end(); ++cit) {
        m_valueForProperty += QString::fromStdString(*cit) + ",";
      }
      m_valueForProperty.chop(1);
    }
    // Else try to run a simple parsing on the string, and find the full paths
    // individually.
    else {
      // Tokenise on ","
      std::vector<std::string> filestext;
      filestext = boost::split(filestext, m_text, boost::is_any_of(","));

      // Iterate over tokens.
      auto it = filestext.begin();
      for (; it != filestext.end(); ++it) {
        boost::algorithm::trim(*it);
        std::string result = fileSearcher.getFullPath(*it);
        Poco::File test(result);
        if ((!result.empty()) && test.exists()) {
          m_filenames.push_back(*it);
          m_valueForProperty += QString::fromStdString(*it) + ",";
        } else {
          throw std::invalid_argument("File \"" + (*it) + "\" not found");
        }
      }
      m_valueForProperty.chop(1);
    }
  } catch (std::exception &exc) {
    m_error = exc.what();
    m_filenames.clear();
  } catch (...) {
    m_error = "An unknown error occurred while trying to locate the file(s). "
              "Please contact the development team";
    m_filenames.clear();
  }

  auto result = createFindFilesSearchResult();
  emit finished(result);
}

/**
 * Create a list of files from the given algorithm property.
 */
void FindFilesThread::getFilesFromAlgorithm() {
  Mantid::API::IAlgorithm_sptr algorithm =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(
          m_algorithm.toStdString());

  if (!algorithm)
    throw std::invalid_argument("Cannot create algorithm " +
                                m_algorithm.toStdString() + ".");

  algorithm->initialize();
  const std::string propName = m_property.toStdString();
  algorithm->setProperty(propName, m_text);

  Property *prop = algorithm->getProperty(propName);
  m_valueForProperty = QString::fromStdString(prop->value());

  FileProperty *fileProp = dynamic_cast<FileProperty *>(prop);
  MultipleFileProperty *multiFileProp =
      dynamic_cast<MultipleFileProperty *>(prop);

  if (fileProp) {
    m_filenames.push_back(fileProp->value());
  } else if (multiFileProp) {
    // This flattens any summed files to a set of single files so that you lose
    // the information about
    // what was summed
    std::vector<std::vector<std::string>> filenames =
        algorithm->getProperty(propName);
    std::vector<std::string> flattenedNames =
        VectorHelper::flattenVector(filenames);

    for (auto filename = flattenedNames.begin();
         filename != flattenedNames.end(); ++filename) {
      m_filenames.push_back(*filename);
    }
  }
}

FindFilesSearchResults FindFilesThread::createFindFilesSearchResult() {
  FindFilesSearchResults results;
  results.error = m_error;
  results.filenames = m_filenames;
  results.valueForProperty = m_valueForProperty.toStdString();
  return results;
}

////////////////////////////////////////////////////////////////////
// FindFilesThreadPoolManager
////////////////////////////////////////////////////////////////////

QThreadPool FindFilesThreadPoolManager::m_pool;

FindFilesThreadPoolManager::FindFilesThreadPoolManager()
    : m_currentWorker(nullptr) {}

void FindFilesThreadPoolManager::createWorker(
    const QObject *parent, const FindFilesSearchParameters &parameters) {
  // if parent is null then don't do anything as there will be no
  // object listening for the search result
  if (!parent)
    return;

  m_currentWorker = new FindFilesThread();

  // Hook up slots for when the thread finishes. By default Qt uses queued
  // connections when connecting signals/slots between threads. Instead here
  // we explicitly choose to use a direct connection so the found result is
  // immediately returned to the GUI thread.
  parent->connect(m_currentWorker,
                  SIGNAL(finished(const FindFilesSearchResults &)), parent,
                  SLOT(inspectThreadResult(const FindFilesSearchResults &)),
                  Qt::DirectConnection);
  parent->connect(m_currentWorker,
                  SIGNAL(finished(const FindFilesSearchResults &)), parent,
                  SIGNAL(fileFindingFinished()),
                  Qt::DirectConnection);

  // Set the search parameters
  m_currentWorker->set(parameters);

  // pass ownership to the thread pool
  // we do not need to worry about deleting m_currentWorker
  m_pool.start(m_currentWorker);
}

void FindFilesThreadPoolManager::cancelWorker(const QObject *parent) {
  if (!isSearchRunning())
    return;

  // Just disconnect any signals from the worker. We leave the worker to
  // continue running in the background because 1) terminating it directly
  // is dangerous (we have no idea what it's currently doing from here) and 2)
  // waiting for it to finish before starting a new thread locks up the GUI
  // event loop.
  m_currentWorker->disconnect(parent);
}

bool FindFilesThreadPoolManager::isSearchRunning() const {
  return m_currentWorker != nullptr;
}

void FindFilesThreadPoolManager::waitForDone() const { m_pool.waitForDone(); }
