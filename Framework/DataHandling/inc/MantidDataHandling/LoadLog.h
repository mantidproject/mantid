// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADLOG_H_
#define MANTID_DATAHANDLING_LOADLOG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"

namespace Mantid {

namespace DataHandling {
/** @class LoadLog LoadLog.h DataHandling/LoadLog.h

    Load ISIS log file(s). Assumes that a log file originates from a
    PC (not VMS) environment, i.e. the log files to be loaded are assumed
    to have the extension .txt. Its filename is assumed to starts with the raw
   data
    file identifier followed by the character '_', and a log file is assumed to
   have a
    format of two columns, where the first column consists of data-time strings
   of the
    ISO 8601 form and the second column consists of either numbers or strings
   that may
    contain spaces.

    The algoritm requires an input filename. If this filename is the name of a
    raw datafile the algorithm will attempt to read in all the log files
   associated
    with that log file. Otherwise it will assume the filename specified is the
    filename of a specific log file.

    LoadLog is an algorithm and as such inherits from the Algorithm class,
    via DataHandlingCommand, and overrides the init() & exec() methods.
    LoadLog is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.

    Required Properties:
    <UL>
    <LI> Filename - The filename (including its full or relative path) of either
   an ISIS log file
    or an ISIS raw file. If a raw file is specified all log files associated
   with that raw file
    are loaded into the specified workspace. The file extension must either be
   .raw or .s when
    specifying a raw file, and at least 10 characters long. </LI>
    <LI> Workspace - The workspace to which the log data is appended </LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 26/09/2007
*/
class DLLExport LoadLog : public API::Algorithm {
public:
  /// Default constructor
  LoadLog();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadLog"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load ISIS log file(s) into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLog", "LoadNexusLogs"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// SNS text
  bool LoadSNSText();

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of an input file. This may be the filename of a raw
  /// datafile or the name of a specific log file.
  std::string m_filename;

  /// type returned by classify
  enum kind { empty, string, number };

  /// Takes as input a string and try to determine what type it is
  kind classify(const std::string &s) const;

  /// Convert string to lower case
  std::string stringToLower(std::string strToConvert);

  /// Checks if the file is an ASCII file
  bool isAscii(const std::string &filename);

  /// Check if first 19 characters of a string is data-time string according to
  /// yyyy-mm-ddThh:mm:ss
  bool isDateTimeString(const std::string &str) const;

  /// Checks if a log file name was provided (e.g. through setPropertyValue). If
  /// not it creates one based on provided path.
  std::string extractLogName(const std::vector<std::string> &logName);

  /// Check for SNS-style text file
  bool SNSTextFormatColumns(const std::string &str,
                            std::vector<double> &out) const;

  /// Create timeseries property from .log file and adds that to sample object
  void loadThreeColumnLogFile(std::ifstream &logFileStream,
                              std::string logFileName, API::Run &run);

  /// Loads two column log file data into local workspace
  void loadTwoColumnLogFile(std::ifstream &logFileStream,
                            std::string logFileName, API::Run &run);

  /// Returns the number of columns in the log file.
  int countNumberColumns(std::ifstream &logFileStream,
                         const std::string &logFileName);

  /// TimeSeriesProperty<int> containing data periods. Created by LogParser
  boost::shared_ptr<Kernel::Property> m_periods;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADLOG_H_*/
