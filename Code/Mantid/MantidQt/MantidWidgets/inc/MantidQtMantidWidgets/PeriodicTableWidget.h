#ifndef MANTID_MANTIDWIDGETS_PERIODICTABLE_H_
#define MANTID_MATIDWIDGETS_PERIODICTABLE_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <QWidget>
#include "ui_PeriodicTableWidget.h"

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS PeriodicTableWidget : public QWidget {
  Q_OBJECT

public:
  PeriodicTableWidget(QWidget *parent = 0);
  std::string
  elementsSelectedToString(std::vector<std::string> elementsSelected);
  void populateGroupVectors();
  void ColourElements();

signals:
  void clicked();
  void ischecked();

private slots:
  std::vector<std::string> getCheckedElements();

private:
  Ui::PeriodicTable ui;
  // vectors for storing Element buttons in associated groups
  std::vector<QPushButton *> OtherNonMetals;
  std::vector<QPushButton *> AlkaliMetals;
  std::vector<QPushButton *> AlkalineEarthMetals;
  std::vector<QPushButton *> TransitionMetals;
  std::vector<QPushButton *> Actinides;
  std::vector<QPushButton *> Lanthanides;
  std::vector<QPushButton *> UnknownProperties;
  std::vector<QPushButton *> PostTransitionMetals;
  std::vector<QPushButton *> Metalloids;
  std::vector<QPushButton *> Halogens;
  std::vector<QPushButton *> NobleGases;

  // Methods to colour by group
  void ColourNonMetals(const std::vector<QPushButton *> &nonMetals);
  void ColourAlkaliMetals(const std::vector<QPushButton *> &alkaliMetals);
  void ColourAlkalineEarthMetals(
      const std::vector<QPushButton *> &alkalineEarthMetals);
  void ColourTransitionMetals(const std::vector<QPushButton *> &transMetals);
  void ColourActinides(const std::vector<QPushButton *> &actinides);
  void ColourLanthanides(const std::vector<QPushButton *> &lanthanides);
  void
  ColourPostTransitionMetals(const std::vector<QPushButton *> &postTransMetals);
  void
  ColourUnknownProperties(const std::vector<QPushButton *> &unknownProperties);
  void ColourMetalloids(const std::vector<QPushButton *> &metalloids);
  void ColourHalogens(const std::vector<QPushButton *> &halogens);
  void ColourNobleGases(const std::vector<QPushButton *> &nobleGases);

  // Methods to colour single element button
  void ColourButton(QPushButton *elementButton, QColor colour);
};

#endif // !MANTID_MANTIDWIDGETS_PERIODICTABLE_H_