// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_

#include "MantidQtWidgets/Common/DllOption.h"

#include <QObject>
#include <QRunnable>
#include <QString>
#include <qmetatype.h>
#include <string>
#include <vector>

namespace MantidQt {
namespace API {

/**
 * POD struct to hold details about the parameters of a file search.
 *
 * This is built by the thread pool manager and passed to the worker thread
 * which uses the information to find files and build a FindFilesSearchResult
 */
struct EXPORT_OPT_MANTIDQT_COMMON FindFilesSearchParameters {
  /// The text to use as a hint to search for files.
  std::string searchText;
  /// Whether the search is for experimental run data.
  bool isForRunFiles;
  /// Whether the search is optional (i.e. a failed search means no error).
  bool isOptional;
  /// The name of the algorithm to load files with
  std::string algorithmName;
  /// The name of the property on the algorithm to use for searching
  std::string algorithmProperty;
};

/**
 * POD struct to hold details about the results of a file search.
 *
 * This is build by the FindFilesThread and returned via a signal to a
 * slot on the QObject.
 */
struct EXPORT_OPT_MANTIDQT_COMMON FindFilesSearchResults {
  /// A string repsresenting the error message. Empty if the search succeded.
  std::string error;
  /// A list of filenames that matched the search hint.
  std::vector<std::string> filenames;
  /// The value to set the algorithm property to.
  std::string valueForProperty;
};

/**
 * A class to allow the asynchronous finding of files.
 */
class EXPORT_OPT_MANTIDQT_COMMON FindFilesWorker : public QObject,
                                                   public QRunnable {
  Q_OBJECT

public:
  /// Constructor.
  FindFilesWorker(const FindFilesSearchParameters &parameters);

signals:
  /// Signal emitted after the search is finished, regardless of whether
  /// any file was found.
  void finished(const FindFilesSearchResults & /*_t1*/);

public slots:
  void disconnectWorker();

protected:
  /// Override parent class run().
  virtual void run() override;

private:
  /// Emit search result if required
  void finishSearching(const FindFilesSearchResults &result);

  /// Use the specified algorithm and property to find files instead of using
  /// the FileFinder.
  std::pair<std::vector<std::string>, std::string> getFilesFromAlgorithm();
  /// Helper method to create a search result object
  FindFilesSearchResults
  createFindFilesSearchResult(const std::string &error,
                              const std::vector<std::string> &filenames,
                              const std::string &valueForProperty);
  /// Struct to hold the parameters of the search
  FindFilesSearchParameters m_parameters;
};

} // namespace API
} // namespace MantidQt

// Add to Qt's meta type system. This allows the type
// to be passed between signals & slots
Q_DECLARE_METATYPE(MantidQt::API::FindFilesSearchResults)

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_
