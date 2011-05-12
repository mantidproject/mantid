#ifndef MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB
#define MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB

#include <QFrame>
#include "ui_SANSPlotSpecial.h"

// Forward Declarations
namespace MantidQt
{
namespace MantidWidgets
{
  class RangeSelector;
}
}
namespace Mantid
{
namespace API
{
  class MatrixWorkspace;
}
}
class QwtPlotCurve;
// End of forward declarations

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSPlotSpecial : public QFrame
{
  Q_OBJECT

public:
  /**
  * Small utility class to hold information about the different functions.
  */
  class Transform : QWidget
  {
  public:
    enum TransformType { GuinierSpheres, GuinierRods, GuinierSheets, Zimm, DebyeBueche,
      Holtzer, Kratky, Porod, LogLog, General };
    Transform(TransformType type, QWidget* parent=0);
    ~Transform();
    void init();
    std::vector<double> functionConstants();
    QList<QWidget*> xWidgets() { return m_xWidgets; };
    QList<QWidget*> yWidgets() { return m_yWidgets; };
    void tidyGeneral();
    
  private:
    TransformType m_type;    
    QList<QWidget*> m_xWidgets;
    QList<QWidget*> m_yWidgets;
    QWidget* m_parent;
  };

public:
  SANSPlotSpecial(QWidget *parent = 0);
  ~SANSPlotSpecial();

public slots:
  void rangeChanged(double, double);
  void plot();
  void help();
  void updateAxisLabels(const QString&);

private:
  void initLayout();
  boost::shared_ptr<Mantid::API::MatrixWorkspace> runIQTransform();
  bool validatePlotOptions();
  void createTransforms();
  QwtPlotCurve* plotMiniplot(QwtPlotCurve* curve, 
    boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace);
  
private:
  Ui::SANSPlotSpecial m_uiForm;
  MantidWidgets::RangeSelector* m_rangeSelector;
  QMap<QString, Transform*> m_transforms;
  QString m_current;
  QwtPlotCurve* m_dataCurve;
  QwtPlotCurve* m_linearCurve;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_workspaceIQT;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_workspaceLinear;
};

}
}

#endif