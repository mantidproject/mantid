#ifndef MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_
#define MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_

#include "ui_Elwin.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/Indirect/IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport Elwin : public IDATab
  {
    Q_OBJECT

  public:
    Elwin(QWidget* parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);
    void setDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws);
    void setDefaultSampleLog(Mantid::API::MatrixWorkspace_const_sptr ws);

  private slots:
    void newInputFiles();
    void newPreviewFileSelected(int index);
    void plotInput();
    void twoRanges(QtProperty* prop, bool);
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);

  private:
    void addSaveAlgorithm(QString workspaceName, QString filename="");

    Ui::Elwin m_uiForm;
    QtTreePropertyBrowser* m_elwTree;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_ELWIN_H_ */
