// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllOption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "ui_MuonPeriodInfo.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

/**
 * A widget used in the Muon GUI's to display period information in a readable format
 */
class EXPORT_OPT_MANTIDQT_COMMON MuonPeriodInfo : public QWidget {
  Q_OBJECT

public:
  /// Reads the data of the sample log from the workspace
  static std::string readSampleLog(Mantid::API::MatrixWorkspace_sptr ws, const std::string &logName);
  /// Splits a string separated by a delimeter
  static std::vector<std::string> parseSampleLog(const std::string &log, const std::string &delim);
  /// Unifies the length of all vectors given
  static std::vector<std::vector<std::string>> makeCorrections(std::vector<std::vector<std::string>> &logs);
  explicit MuonPeriodInfo(QWidget *parent = nullptr);
  /// Add a period to the table on the widget
  void addPeriodToTable(const std::string &name, const std::string &type, const std::string &frames,
                        const std::string &totalFrames, const std::string &counts, const std::string &tag);
  /// Gets all sample log data related to periods
  std::vector<std::vector<std::string>> getInfo(Mantid::API::MatrixWorkspace_sptr ws);
  /// Takes the workspace and adds it's period info to the table if any
  void addInfo(const Mantid::API::Workspace_sptr ws);
  /// Set the title of the widget
  void setWidgetTitleRuns(const std::string &title);
  /// Get the title of the widget
  std::string getWidgetTitleRuns() const;
  /// Set the number of sequences that was gathered
  void setNumberOfSequences(const int numberOfSequences);
  /// Get the number of sequences as an int
  int getNumberOfSequences() const;
  /// Get the number of sequences as a string
  std::string getNumberOfSequencesString() const;
  /// Get the number of DAQ periods currently stored in the table
  int getDAQCount() const;
  /// Clear the widget of all information
  void clear();
  /// Checks if the table is empty
  bool isEmpty() const;
  QTableWidget *getTable() const;

private:
  void setUpTable();
  QTableWidgetItem *createNewItem(const std::string &value) const;

  int m_numberOfSequences;
  int m_DAQCount;
  Ui::MuonPeriodInfo m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt
