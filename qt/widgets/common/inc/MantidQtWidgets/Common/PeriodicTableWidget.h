#ifndef MANTID_MANTIDWIDGETS_PERIODICTABLEWIDGET_H_
#define MANTID_MANTIDWIDGETS_PERIODICTABLEWIDGET_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "ui_PeriodicTableWidget.h"
#include <QVector>
#include <QWidget>

/**
  PeriodicTableWidget: A Widget representing a colour coded Periodic Table of
  Elements, with corresponding buttons as the elements

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class EXPORT_OPT_MANTIDQT_COMMON PeriodicTableWidget : public QWidget {
  Q_OBJECT

public:
  /// Constructor
  PeriodicTableWidget(QWidget *parent = nullptr);
  /// Destructor
  ~PeriodicTableWidget() override{};
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
  QString elementsSelectedToString(QVector<QPushButton *> elementsSelected);

  /// @return Comma-separated string of all element buttons that are checked in
  /// the whole PeriodicTableWidget
  QString getAllCheckedElementsStr();

  /// Disables all buttons associated with a group.
  void disableButtons(QVector<QPushButton *> buttons);

  /// Disables All buttons in periodicTableWidget.
  void disableAllElementButtons();

  /// Enables a button for an element by the element name i.e 'Au' for Gold.
  void enableButtonByName(QString elementStr);

  ///@return the result of the comparison between a string and the text of a
  /// button.
  bool compareButtonNameToStr(QPushButton *buttonToCompare,
                              QString stringToCompare);

  /// Displays or hides the Legend for the colour coding of periodic groups
  void showGroupLegend(bool checked);

  QString getValue();

private:
  /// The Form containing the PeriodicTableWidget
  Ui::PeriodicTable ui;
  /// Methods to colour element buttons by periodic group
  void ColourNonMetals(const QVector<QPushButton *> &nonMetals);
  void ColourAlkaliMetals(const QVector<QPushButton *> &alkaliMetals);
  void
  ColourAlkalineEarthMetals(const QVector<QPushButton *> &alkalineEarthMetals);
  void ColourTransitionMetals(const QVector<QPushButton *> &transMetals);
  void ColourActinides(const QVector<QPushButton *> &actinides);
  void ColourLanthanides(const QVector<QPushButton *> &lanthanides);
  void
  ColourPostTransitionMetals(const QVector<QPushButton *> &postTransMetals);
  void ColourUnknownProperties(const QVector<QPushButton *> &unknownProperties);
  void ColourMetalloids(const QVector<QPushButton *> &metalloids);
  void ColourHalogens(const QVector<QPushButton *> &halogens);
  void ColourNobleGases(const QVector<QPushButton *> &nobleGases);

  /// Methods to colour single element button by setting styleSheet
  void ColourButton(QPushButton *elementButton, QString colour);

  /// Method to populate Group Vectors with element QPushButtons
  void populateGroupVectors();

  /// Method to populate Vector with all Group vectors
  void populateAllButtonsVector();

  /// Colour all of the elements by calls to individual group colouring methods
  void ColourElements();
};

#endif // !MANTID_MANTIDWIDGETS_PERIODICTABLE_H_
