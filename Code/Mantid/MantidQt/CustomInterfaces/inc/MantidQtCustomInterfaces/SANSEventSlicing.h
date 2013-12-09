#ifndef MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_
#define MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_

#include "ui_SANSEventSlicing.h"
#include "MantidQtAPI/UserSubWindow.h"
#include <QString>

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSEventSlicing : public API::UserSubWindow
{
  Q_OBJECT

public:
  /// Default Constructor
  SANSEventSlicing(QWidget *parent=0);
  /// Destructor
  virtual ~SANSEventSlicing();

  static std::string name(){return "SANS ISIS Slicing";}
  static QString categoryInfo() {return "SANS";}

private:
  ///A reference to a logger
  static Mantid::Kernel::Logger & g_log;

  struct ChargeAndTime{
    QString charge; 
    QString time;
  };

  void initLayout();
  
  ChargeAndTime getFullChargeAndTime(const QString & name_ws); 
  QString createSliceEventCode(const QString & name_ws, const QString & start, const QString & stop); 
  ChargeAndTime runSliceEvent(const QString & code2run); 
  void checkPythonOutput(const QString & result); 
  ChargeAndTime values2ChargeAndTime(const QString & input);
  void raiseWarning(QString title, QString message); 

 protected: 
  virtual void showEvent(QShowEvent*); 
private slots:

  /// Apply the slice for the SANS data, and update the view with the last sliced data.
  void doApplySlice();
  void onChangeWorkspace(const QString & newWs);
  
 private:
  Ui::SANSEventSlicing ui; 
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSEVENTSLICING_H_
