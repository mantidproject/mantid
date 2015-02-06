#ifndef MANTIDQTCUSTOMINTERFACESIDA_FURY_H_
#define MANTIDQTCUSTOMINTERFACESIDA_FURY_H_

#include "ui_Fury.h"
#include "IDATab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  class DLLExport Fury : public IDATab
  {
    Q_OBJECT

  public:
    Fury(QWidget * parent = 0);

  private:
    virtual void setup();
    virtual void run();
    virtual bool validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void plotInput(const QString& wsname);
    void rsRangeChangedLazy(double min, double max);
    void updateRS(QtProperty* prop, double val);
    void updatePropertyValues(QtProperty* prop, double val);
    void calculateBinning();

  private:
    Ui::Fury m_uiForm;
    QtTreePropertyBrowser* m_furTree;
    bool m_furyResFileType;

  };
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_FURY_H_ */
