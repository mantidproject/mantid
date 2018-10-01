#ifndef MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_

#include "IndirectDataAnalysisTab.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_Elwin.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport Elwin : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  Elwin(QWidget *parent = nullptr);

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws,
                            const QPair<double, double> &range);
  void setDefaultSampleLog(Mantid::API::MatrixWorkspace_const_sptr ws);

  void setRunEnabled(bool enabled);
  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);

  void setRunIsRunning(bool running);
  void setPlotResultIsPlotting(bool plotting);

private slots:
  void newInputFiles();
  void newPreviewFileSelected(int index);
  void plotInput();
  void twoRanges(QtProperty *prop, bool);
  void minChanged(double val);
  void maxChanged(double val);
  void updateRS(QtProperty *prop, double val);
  void unGroupInput(bool error);
  void runClicked();
  void saveClicked();
  void plotClicked();

private:
  Ui::Elwin m_uiForm;
  QtTreePropertyBrowser *m_elwTree;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_ */
