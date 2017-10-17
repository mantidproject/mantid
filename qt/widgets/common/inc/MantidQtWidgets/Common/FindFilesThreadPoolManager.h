#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_

#include <QThreadPool>
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
  QString searchText;
  /// Whether the search is for experimental run data.
  bool isForRunFiles;
  /// Whether the search is optional (i.e. a failed search means no error).
  bool isOptional;
  /// The name of the algorithm to load files with
  QString algorithmName;
  /// The name of the property on the algorithm to use for searching
  QString algorithmProperty;
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
class FindFilesThread : public QObject, public QRunnable {
  Q_OBJECT

signals:
  void finished(const FindFilesSearchResults &);

public:
  /// Constructor.
  FindFilesThread();
  /// Set the various file-finding values / options.
  void set(const FindFilesSearchParameters &parameters);

protected:
  /// Override parent class run().
  void run() override;

private:
  /// Use the specified algorithm and property to find files instead of using
  /// the FileFinder.
  void getFilesFromAlgorithm();
  /// Helper method to create a search result object
  FindFilesSearchResults createFindFilesSearchResult();

  /// Storage for any error thrown while trying to find files.
  std::string m_error;
  /// Filenames found during execution of the thread.
  std::vector<std::string> m_filenames;
  /// Stores the string value to be used as input for an algorithm property
  QString m_valueForProperty;
  /// File name text typed in by the user.
  std::string m_text;

  QString m_algorithm;
  QString m_property;
  bool m_isForRunFiles;
  bool m_isOptional;
};

/**
 * A small helper class to hold the thread pool
 */
class FindFilesThreadPoolManager {

public:
  FindFilesThreadPoolManager();
  void createWorker(const QObject* parent,
                    const FindFilesSearchParameters& parameters);
  void cancelWorker(const QObject *parent);
  bool isSearchRunning() const;
  void waitForDone() const;

private:
  // make a local thread pool
  FindFilesThread *m_currentWorker;
  static QThreadPool m_pool;
};

}
}

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
