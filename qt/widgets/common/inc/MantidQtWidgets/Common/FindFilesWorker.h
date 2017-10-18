#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_

#include <QObject>
#include <QRunnable>
#include <QString>
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
struct FindFilesSearchParameters {
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
struct FindFilesSearchResults {
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
class FindFilesWorker : public QObject, public QRunnable {
  Q_OBJECT

signals:
  /// Signal emitted after the search is finished, regardless of whether
  /// any file was found.
  void finished(const FindFilesSearchResults &);

public:
  /// Constructor.
  FindFilesWorker(const FindFilesSearchParameters &parameters);

protected:
  /// Override parent class run().
  virtual void run() override;

private:
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

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESWORKER_H_
