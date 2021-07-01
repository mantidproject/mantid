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
  static std::string readSampleLog(Mantid::API::MatrixWorkspace_const_sptr ws, const std::string &logName);
  static std::vector<std::string> parseSampleLog(const std::string &log, const std::string &delim);
  static std::vector<std::vector<std::string>> makeCorrections(std::vector<std::vector<std::string>> &logs);
  explicit MuonPeriodInfo(QWidget *parent = nullptr);
  void addPeriodToTable(const std::string &name, const std::string &type, const std::string &frames,
                        const std::string &total_frames, const std::string &counts, const std::string &tag);
  std::vector<std::vector<std::string>> getInfo(Mantid::API::MatrixWorkspace_const_sptr ws);
  void addInfo(Mantid::API::MatrixWorkspace_const_sptr ws);
  void setWidgetTitleRuns(const std::string &title);
  void setNumberOfSequences(const int numberOfSequences);
  int getNumberOfSequences() const;
  int getDAQCount() const;
  void clear();
  bool isEmpty() const;

private:
  void setUpTable();
  QTableWidgetItem *createNewItem(const std::string &value) const;

  int m_numberOfSequences;
  int m_DAQCount;
  Ui::MuonPeriodInfo m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt
