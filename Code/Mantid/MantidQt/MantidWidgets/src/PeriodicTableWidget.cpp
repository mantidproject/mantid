#include "MantidQtMantidWidgets/PeriodicTableWidget.h"
#include <QVector>

PeriodicTableWidget::PeriodicTableWidget(QWidget *parent) : QWidget(parent) {
  populateGroupVectors();
  ColourElements();
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
}
void PeriodicTableWidget::ColourActinides(const QVector<QPushButton *> &actinides) {
  QColor buttonColour = QColor(255, 85, 127, 255);
  for (auto i = actinides.begin(); i != actinides.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourAlkaliMetals(
    const QVector<QPushButton *> &alkaliMetals) {
  QColor buttonColour = QColor(255, 255, 0, 255);
  for (auto i = alkaliMetals.begin(); i != alkaliMetals.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourAlkalineEarthMetals(
    const QVector<QPushButton *> &alkalineEarthMetals) {
  QColor buttonColour = QColor(170, 170, 127, 255);
  for (auto i = alkalineEarthMetals.begin(); i != alkalineEarthMetals.end();
       i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourHalogens(
    const QVector<QPushButton *> &halogens) {
  QColor buttonColour = QColor(0, 255, 255, 255);
  for (auto i = halogens.begin(); i != halogens.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourLanthanides(
    const QVector<QPushButton *> &lanthanides) {
  QColor buttonColour = QColor(170, 85, 255, 255);
  for (auto i = lanthanides.begin(); i != lanthanides.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourMetalloids(
    const QVector<QPushButton *> &metalloids) {
  QColor buttonColour = QColor(255, 170, 255, 255);
  for (auto i = metalloids.begin(); i != metalloids.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourNobleGases(
    const QVector<QPushButton *> &nobleGases) {
  QColor buttonColour = QColor(255, 170, 0, 255);
  for (auto i = nobleGases.begin(); i != nobleGases.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourNonMetals(
    const QVector<QPushButton *> &nonMetals) {
  QColor buttonColour = QColor(0, 170, 255, 255);
  for (auto i = nonMetals.begin(); i != nonMetals.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourPostTransitionMetals(
    const QVector<QPushButton *> &postTransMetals) {
  QColor buttonColour = QColor(116, 116, 116, 255);
  for (auto i = postTransMetals.begin(); i != postTransMetals.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourTransitionMetals(
    const QVector<QPushButton *> &transMetals) {
  QColor buttonColour = QColor(0, 255, 127, 255);
  for (auto i = transMetals.begin(); i != transMetals.end(); i++) {
    ColourButton(*i, buttonColour);
  }
}
void PeriodicTableWidget::ColourUnknownProperties(
    const QVector<QPushButton *> &UnknownProperties) {
  QColor buttonColour = QColor(255, 0, 0, 255);
}

  QVector<QString> PeriodicTableWidget::getCheckedElements() {
  QVector<QString> temp;
  return temp;
}

void PeriodicTableWidget::ColourButton(QPushButton *element, QColor colour) {
  element->setPaletteBackgroundColor(colour);
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