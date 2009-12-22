#ifndef MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_
#define MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_

#include "MantidQtCustomInterfaces/ui_Excitations.h"
#include "poco/Path.h"
#include <QString>
#include <string>
#include <vector>
#include <climits>

namespace MantidQt
{
namespace CustomInterfaces
{

class pythonCalc
{
public:
  // stores the informtion returned by the python script
  struct TestSummary
  {
    QString test;                       //< Name of the test is displayed to users
    QString status;                     //< status is displayed to users
    QString outputWS;                   //< Name of the workspace that contains the bad detectors
    int numBad;                         //< The total number of bad detectors
    QString inputWS;                    //< If these results came from loading another workspace this contains the name of that workspace
    enum resultsStatus {NORESULTS = 15-INT_MAX};  //< a flag value to indicate that there are no results to show, could be that the test has not completed or there was an error
  };
  const QString& python() const; //  void run();
protected:
  QString m_pyScript;
};

class deltaECalc : public pythonCalc
{
public:
  class FileInput
  {
  private:
    QLineEdit &m_lineEdit;
	QComboBox &m_box;
    std::vector<std::string> m_files;
  public:
    FileInput(QLineEdit &num, QComboBox &instr);
    const std::vector<std::string>& getRunFiles();
    //??STEVES? move this function into the file widget
    void readComasAndHyphens(const std::string &in, std::vector<std::string> &out);
  };
  
  explicit deltaECalc(const Ui::Excitations &userSettings, deltaECalc::FileInput &runFiles);
  
  /** removes the path and replaces extensions with .spe
  * @param inputFilename name of the file that the .SPE file is based on
  */
  static QString SPEFileName(const std::string &inputFilename)
  {
    std::string root = Poco::Path(inputFilename).getBaseName();
    return root.empty() ? "" : QString::fromStdString(root)+".spe";
  }
  std::string insertNumber(const std::string &filename, const int number) const;
private:
  /// the form that ws filled in by the user
  const Ui::Excitations &m_settings;
  /// a copy of the section of the template that contains the Python import statements
  QString m_templateH;
  /// a copy of the section of the template that contains the body of the Python code
  QString m_templateB;
  
  QString getScaling() const;
  QString getNormalization() const;
  QString getEGuess() const;
  void readFile(const QString &pythonFile);
  QString createProcessingScript(const std::string &inFiles, const std::string &oName);
  void renameWorkspace(const QString &name);
  
  // holds the prefix that we give to output workspaces that will be deleted in the Python
  static const QString tempWS;
};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_
