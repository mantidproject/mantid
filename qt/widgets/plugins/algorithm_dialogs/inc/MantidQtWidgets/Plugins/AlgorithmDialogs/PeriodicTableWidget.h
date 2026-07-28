// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "ui_PeriodicTableWidget.h"
#include <QList>
#include <QWidget>

/**
  PeriodicTableWidget: A Widget representing a colour coded Periodic Table of
  Elements, with corresponding buttons as the elements
*/

class PeriodicTableWidget : public QWidget {
  Q_OBJECT

public:
  /// Constructor
  PeriodicTableWidget(QWidget *parent = nullptr);
  /// Destructor
  ~PeriodicTableWidget() override {};
  /// Vectors to Hold the QPushButtons of Elements in corresponding Groups
  QList<QPushButton *> OtherNonMetals;
  QList<QPushButton *> AlkaliMetals;
  QList<QPushButton *> AlkalineEarthMetals;
  QList<QPushButton *> TransitionMetals;
  QList<QPushButton *> Actinides;
  QList<QPushButton *> Lanthanides;
  QList<QPushButton *> UnknownProperties;
  QList<QPushButton *> PostTransitionMetals;
  QList<QPushButton *> Metalloids;
  QList<QPushButton *> Halogens;
  QList<QPushButton *> NobleGases;

  /// Vector to hold all group vectors for access to All Buttons at once
  QList<QList<QPushButton *>> AllElementButtons;

  /// @return Comma-separated string of all the element buttons for one group
  /// that are currently checked
  QString elementsSelectedToString(const QList<QPushButton *> &elementsSelected);

  /// @return Comma-separated string of all element buttons that are checked in
  /// the whole PeriodicTableWidget
  QString getAllCheckedElementsStr();

  /// Disables all buttons associated with a group.
  void disableButtons(QList<QPushButton *> buttons);

  /// Disables All buttons in periodicTableWidget.
  void disableAllElementButtons();

  /// Enables a button for an element by the element name i.e 'Au' for Gold.
  void enableButtonByName(const QString &elementStr);

  ///@return the result of the comparison between a string and the text of a
  /// button.
  bool compareButtonNameToStr(QPushButton *buttonToCompare, const QString &stringToCompare);

  /// Displays or hides the Legend for the colour coding of periodic groups
  void showGroupLegend(bool checked);

  QString getValue();

private:
  /// The Form containing the PeriodicTableWidget
  Ui::PeriodicTable ui;
  /// Methods to colour element buttons by periodic group
  void ColourNonMetals(const QList<QPushButton *> &nonMetals);
  void ColourAlkaliMetals(const QList<QPushButton *> &alkaliMetals);
  void ColourAlkalineEarthMetals(const QList<QPushButton *> &alkalineEarthMetals);
  void ColourTransitionMetals(const QList<QPushButton *> &transMetals);
  void ColourActinides(const QList<QPushButton *> &actinides);
  void ColourLanthanides(const QList<QPushButton *> &lanthanides);
  void ColourPostTransitionMetals(const QList<QPushButton *> &postTransMetals);
  void ColourUnknownProperties(const QList<QPushButton *> &unknownProperties);
  void ColourMetalloids(const QList<QPushButton *> &metalloids);
  void ColourHalogens(const QList<QPushButton *> &halogens);
  void ColourNobleGases(const QList<QPushButton *> &nobleGases);

  /// Methods to colour single element button by setting styleSheet
  void ColourButton(QPushButton *elementButton, const QString &colour);

  /// Method to populate Group Vectors with element QPushButtons
  void populateGroupVectors();

  /// Method to populate Vector with all Group vectors
  void populateAllButtonsVector();

  /// Colour all of the elements by calls to individual group colouring methods
  void ColourElements();
};
