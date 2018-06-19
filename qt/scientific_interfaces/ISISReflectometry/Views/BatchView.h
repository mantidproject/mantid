#ifndef MANTID_CUSTOMINTERFACES_BATCHVIEW_H_
#define MANTID_CUSTOMINTERFACES_BATCHVIEW_H_
#include "DllConfig.h"
#include <memory>
#include <vector>
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "IBatchView.h"
#include "ui_BatchView.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchView : public QWidget,
                                                 public IBatchView {
  Q_OBJECT
public:
  explicit BatchView(std::vector<std::string> const &instruments,
                     int defaultInstrumentIndex);
  void subscribe(BatchViewSubscriber *notifyee) override;
  void setProgress(int value) override;
  void resetFilterBox() override;
  MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() override;

  void invalidSelectionForCopy() override;
  void invalidSelectionForPaste() override;
  void invalidSelectionForCut() override;
  void mustSelectRow() override;
  void mustSelectGroup() override;
  void mustNotSelectGroup() override;
  void mustSelectGroupOrRow() override;

private slots:
  void onProcessPressed(bool);
  void onPausePressed(bool);
  void onExpandAllGroupsPressed(bool);
  void onCollapseAllGroupsPressed(bool);
  void onInsertRowPressed(bool);
  void onInsertGroupPressed(bool);
  void onDeleteRowPressed(bool);
  void onDeleteGroupPressed(bool);
  void onCopyPressed(bool);
  void onCutPressed(bool);
  void onPastePressed(bool);
  void onFilterChanged(QString const &);

private:
  void addToolbarActions();
  QAction *addToolbarItem(std::string const &iconPath,
                          std::string const &description);
  void showAlgorithmPropertyHintsInOptionsColumn();
  Ui::BatchView m_ui;
  std::unique_ptr<MantidQt::MantidWidgets::Batch::JobTreeView> m_jobs;
  std::vector<std::string> m_instruments;
  BatchViewSubscriber *m_notifyee;
};

class BatchViewFactory {
public:
  BatchViewFactory(std::vector<std::string> const &instruments);
  BatchView *operator()(int defaultInstrumentIndex) const;
  BatchView *operator()() const;
  int defaultInstrumentFromConfig() const;
  int indexOfElseFirst(std::string const &instrument) const;

private:
  std::vector<std::string> m_instruments;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEW_H_
