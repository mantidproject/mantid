// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QVariant>
#include <map>
#include <set>
#include <string>
#include <vector>

using ParentItems = std::set<int>;
using ChildItems = std::map<int, std::set<int>>;

namespace MantidQt {
namespace MantidWidgets {
class ProgressableView;
namespace DataProcessor {
// Forward decs
class Command;
class DataProcessorMainPresenter;
class DataProcessorView;

/** @class DataProcessorPresenter

DataProcessorPresenter is an interface which defines the functions any data
processor interface presenter needs to support.
*/
class DataProcessorPresenter {
public:
  class DeleteAllRowsCancelledException : public std::exception {
  public:
    const char *what() const noexcept override { return m_msg.c_str(); }

  private:
    std::string m_msg{"User cancelled operation to delete all existing rows"};
  };

  virtual ~DataProcessorPresenter(){};

  enum Flag {
    SaveFlag,
    SaveAsFlag,
    AppendRowFlag,
    AppendGroupFlag,
    DeleteRowFlag,
    DeleteGroupFlag,
    DeleteAllFlag,
    ProcessFlag,
    ProcessAllFlag,
    GroupRowsFlag,
    OpenTableFlag,
    NewTableFlag,
    TableUpdatedFlag,
    ExpandSelectionFlag,
    OptionsDialogFlag,
    ClearSelectedFlag,
    CopySelectedFlag,
    CutSelectedFlag,
    PasteSelectedFlag,
    ImportTableFlag,
    ExportTableFlag,
    PlotRowFlag,
    PlotGroupFlag,
    ExpandAllGroupsFlag,
    CollapseAllGroupsFlag,
    SelectAllFlag,
    PauseFlag
  };

  // Tell the presenter something happened
  virtual void notify(DataProcessorPresenter::Flag flag) = 0;
  virtual void settingsChanged() = 0;
  virtual const std::map<QString, QVariant> &options() const = 0;
  virtual void setOptions(const std::map<QString, QVariant> &options) = 0;
  virtual void transfer(const std::vector<std::map<QString, QString>> &runs) = 0;
  virtual void setInstrumentList(const QStringList &instruments, const QString &defaultInstrument) = 0;
  virtual std::vector<std::unique_ptr<Command>> publishCommands() = 0;
  virtual void accept(DataProcessorMainPresenter *mainPresenter) = 0;
  virtual void acceptViews(DataProcessorView *tableView, ProgressableView *progressView) = 0;
  virtual void setModel(QString const &name) = 0;
  virtual ParentItems selectedParents() const = 0;
  virtual ChildItems selectedChildren() const = 0;
  virtual bool askUserYesNo(const QString &prompt, const QString &title) const = 0;
  virtual void giveUserWarning(const QString &prompt, const QString &title) const = 0;
  virtual bool isProcessing() const = 0;
  virtual void setForcedReProcessing(bool forceReProcessing) = 0;
  virtual void setCell(int row, int column, int parentRow, int parentColumn, const std::string &value) = 0;
  virtual std::string getCell(int row, int column, int parentRow, int parentColumn) = 0;
  virtual int getNumberOfRows() = 0;
  virtual void clearTable() = 0;

  virtual void skipProcessing() = 0;
  virtual void setPromptUser(bool allowPrompt) = 0;
  virtual void confirmReductionPaused() {}
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt