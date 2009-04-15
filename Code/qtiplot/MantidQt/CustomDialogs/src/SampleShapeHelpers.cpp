//------------------------------------------
// Includes
//-----------------------------------------
#include "MantidQtCustomDialogs/SampleShapeHelpers.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QRadioButton>

using namespace MantidQt::CustomDialogs;

//--------------------------------------------------------//
//         PointGroupBox helper class
//--------------------------------------------------------//

PointGroupBox::PointGroupBox(QWidget* parent) : QGroupBox(parent), m_icoord(0)
{
  QGridLayout *grid = new QGridLayout;
  
  // The line edit fields
  m_midx = new QLineEdit;
  m_midy = new QLineEdit;
  m_midz = new QLineEdit;

  m_xunits = ShapeDetails::createLengthUnitsCombo();
  m_yunits = ShapeDetails::createLengthUnitsCombo();
  m_zunits = ShapeDetails::createLengthUnitsCombo();

  // Radio selections
  m_cartesian = new QRadioButton("Cartesian");
  m_cartesian->setChecked(true);
  m_spherical = new QRadioButton("Spherical");

  connect(m_cartesian, SIGNAL(clicked(bool)), this, SLOT(changeToCartesian()));
  connect(m_spherical, SIGNAL(clicked(bool)), this, SLOT(changeToSpherical()));

  int row(0);
  grid->addWidget(m_cartesian, row, 0, 1, 2);
  grid->addWidget(m_spherical, row, 2, 1, 2);
  ++row;
  //labels
  m_xlabel = new QLabel("x: ");
  m_ylabel = new QLabel("y: ");
  m_zlabel = new QLabel("z: ");
  
  // x
  grid->addWidget(m_xlabel, row, 0, Qt::AlignRight);
  grid->addWidget(m_midx, row, 1);
  grid->addWidget(m_xunits, row, 2);
  ++row;
  // y
  grid->addWidget(m_ylabel, row, 0);//, Qt::AlignRight);
  grid->addWidget(m_midy, row, 1);
  grid->addWidget(m_yunits, row, 2);
  ++row;
  // z
  grid->addWidget(m_zlabel, row, 0, Qt::AlignRight);
  grid->addWidget(m_midz, row, 1);
  grid->addWidget(m_zunits, row, 2);
  
  setLayout(grid);
}

// Switch to cartesian coordinates
void PointGroupBox::changeToCartesian()
{
  if( m_icoord == 0 ) return;

  m_xlabel->setText("x: ");
  m_ylabel->setText("y: ");
  m_zlabel->setText("z: ");

  m_yunits->setEnabled(true);
  m_zunits->setEnabled(true);

  m_icoord = 0;
}

// Switch to spherical coordinates
void PointGroupBox::changeToSpherical()
{
  if( m_icoord == 1 ) return;

  m_xlabel->setText("r: ");
  m_ylabel->setText("theta: ");
  m_zlabel->setText("phi: ");

  //Units are in degrees for theta and phi
  m_yunits->setEnabled(false);
  m_zunits->setEnabled(false);

  m_icoord = 1;
}

/**
 * Write the element tag for a 3D point.
 * elem_name The name of the element
 */
QString PointGroupBox::write3DElement(const QString & elem_name) const
{
  QString valx("0.0"), valy("0.0"), valz("0.0");
  if( !m_midx->text().isEmpty() )
  {
    valx = ShapeDetails::convertToMetres(m_midx->text(), ShapeDetails::Unit(m_xunits->currentIndex()));
  }
  if( !m_midy->text().isEmpty() )
  {
    if( m_icoord == 0 )
    {
      valy = ShapeDetails::convertToMetres(m_midy->text(), ShapeDetails::Unit(m_yunits->currentIndex()));
    }
    else 
    {
      valy = m_midy->text();
    }      
  }
  if( !m_midz->text().isEmpty() )
  {
    if( m_icoord == 0 )
    {
      valz = ShapeDetails::convertToMetres(m_midz->text(), ShapeDetails::Unit(m_zunits->currentIndex()));
    }
    else 
    {
      valz = m_midz->text();
    }      
  }
  QString tag;
  if( m_icoord == 0 )
  {
    tag = "<" + elem_name + " x=\"" + valx + "\" y=\"" + valy + "\" z= \"" + valz + "\" />\n";
  }
  else
  {
    tag = "<" + elem_name + " r=\"" + valx + "\" t=\"" + valy + "\" p= \"" + valz + "\" />\n";
  }
  return tag;
}

//----------------------------------------------------//
//         Operation class member function
//---------------------------------------------------//
/**
 * Take the arguments given and form a string using the
 * current algebra
 * @param Left-hand side of binary operation
 * @param Right-hand side of binary operation
 * @returns A string representing the result of the operation on the arguments
 */
QString Operation::toString(QString left, QString right) const
{
  QString result;
  switch( binaryop )
  {
  // union
  case 1:
   result = left + ":" + right;
    break;
  // difference (intersection of the complement)
  case 2:
    result = left + " (# " + right + ")";
    break;
  // intersection
  case 0: 
  default:
    result = left + " " + right;
    break;
  }
  return "(" + result + ")";
}

//----------------------------------------------------//
//         Base ShapeDetails
//---------------------------------------------------//
/**
 * Create a QComboBox filled with length units (static)
 */
QComboBox* ShapeDetails::createLengthUnitsCombo()
{
  QComboBox *units = new QComboBox;
  QStringList unit_labels("mm");
  unit_labels << "cm" << "m";
  units->addItems(unit_labels);
  return units;
}

/** Convert a string value from the given unit to metres (static)
 * @param value The value to change
 * @param start_unit Initial unit
 * @returns A new string value in metres
 */
QString ShapeDetails::convertToMetres(const QString & value, Unit start_unit)
{
  QString converted;
  switch( start_unit )
  {
  case ShapeDetails::centimetre: 
    converted = QString::number(value.toDouble() / 100.0);
    break;
  case ShapeDetails::millimetre: 
    converted = QString::number(value.toDouble() / 1000.0);
    break;
  default: 
    converted = value;
  }
  return converted;
}

//--------------------------------------------//
//                Sphere 
//--------------------------------------------//
/// Static counter
int SphereDetails::g_nspheres = 0;

/// Default constructor
SphereDetails::SphereDetails(QWidget *parent) : ShapeDetails(parent)
{
  //Update number of sphere objects and the set the ID of this one
  ++g_nspheres;
  m_idvalue = "sphere_" + QString::number(g_nspheres);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //radius
  m_radius_box = new QLineEdit;
  m_runits = createLengthUnitsCombo();
  QHBoxLayout *rad_layout = new QHBoxLayout;
  rad_layout->addWidget(new QLabel("Radius: "));
  rad_layout->addWidget(m_radius_box);
  rad_layout->addWidget(m_runits);

  m_centre = new PointGroupBox;
  m_centre->setTitle("Centre");
  
  main_layout->addLayout(rad_layout);
  main_layout->addWidget(m_centre);
}

/**
 * Write the XML definition
 */
QString SphereDetails::writeXML() const
{
  QString valr("0.0");
  if( !m_radius_box->text().isEmpty() )
  {
    valr = convertToMetres(m_radius_box->text(), ShapeDetails::Unit(m_runits->currentIndex()));
  }
  QString xmldef = 
    "<sphere id=\"" + m_idvalue + "\">\n" + m_centre->write3DElement("centre") +
    "<radius val=\"" + valr + "\" />\n"
    "</sphere>\n";
  return xmldef;
}

//--------------------------------------------------------//
//                Cylinder
//--------------------------------------------------------//
/// Static counter
int CylinderDetails::g_ncylinders = 0;

/// Default constructor
CylinderDetails::CylinderDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ncylinders;
  m_idvalue = "cylinder_" + QString::number(g_ncylinders);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //radius
  m_radius_box = new QLineEdit;
  m_runits = createLengthUnitsCombo();
  QHBoxLayout *rad_layout = new QHBoxLayout;
  rad_layout->addWidget(new QLabel("Radius: "));
  rad_layout->addWidget(m_radius_box);
  rad_layout->addWidget(m_runits);

  //height
  m_height_box = new QLineEdit;
  m_hunits = createLengthUnitsCombo();
  QHBoxLayout *hgt_layout = new QHBoxLayout;
  hgt_layout->addWidget(new QLabel("Height:  "));
  hgt_layout->addWidget(m_height_box);
  hgt_layout->addWidget(m_hunits);

  //Point boxes
  m_lower_centre = new PointGroupBox;
  m_lower_centre->setTitle("Bottom Base Centre");

  m_axis = new PointGroupBox;
  m_axis->setTitle("Axis");
  
  main_layout->addLayout(rad_layout);
  main_layout->addLayout(hgt_layout);
  main_layout->addWidget(m_lower_centre);
  main_layout->addWidget(m_axis);
}

/**
 * Write the XML definition
 */
QString CylinderDetails::writeXML() const
{
  QString valr("0.0"), valh("0.0");
  if( !m_radius_box->text().isEmpty() )
  {
    valr = convertToMetres(m_radius_box->text(), ShapeDetails::Unit(m_runits->currentIndex()));
  }
  if( !m_height_box->text().isEmpty() )
  {
    valh = convertToMetres(m_height_box->text(), ShapeDetails::Unit(m_hunits->currentIndex()));
  }
  QString xmldef = 
    "<cylinder id=\"" + m_idvalue + "\" >\n"
    "<radius val=\"" + valr + "\" />\n"
    "<height val=\"" + valh + "\" />\n" + 
    m_lower_centre->write3DElement("centre-of-bottom-base") +
    m_axis->write3DElement("axis") +
    "</cylinder>\n";
  return xmldef;
}

//--------------------------------------------------------//
//                InfiniteCylinder
//--------------------------------------------------------//
/// Static counter
int InfiniteCylinderDetails::g_ninfcyls = 0;

/// Default constructor
InfiniteCylinderDetails::InfiniteCylinderDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ninfcyls;
  m_idvalue = "infcyl_" + QString::number(g_ninfcyls);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //radius
  m_radius_box = new QLineEdit;
  m_runits = createLengthUnitsCombo();
  QHBoxLayout *rad_layout = new QHBoxLayout;
  rad_layout->addWidget(new QLabel("Radius: "));
  rad_layout->addWidget(m_radius_box);
  rad_layout->addWidget(m_runits);

  //Point boxes
  m_centre = new PointGroupBox;
  m_centre->setTitle("Centre");

  m_axis = new PointGroupBox;
  m_axis->setTitle("Axis");
  
  main_layout->addLayout(rad_layout);
  main_layout->addWidget(m_centre);
  main_layout->addWidget(m_axis);
}

/**
 * Write the XML definition
 */
QString InfiniteCylinderDetails::writeXML() const
{
  QString valr("0.0");
  if( !m_radius_box->text().isEmpty() )
  {
    valr = convertToMetres(m_radius_box->text(), ShapeDetails::Unit(m_runits->currentIndex()));
  }
  QString xmldef = 
    "<infinite-cylinder id=\"" + m_idvalue + "\" >\n"
    "<radius val=\"" + valr + "\" />\n" +
    m_centre->write3DElement("centre") +
    m_axis->write3DElement("axis") +
    "</cylinder>\n";
  return xmldef;
}

