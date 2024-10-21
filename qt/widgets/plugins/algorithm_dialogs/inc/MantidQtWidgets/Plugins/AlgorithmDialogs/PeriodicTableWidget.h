// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "ui_PeriodicTableWidget.h"
#include <QVector>
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

  /// Vector to hold all group vectors for access to All Buttons at once
  QVector<QVector<QPushButton *>> AllElementButtons;

  /// @return Comma-separated string of all the element buttons for one group
  /// that are currently checked
  QString elementsSelectedToString(const QVector<QPushButton *> &elementsSelected);

  /// @return Comma-separated string of all element buttons that are checked in
  /// the whole PeriodicTableWidget
  QString getAllCheckedElementsStr();

  /// Disables all buttons associated with a group.
  void disableButtons(QVector<QPushButton *> buttons);

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
  void ColourNonMetals(const QVector<QPushButton *> &nonMetals);
  void ColourAlkaliMetals(const QVector<QPushButton *> &alkaliMetals);
  void ColourAlkalineEarthMetals(const QVector<QPushButton *> &alkalineEarthMetals);
  void ColourTransitionMetals(const QVector<QPushButton *> &transMetals);
  void ColourActinides(const QVector<QPushButton *> &actinides);
  void ColourLanthanides(const QVector<QPushButton *> &lanthanides);
  void ColourPostTransitionMetals(const QVector<QPushButton *> &postTransMetals);
  void ColourUnknownProperties(const QVector<QPushButton *> &unknownProperties);
  void ColourMetalloids(const QVector<QPushButton *> &metalloids);
  void ColourHalogens(const QVector<QPushButton *> &halogens);
  void ColourNobleGases(const QVector<QPushButton *> &nobleGases);

  /// Methods to colour single element button by setting styleSheet
  void ColourButton(QPushButton *elementButton, const QString &colour);

  /// Method to populate Group Vectors with element QPushButtons
  void populateGroupVectors();

  /// Method to populate Vector with all Group vectors
  void populateAllButtonsVector();

  /// Colour all of the elements by calls to individual group colouring methods
  void ColourElements();
};
