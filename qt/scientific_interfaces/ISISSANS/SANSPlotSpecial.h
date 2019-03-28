// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB
#define MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB

#include "ui_SANSPlotSpecial.h"
#include <QFrame>

// Forward Declarations
namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}
} // namespace MantidQt
namespace Mantid {
namespace API {
class MatrixWorkspace;
}
} // namespace Mantid
class QwtPlotCurve;
// End of forward declarations

namespace MantidQt {
namespace CustomInterfaces {

class SANSPlotSpecial : public QFrame {
  Q_OBJECT

public:
  /**
   * Small utility class to hold information about the different functions.
   */
  class Transform {
  public:
    enum TransformType {
      GuinierSpheres,
      GuinierRods,
      GuinierSheets,
      Zimm,
      DebyeBueche,
      Holtzer,
      Kratky,
      Porod,
      LogLog,
      General
    };
    explicit Transform(TransformType type);
    ~Transform();
    void init();
    std::vector<double> functionConstants();
    QPair<QStringList, QList<QPair<int, int>>> derivatives();
    QList<QWidget *> xWidgets() { return m_xWidgets; }
    QList<QWidget *> yWidgets() { return m_yWidgets; }
    TransformType type() { return m_type; }
    QStringList interceptDerivatives();
    void tidyGeneral();

  private:
    TransformType m_type;
    QList<QWidget *> m_xWidgets;
    QList<QWidget *> m_yWidgets;
    QString m_gDeriv;
    QString m_iDeriv;
  };

public:
  enum Column {
    FitInformation,
    FitInformationValues,
    GradientLabels,
    GradientDerived,
    GradientUnits,
    InterceptLabels,
    InterceptDerived,
    InterceptUnits
  };
  explicit SANSPlotSpecial(QWidget *parent = nullptr);
  ~SANSPlotSpecial() override;

public slots:
  void rangeChanged(double /*low*/, double /*high*/);
  void plot();
  void updateAxisLabels(const QString & /*value*/);
  void clearTable();
  void calculateDerivatives();
  void tableUpdated(int row, int column);
  void clearInterceptDerived();
  void scalePlot(double /*start*/, double /*end*/);
  void resetSelectors();

private:
  void initLayout();
  boost::shared_ptr<Mantid::API::MatrixWorkspace> runIQTransform();
  void tableDisplay(QStringList properties, QList<QPair<int, int>> positions);
  bool validatePlotOptions();
  void setupTable();
  void createTransforms();
  QwtPlotCurve *
  plotMiniplot(QwtPlotCurve *curve,
               boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace,
               size_t workspaceIndex = 0);

  void deriveGuinierSpheres();
  void deriveGuinierRods();
  void deriveZimm();
  void deriveKratky();
  void derivePorod();

  double getValue(QTableWidgetItem * /*item*/);
  QPair<QStringList, QMap<QString, double>>
  getProperties(const QString &transform);

private:
  Ui::SANSPlotSpecial m_uiForm;
  MantidWidgets::RangeSelector *m_rangeSelector;
  QMap<QString, Transform *> m_transforms;
  QMap<QString, QTableWidgetItem *> m_derivatives;
  QMap<QString, QString> m_units;
  QString m_current;
  QwtPlotCurve *m_dataCurve;
  QwtPlotCurve *m_linearCurve;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_workspaceIQT;
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_workspaceLinear;
  bool m_rearrangingTable;
  QTableWidgetItem *m_emptyCell;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
