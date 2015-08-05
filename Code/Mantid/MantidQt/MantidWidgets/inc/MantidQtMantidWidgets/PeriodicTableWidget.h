#ifndef MANTID_MANTIDWIDGETS_PERIODICTABLEWIDGET_H_
#define MANTID_MANTIDWIDGETS_PERIODICTABLEWIDGET_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <qvector.h>
#include <QWidget>
#include "ui_PeriodicTableWidget.h"

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS PeriodicTableWidget : public QWidget {
  Q_OBJECT

public:
  PeriodicTableWidget(QWidget *parent = 0);
  QString elementsSelectedToString(QVector<QPushButton *> elementsSelected);
  QString getAllCheckedElementsStr();
  void disableButtons(QVector<QPushButton *> buttons);
  void disableAllElementButtons();
  //void enableButtonsByGroup(QString groupNameStr);
  void enableButtonByName(QString elementStr);
  bool compareButtonNameToStr(QPushButton * buttonToCompare,QString stringToCompare);
  // vectors for storing Element buttons in associated groups
  QVector<QPushButton *> OtherNonMetals;
  QVector<QPushButton *> AlkaliMetals;
  QVector<QPushButton *> AlkalineEarthMetals;
  QVector<QPushButton *> TransitionMetals;
  QVector<QPushButton *> Actinides;
  QVector<QPushButton *> Lanthanides;
  QVector<QPushButton *> UnknownProperties;
  QVector<QPushButton *> PostTransitionMetals;
  QVector<QPushButton *> Metalloids;
  QVector<QPushButton *> Halogens;
  QVector<QPushButton *> NobleGases;
  QVector<QVector <QPushButton *> > AllElementButtons;
private slots:
  void showGroupLegend();
private:
  Ui::PeriodicTable ui;
  // Methods to colour by group
  void ColourNonMetals(const QVector<QPushButton *> &nonMetals);
  void ColourAlkaliMetals(const QVector<QPushButton *> &alkaliMetals);
  void ColourAlkalineEarthMetals(
      const QVector<QPushButton *> &alkalineEarthMetals);
  void ColourTransitionMetals(const QVector<QPushButton *> &transMetals);
  void ColourActinides(const QVector<QPushButton *> &actinides);
  void ColourLanthanides(const QVector<QPushButton *> &lanthanides);
  void
  ColourPostTransitionMetals(const QVector<QPushButton *> &postTransMetals);
  void
  ColourUnknownProperties(const QVector<QPushButton *> &unknownProperties);
  void ColourMetalloids(const QVector<QPushButton *> &metalloids);
  void ColourHalogens(const QVector<QPushButton *> &halogens);
  void ColourNobleGases(const QVector<QPushButton *> &nobleGases);

  // Methods to colour single element button
  void ColourButton(QPushButton *elementButton, QString colour);
  //Method to populate vector
  void populateGroupVectors();
  void populateAllButtonsVector();
  //Colour All Elements
  void ColourElements();
};

#endif // !MANTID_MANTIDWIDGETS_PERIODICTABLE_H_