#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQT_H_

#include "ui_Iqt.h"
#include "IndirectDataAnalysisTab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport Iqt : public IndirectDataAnalysisTab
  {
    Q_OBJECT

  public:
    Iqt(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void algorithmComplete(bool error);
    void plotInput(const QString& wsname);
    void rsRangeChangedLazy(double min, double max);
    void updateRS(QtProperty* prop, double val);
    void updatePropertyValues(QtProperty* prop, double val);
    void calculateBinning();

  private:
    QString makePyPlotSource(const int &index);

    Ui::Iqt m_uiForm;
    QtTreePropertyBrowser* m_furTree;
    bool m_furyResFileType;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQT_H_ */
