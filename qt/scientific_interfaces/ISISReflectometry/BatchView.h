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
  BatchView(std::vector<std::string> const &instruments,
            int defaultInstrumentIndex);
  void subscribe(BatchViewSubscriber *notifyee) override;
  void setProgress(int value) override;
  void expandAllGroups() override;
  MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() override;

private slots:
  void onProcessPressed(bool);
  void onExpandAllGroupsPressed(bool);

private:
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
  int indexOfElseFirst(std::string const& instrument) const;
private:
  std::vector<std::string> m_instruments;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEW_H_
