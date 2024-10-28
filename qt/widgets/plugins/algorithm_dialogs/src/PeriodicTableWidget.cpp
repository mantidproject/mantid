// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/PeriodicTableWidget.h"
#include <QVector>

/**
 * Default constructor
 * @param parent :: default parameter
 */
PeriodicTableWidget::PeriodicTableWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  populateGroupVectors();
  populateAllButtonsVector();
  ColourElements();
  /// Hide the legend by default
  ui.Groups->setVisible(false);
}

// slot for showing group legend dependant on state of radioButton
void PeriodicTableWidget::showGroupLegend(bool checked) {
  if (checked) {
    ui.Groups->setVisible(true);
  } else {
    ui.Groups->setVisible(false);
  }
}

void PeriodicTableWidget::ColourElements() {
  ColourActinides(Actinides);
  ColourAlkalineEarthMetals(AlkalineEarthMetals);
  ColourAlkaliMetals(AlkaliMetals);
  ColourHalogens(Halogens);
  ColourLanthanides(Lanthanides);
  ColourMetalloids(Metalloids);
  ColourNobleGases(NobleGases);
  ColourNonMetals(OtherNonMetals);
  ColourPostTransitionMetals(PostTransitionMetals);
  ColourTransitionMetals(TransitionMetals);
  ColourUnknownProperties(UnknownProperties);
}
void PeriodicTableWidget::ColourActinides(const QVector<QPushButton *> &actinides) {
  QString buttonColourStr = "background-color: rgb(255, 85, 127, 255)";
  for (auto actinide : actinides) {
    ColourButton(actinide, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourAlkaliMetals(const QVector<QPushButton *> &alkaliMetals) {
  QString buttonColourStr = "background-color: rgb(255, 255, 0, 255)";
  for (auto alkaliMetal : alkaliMetals) {
    ColourButton(alkaliMetal, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourAlkalineEarthMetals(const QVector<QPushButton *> &alkalineEarthMetals) {
  QString buttonColourStr = "background-color: rgb(170, 170, 127, 255)";
  for (auto alkalineEarthMetal : alkalineEarthMetals) {
    ColourButton(alkalineEarthMetal, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourHalogens(const QVector<QPushButton *> &halogens) {
  QString buttonColourStr = "background-color: rgb(0, 255, 255, 255)";
  for (auto halogen : halogens) {
    ColourButton(halogen, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourLanthanides(const QVector<QPushButton *> &lanthanides) {
  QString buttonColourStr = "background-color: rgb(170, 85, 255, 255)";
  for (auto lanthanide : lanthanides) {
    ColourButton(lanthanide, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourMetalloids(const QVector<QPushButton *> &metalloids) {
  QString buttonColourStr = "background-color: rgb(255, 170, 255, 255)";
  for (auto metalloid : metalloids) {
    ColourButton(metalloid, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourNobleGases(const QVector<QPushButton *> &nobleGases) {
  QString buttonColourStr = "background-color: rgb(255, 170, 0, 255)";
  for (auto nobleGas : nobleGases) {
    ColourButton(nobleGas, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourNonMetals(const QVector<QPushButton *> &nonMetals) {
  QString buttonColourStr = "background-color: rgb(0, 170, 255, 255)";
  for (auto nonMetal : nonMetals) {
    ColourButton(nonMetal, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourPostTransitionMetals(const QVector<QPushButton *> &postTransMetals) {
  QString buttonColourStr = "background-color: rgb(116, 116, 116, 255)";
  for (auto postTransMetal : postTransMetals) {
    ColourButton(postTransMetal, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourTransitionMetals(const QVector<QPushButton *> &transMetals) {
  QString buttonColourStr = "background-color: rgb(0, 255, 127, 255)";
  for (auto transMetal : transMetals) {
    ColourButton(transMetal, buttonColourStr);
    update();
  }
}
void PeriodicTableWidget::ColourUnknownProperties(const QVector<QPushButton *> &UnknownProperties) {
  QString buttonColourStr = "background-color: rgb(255, 0, 0, 255)";
  for (auto unknownProperty : UnknownProperties) {
    ColourButton(unknownProperty, buttonColourStr);
    update();
  }
}

void PeriodicTableWidget::enableButtonByName(const QString &elementStr) {
  for (auto &AllElementButton : AllElementButtons) {
    for (auto btn_i = AllElementButton.begin(); btn_i != AllElementButton.end(); btn_i++) {
      if (compareButtonNameToStr((*btn_i), elementStr)) {
        (*btn_i)->setDisabled(false);
      }
    }
  }
}

bool PeriodicTableWidget::compareButtonNameToStr(QPushButton *buttonToCompare, const QString &stringToCompare) {
  return (strcmp(buttonToCompare->text().toStdString().c_str(), stringToCompare.toStdString().c_str()) == 0);
}

void PeriodicTableWidget::disableAllElementButtons() {
  disableButtons(Actinides);
  disableButtons(AlkaliMetals);
  disableButtons(AlkalineEarthMetals);
  disableButtons(Halogens);
  disableButtons(Lanthanides);
  disableButtons(Metalloids);
  disableButtons(NobleGases);
  disableButtons(OtherNonMetals);
  disableButtons(PostTransitionMetals);
  disableButtons(TransitionMetals);
  disableButtons(UnknownProperties);
}
void PeriodicTableWidget::ColourButton(QPushButton *element, const QString &colourStr) {
  element->setStyleSheet("QPushButton{border:1px solid rgb(0, 0, 0); " + colourStr + ";}" +
                         "QPushButton:checked{ background-color:rgb(175,255,255)}" +
                         "QPushButton:!enabled{background-color: rgb(204,204,204);" + "}");
}

QString PeriodicTableWidget::elementsSelectedToString(const QVector<QPushButton *> &elements) {
  QString selectedElements = "";
  /* Loop through QPushButtons and if they are checked
   * then retrieve the text on the button i.e the
   * element and add it to the string (space delimiter).
   */
  for (auto &element : elements) {
    if (element->isChecked()) {
      selectedElements += element->text() + ",";
    }
  }
  return selectedElements;
}

QString PeriodicTableWidget::getAllCheckedElementsStr() {
  /*checking all groups of buttons to see if they
   * have been selected in the Widget.
   * if they have been selected, the button text is added to
   * the comma-separated list of elements checked.
   */
  QString allCheckedElementsStr = "";
  allCheckedElementsStr += elementsSelectedToString(Actinides);
  allCheckedElementsStr += elementsSelectedToString(AlkaliMetals);
  allCheckedElementsStr += elementsSelectedToString(AlkalineEarthMetals);
  allCheckedElementsStr += elementsSelectedToString(Halogens);
  allCheckedElementsStr += elementsSelectedToString(Lanthanides);
  allCheckedElementsStr += elementsSelectedToString(NobleGases);
  allCheckedElementsStr += elementsSelectedToString(Metalloids);
  allCheckedElementsStr += elementsSelectedToString(OtherNonMetals);
  allCheckedElementsStr += elementsSelectedToString(PostTransitionMetals);
  allCheckedElementsStr += elementsSelectedToString(TransitionMetals);
  allCheckedElementsStr += elementsSelectedToString(UnknownProperties);

  // return a string with all the elements that have been selected
  return allCheckedElementsStr;
}

QString PeriodicTableWidget::getValue() { return getAllCheckedElementsStr(); }

void PeriodicTableWidget::disableButtons(QVector<QPushButton *> buttonsToDisable) {
  for (auto &button : buttonsToDisable) {
    button->setDisabled(true);
  }
}

void PeriodicTableWidget::populateGroupVectors() {
  /*Populate Group Vectors with corresponding Element Buttons*/

  // Populate Other Non-Metals
  OtherNonMetals.push_back(ui.C);  // Carbon
  OtherNonMetals.push_back(ui.N);  // Nitrogen
  OtherNonMetals.push_back(ui.H);  // Hydrogen
  OtherNonMetals.push_back(ui.O);  // Oxygen
  OtherNonMetals.push_back(ui.Se); // Selenium
  OtherNonMetals.push_back(ui.S);  // Sulfur
  OtherNonMetals.push_back(ui.P);  // Phospherus
  // Populate Alkali Metals
  AlkaliMetals.push_back(ui.Cs); // Cesium
  AlkaliMetals.push_back(ui.Fr); // Francium
  AlkaliMetals.push_back(ui.Li); // Lithium
  AlkaliMetals.push_back(ui.K);  // Potassium
  AlkaliMetals.push_back(ui.Rb); // Rubidium
  AlkaliMetals.push_back(ui.Na); // Sodium
  // Populate Alkaline Earth Metals
  AlkalineEarthMetals.push_back(ui.Ba); // Barium
  AlkalineEarthMetals.push_back(ui.Be); // Beryllium
  AlkalineEarthMetals.push_back(ui.Ca); // Calcium
  AlkalineEarthMetals.push_back(ui.Mg); // Magnesium
  AlkalineEarthMetals.push_back(ui.Ra); // Radium
  AlkalineEarthMetals.push_back(ui.Sr); // Strontium
  // Populate Transition Metals
  TransitionMetals.push_back(ui.Ag); // Silver
  TransitionMetals.push_back(ui.Au); // Gold
  TransitionMetals.push_back(ui.Bh); // Bohrium
  TransitionMetals.push_back(ui.Cd); // Cadmium
  TransitionMetals.push_back(ui.Cn); // Copernicium
  TransitionMetals.push_back(ui.Co); // Cobalt
  TransitionMetals.push_back(ui.Cr); // Chromium
  TransitionMetals.push_back(ui.Cu); // Copper
  TransitionMetals.push_back(ui.Db); // Dubnium
  TransitionMetals.push_back(ui.Fe); // Iron
  TransitionMetals.push_back(ui.Hf); // Hafnium
  TransitionMetals.push_back(ui.Hg); // Mercury
  TransitionMetals.push_back(ui.Hs); // Hassium
  TransitionMetals.push_back(ui.Ir); // Iridium
  TransitionMetals.push_back(ui.Mn); // Manganese
  TransitionMetals.push_back(ui.Mo); // Molybdenum
  TransitionMetals.push_back(ui.Nb); // Niobium
  TransitionMetals.push_back(ui.Ni); // Nickel
  TransitionMetals.push_back(ui.Os); // Osmium
  TransitionMetals.push_back(ui.Pd); // Palladium
  TransitionMetals.push_back(ui.Pt); // Platinum
  TransitionMetals.push_back(ui.Re); // Rhenium
  TransitionMetals.push_back(ui.Rf); // Rutherfordium
  TransitionMetals.push_back(ui.Rh); // Rhodium
  TransitionMetals.push_back(ui.Ru); // Ruthenium
  TransitionMetals.push_back(ui.Sc); // Scandium
  TransitionMetals.push_back(ui.Sg); // Seaborgium
  TransitionMetals.push_back(ui.Ta); // Tantalum
  TransitionMetals.push_back(ui.Tc); // Technetium
  TransitionMetals.push_back(ui.Ti); // Titanium
  TransitionMetals.push_back(ui.V);  // Vanadium
  TransitionMetals.push_back(ui.W);  // Tungsten
  TransitionMetals.push_back(ui.Y);  // Yttrium
  TransitionMetals.push_back(ui.Zn); // Zinc
  TransitionMetals.push_back(ui.Zr); // Zirconium
  // Populate Actinides
  Actinides.push_back(ui.Ac); // Actinium
  Actinides.push_back(ui.Am); // Americium
  Actinides.push_back(ui.Bk); // Berkelium
  Actinides.push_back(ui.Cf); // Californium
  Actinides.push_back(ui.Cm); // Curium
  Actinides.push_back(ui.Es); // Einsteinium
  Actinides.push_back(ui.Fm); // Fermium
  Actinides.push_back(ui.Lr); // Lawrencium
  Actinides.push_back(ui.Md); // Mendelevium
  Actinides.push_back(ui.No); // Nobelium
  Actinides.push_back(ui.Np); // Neptunium
  Actinides.push_back(ui.Pa); // Protractinium
  Actinides.push_back(ui.Pu); // Plutonium
  Actinides.push_back(ui.Th); // Thorium
  Actinides.push_back(ui.U);  // Uranium
  // Populate Lanthanides
  Lanthanides.push_back(ui.Ce); // Cerium
  Lanthanides.push_back(ui.Dy); // Dysprosium
  Lanthanides.push_back(ui.Er); // Erbium
  Lanthanides.push_back(ui.Eu); // Europium
  Lanthanides.push_back(ui.Gd); // Gadolinium
  Lanthanides.push_back(ui.Ho); // Holmium
  Lanthanides.push_back(ui.La); // Lanthanum
  Lanthanides.push_back(ui.Lu); // Lutetium
  Lanthanides.push_back(ui.Nd); // Neodymium
  Lanthanides.push_back(ui.Pm); // Promethium
  Lanthanides.push_back(ui.Pr); // Praseodymium
  Lanthanides.push_back(ui.Sm); // Samarium
  Lanthanides.push_back(ui.Tb); // Terbium
  Lanthanides.push_back(ui.Tm); // Thulium
  Lanthanides.push_back(ui.Yb); // Yttrbium
  // Populate Unknown Properties
  UnknownProperties.push_back(ui.Ds);  // Damstadium
  UnknownProperties.push_back(ui.Fl);  // Flerovium
  UnknownProperties.push_back(ui.Lv);  // Livermorium
  UnknownProperties.push_back(ui.Mt);  // Meitnerium
  UnknownProperties.push_back(ui.Rg);  // Roentgenium
  UnknownProperties.push_back(ui.Uuo); // Ununoctium
  UnknownProperties.push_back(ui.Uup); // Ununpentium
  UnknownProperties.push_back(ui.Uus); // Ununseptium
  UnknownProperties.push_back(ui.Uut); // Ununtrium
  // Populate Post-Transition Metals
  PostTransitionMetals.push_back(ui.Al); // Aluminium
  PostTransitionMetals.push_back(ui.Bi); // Bismuth
  PostTransitionMetals.push_back(ui.Ga); // Gallium
  PostTransitionMetals.push_back(ui.In); // Indium
  PostTransitionMetals.push_back(ui.Pb); // Lead
  PostTransitionMetals.push_back(ui.Po); // Polonium
  PostTransitionMetals.push_back(ui.Sn); // Tin
  PostTransitionMetals.push_back(ui.Tl); // Thalium
  // Populate Metalloids
  Metalloids.push_back(ui.As); // Arsenic
  Metalloids.push_back(ui.B);  // Boron
  Metalloids.push_back(ui.Ge); // Germanium
  Metalloids.push_back(ui.Sb); // Antimony
  Metalloids.push_back(ui.Si); // Silicon
  Metalloids.push_back(ui.Te); // Tellurium
  // Populate Halogens
  Halogens.push_back(ui.At); // Astatine
  Halogens.push_back(ui.Cl); // Chlorine
  Halogens.push_back(ui.Br); // Bromine
  Halogens.push_back(ui.F);  // Flourine
  Halogens.push_back(ui.I);  // Iodine
  // Populate Noble Gases
  NobleGases.push_back(ui.Ar); // Argon
  NobleGases.push_back(ui.He); // Helium
  NobleGases.push_back(ui.Kr); // Krypton
  NobleGases.push_back(ui.Ne); // Neon
  NobleGases.push_back(ui.Rn); // Radon
  NobleGases.push_back(ui.Xe); // Xenon
}

void PeriodicTableWidget::populateAllButtonsVector() {
  AllElementButtons.push_back(Actinides);
  AllElementButtons.push_back(OtherNonMetals);
  AllElementButtons.push_back(AlkaliMetals);
  AllElementButtons.push_back(AlkalineEarthMetals);
  AllElementButtons.push_back(TransitionMetals);
  AllElementButtons.push_back(Lanthanides);
  AllElementButtons.push_back(UnknownProperties);
  AllElementButtons.push_back(PostTransitionMetals);
  AllElementButtons.push_back(Metalloids);
  AllElementButtons.push_back(Halogens);
  AllElementButtons.push_back(NobleGases);
}
