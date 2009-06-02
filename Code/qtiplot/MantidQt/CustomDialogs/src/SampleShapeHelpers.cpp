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

  m_yunits->setItemText(0, "mm");
  m_zunits->setItemText(0, "mm");
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
  m_yunits->setItemText(0, "deg");
  m_zunits->setItemText(0, "deg");
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

/**
 * Set the complement flag
 * @param flag The value of the flag
 */
void ShapeDetails::setComplementFlag(bool flag)
{
  m_isComplement = flag;
}

/**
 * Get the complement flag
 * @returns The value of the complement flag
 */
bool ShapeDetails::getComplementFlag() const
{
  return m_isComplement;
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
    "</infinite-cylinder>\n";
  return xmldef;
}

//--------------------------------------------------------//
//                SliceOfCylinderRing
//--------------------------------------------------------//
/// Static counter
int SliceOfCylinderRingDetails::g_ncylrings = 0;

/// Default constructor
SliceOfCylinderRingDetails::SliceOfCylinderRingDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ncylrings;
  m_idvalue = "cylslice_" + QString::number(g_ncylrings);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //inner radius
  m_rinner_box = new QLineEdit;
  m_iunits = createLengthUnitsCombo();
  QHBoxLayout *rad_layout = new QHBoxLayout;
  rad_layout->addWidget(new QLabel("Inner radius: "));
  rad_layout->addWidget(m_rinner_box);
  rad_layout->addWidget(m_iunits);
  //Outer
  m_router_box = new QLineEdit;
  m_ounits = createLengthUnitsCombo();
  QHBoxLayout *rad2_layout = new QHBoxLayout;
  rad2_layout->addWidget(new QLabel("Outer radius: "));
  rad2_layout->addWidget(m_router_box);
  rad2_layout->addWidget(m_ounits);
  //Depth
  m_depth_box = new QLineEdit;
  m_dunits = createLengthUnitsCombo();
  QHBoxLayout *dep_layout = new QHBoxLayout;
  dep_layout->addWidget(new QLabel("       Depth: "));
  dep_layout->addWidget(m_depth_box);
  dep_layout->addWidget(m_dunits);
  
  //Arc
  m_arc_box = new QLineEdit;
  QHBoxLayout *arc_layout = new QHBoxLayout;
  arc_layout->addWidget(new QLabel("Arc: "));
  arc_layout->addWidget(m_arc_box);
  arc_layout->addWidget(new QLabel(" deg "));
  
  main_layout->addLayout(rad_layout);
  main_layout->addLayout(rad2_layout);
  main_layout->addLayout(dep_layout);
  main_layout->addLayout(arc_layout);

}

/**
 * Write the XML definition
 */
QString SliceOfCylinderRingDetails::writeXML() const
{
  QString valir("0.0"), valor("0.0"), vald("0.0"), vala("0.0");
  if( !m_rinner_box->text().isEmpty() )
  {
    valir = convertToMetres(m_rinner_box->text(), ShapeDetails::Unit(m_iunits->currentIndex()));
  }
  if( !m_router_box->text().isEmpty() )
  {
    valor = convertToMetres(m_router_box->text(), ShapeDetails::Unit(m_ounits->currentIndex()));
  }
  if( !m_depth_box->text().isEmpty() )
  {
    vald = convertToMetres(m_depth_box->text(), ShapeDetails::Unit(m_dunits->currentIndex()));
  }
  if( !m_arc_box->text().isEmpty() )
  {
    vala = m_arc_box->text();
  }
  
  QString xmldef = 
    "<slice-of-cylinder-ring id=\"" + m_idvalue + "\" >\n"
    "<inner-radius val=\"" + valir + "\" />\n"
    "<outer-radius val=\"" + valor + "\" />\n"
    "<depth val=\"" + vald + "\" />\n"
    "<arc val=\"" + vala + "\" />\n"
    "</slice-of-cylinder-ring>\n";
  
  return xmldef;
}

//--------------------------------------------------------//
//                Cone
//--------------------------------------------------------//
/// Static counter
int ConeDetails::g_ncones = 0;

/// Default constructor
ConeDetails::ConeDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ncones;
  m_idvalue = "cone_" + QString::number(g_ncones);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //Height
  m_height_box = new QLineEdit;
  m_hunits = createLengthUnitsCombo();
  QHBoxLayout *hgt_layout = new QHBoxLayout;
  hgt_layout->addWidget(new QLabel("Height: "));
  hgt_layout->addWidget(m_height_box);
  hgt_layout->addWidget(m_hunits);

  //Angle
  m_angle_box = new QLineEdit;
  QHBoxLayout *ang_layout = new QHBoxLayout;
  ang_layout->addWidget(new QLabel("Angle: "));
  ang_layout->addWidget(m_angle_box);
  ang_layout->addWidget(new QLabel(" deg "));
 
  //Point boxes
  m_tippoint = new PointGroupBox;
  m_tippoint->setTitle("Tip point");

  m_axis = new PointGroupBox;
  m_axis->setTitle("Base-to-Tip Axis");

  main_layout->addLayout(hgt_layout);
  main_layout->addLayout(ang_layout);
  main_layout->addWidget(m_tippoint);
  main_layout->addWidget(m_axis);
}

/**
 * Write the XML definition
 */
QString ConeDetails::writeXML() const
{
  QString valh("0.0"), valan("0.0");
  if( !m_height_box->text().isEmpty() )
  {
    valh = convertToMetres(m_height_box->text(), ShapeDetails::Unit(m_hunits->currentIndex()));
  }
  if( !m_angle_box->text().isEmpty() )
  {
    valan = m_angle_box->text();
  }

  QString xmldef = 
    "<cone id=\"" + m_idvalue + "\" >\n"
    "<height val=\"" + valh + "\" />\n"
    "<angle val=\"" + valan + "\" />\n" +
    m_tippoint->write3DElement("tip-point") + 
    m_axis->write3DElement("axis") + 
    "</cone>\n";
  
  return xmldef;
}

//--------------------------------------------------------//
//                InfiniteCone
//--------------------------------------------------------//
/// Static counter
int InfiniteConeDetails::g_ninfcones = 0;

/// Default constructor
InfiniteConeDetails::InfiniteConeDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ninfcones;
  m_idvalue = "infcone_" + QString::number(g_ninfcones);

  QVBoxLayout *main_layout = new QVBoxLayout(this);
  //Angle
  m_angle_box = new QLineEdit;
  QHBoxLayout *ang_layout = new QHBoxLayout;
  ang_layout->addWidget(new QLabel("Angle: "));
  ang_layout->addWidget(m_angle_box);
  ang_layout->addWidget(new QLabel(" deg "));
 
  //Point boxes
  m_tippoint = new PointGroupBox;
  m_tippoint->setTitle("Tip point");

  m_axis = new PointGroupBox;
  m_axis->setTitle("Base-to-Tip Axis");

  main_layout->addLayout(ang_layout);
  main_layout->addWidget(m_tippoint);
  main_layout->addWidget(m_axis);
}

/**
 * Write the XML definition
 */
QString InfiniteConeDetails::writeXML() const
{
  QString valan("0.0");
  if( !m_angle_box->text().isEmpty() )
  {
    valan = m_angle_box->text();
  }

  QString xmldef = 
    "<infinite-cone id=\"" + m_idvalue + "\" >\n"
    "<angle val=\"" + valan + "\" />\n" +
    m_tippoint->write3DElement("tip-point") + 
    m_axis->write3DElement("axis") + 
    "</infinite-cone>\n";
  
  return xmldef;
}

//--------------------------------------------------------//
//                InfinitePlane
//--------------------------------------------------------//
/// Static counter
int InfinitePlaneDetails::g_ninfplanes = 0;

/// Default constructor
InfinitePlaneDetails::InfinitePlaneDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ninfplanes;
  m_idvalue = "infplane_" + QString::number(g_ninfplanes);

  QVBoxLayout *main_layout = new QVBoxLayout(this);

  //Point boxes
  m_plane = new PointGroupBox;
  m_plane->setTitle("Point in plane");

  m_normal = new PointGroupBox;
  m_normal->setTitle("Point normal to plane");

  main_layout->addWidget(m_plane);
  main_layout->addWidget(m_normal);
}

/**
 * Write the XML definition
 */
QString InfinitePlaneDetails::writeXML() const
{
  QString xmldef = 
    "<infinite-plane id=\"" + m_idvalue + "\" >\n" +
    m_plane->write3DElement("point-in-plane") + 
    m_normal->write3DElement("normal-to-plane") + 
    "</infinite-plane>\n";
  
  return xmldef;
}

//--------------------------------------------------------//
//                Cuboid
//--------------------------------------------------------//
/// Static counter
int CuboidDetails::g_ncuboids = 0;

/// Default constructor
CuboidDetails::CuboidDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_ncuboids;
  m_idvalue = "cuboid_" + QString::number(g_ncuboids);

  QVBoxLayout *main_layout = new QVBoxLayout(this);

  //Point boxes
  m_left_frt_bot = new PointGroupBox;
  m_left_frt_bot->setTitle("Left front bottom point");

  m_left_frt_top = new PointGroupBox;
  m_left_frt_top->setTitle("Left front top point");
  
  m_left_bck_bot = new PointGroupBox;
  m_left_bck_bot->setTitle("Left back bottom point");

  m_right_frt_bot = new PointGroupBox;
  m_right_frt_bot->setTitle("Right front bottom point");

  main_layout->addWidget(m_left_frt_bot);
  main_layout->addWidget(m_left_frt_top);
  main_layout->addWidget(m_left_bck_bot);
  main_layout->addWidget(m_right_frt_bot);

}

/**
 * Write the XML definition
 */
QString CuboidDetails::writeXML() const
{
  QString xmldef = 
    "<cuboid id=\"" + m_idvalue + "\" >\n" +
    m_left_frt_bot->write3DElement("left-front-bottom-point") + 
    m_left_frt_top->write3DElement("left-front-top-point") + 
    m_left_bck_bot->write3DElement("left-back-bottom-point") + 
    m_right_frt_bot->write3DElement("right-front-bottom-point") + 
    "</cuboid>\n";
  return xmldef;
}

//--------------------------------------------------------//
//                Hexahedron
//--------------------------------------------------------//
/// Static counter
int HexahedronDetails::g_nhexahedrons = 0;

/// Default constructor
HexahedronDetails::HexahedronDetails(QWidget *parent) : ShapeDetails(parent)
{
  /// Update number of sphere objects and the set the ID of this one
  ++g_nhexahedrons;
  m_idvalue = "hexahedron_" + QString::number(g_nhexahedrons);

  QVBoxLayout *main_layout = new QVBoxLayout(this);

  //Point boxes
  m_left_bck_bot = new PointGroupBox;
  m_left_bck_bot->setTitle("Left back bottom point");

  m_left_frt_bot = new PointGroupBox;
  m_left_frt_bot->setTitle("Left front bottom point");

  m_right_frt_bot = new PointGroupBox;
  m_right_frt_bot->setTitle("Right front bottom point");

  m_right_bck_bot = new PointGroupBox;
  m_right_bck_bot->setTitle("Right back bottom point");

  m_left_bck_top = new PointGroupBox;
  m_left_bck_top->setTitle("Left back top point");

  m_left_frt_top = new PointGroupBox;
  m_left_frt_top->setTitle("Left front top point");

  m_right_frt_top = new PointGroupBox;
  m_right_frt_top->setTitle("Right front top point");

  m_right_bck_top = new PointGroupBox;
  m_right_bck_top->setTitle("Right back top point");

  main_layout->addWidget(m_left_bck_bot);
  main_layout->addWidget(m_left_frt_bot);
  main_layout->addWidget(m_right_bck_bot);
  main_layout->addWidget(m_right_frt_bot);
  main_layout->addWidget(m_left_bck_top);
  main_layout->addWidget(m_left_frt_top);
  main_layout->addWidget(m_right_bck_top);
  main_layout->addWidget(m_right_frt_top);
  
}

/**
 * Write the XML definition
 */
QString HexahedronDetails::writeXML() const
{
  QString xmldef = 
    "<hexahedron id=\"" + m_idvalue + "\" >\n" +
    m_left_bck_bot->write3DElement("left-back-bottom-point") + 
    m_left_frt_bot->write3DElement("left-front-bottom-point") + 
    m_right_bck_bot->write3DElement("right-back-bottom-point") + 
    m_right_frt_bot->write3DElement("right-front-bottom-point") + 
    m_left_bck_top->write3DElement("left-back-top-point") + 
    m_left_frt_top->write3DElement("left-front-top-point") + 
    m_right_bck_top->write3DElement("right-back-top-point") + 
    m_right_frt_top->write3DElement("right-front-top-point") + 
    "</hexahedron>\n";
  return xmldef;
}

// This is not implemented in OpenCascade yet 

//--------------------------------------------------------//
//                Torus
//--------------------------------------------------------//
/// Static counter
// int TorusDetails::g_ntori = 0;

// /// Default constructor
// TorusDetails::TorusDetails(QWidget *parent) : ShapeDetails(parent)
// {
//   /// Update number of sphere objects and the set the ID of this one
//   ++g_ntori;
//   m_idvalue = "torus_" + QString::number(g_ntori);

//   QVBoxLayout *main_layout = new QVBoxLayout(this);

//   //Radi
//   m_tube_rad = new QLineEdit;
//   m_tunits = createLengthUnitsCombo();
//   QHBoxLayout *tub_layout = new QHBoxLayout;
//   tub_layout->addWidget(new QLabel("Tube radius: "));
//   tub_layout->addWidget(m_tube_rad);
//   tub_layout->addWidget(m_tunits);

//   m_inner_rad = new QLineEdit;
//   m_iunits = createLengthUnitsCombo();
//   QHBoxLayout *hol_layout = new QHBoxLayout;
//   hol_layout->addWidget(new QLabel("Hole radius: "));
//   hol_layout->addWidget(m_inner_rad);
//   hol_layout->addWidget(m_iunits);
  
//   //Point boxes
//   m_centre = new PointGroupBox;
//   m_centre->setTitle("Centre");

//   m_axis = new PointGroupBox;
//   m_axis->setTitle("Axis");

//   main_layout->addLayout(tub_layout);
//   main_layout->addLayout(hol_layout);
//   main_layout->addWidget(m_centre);
//   main_layout->addWidget(m_axis);
// }

// /**
//  * Write the XML definition
//  */
// QString TorusDetails::writeXML() const
// {
//   QString valt("0.0"), vali("0.0");
//   if( !m_tube_rad->text().isEmpty() )
//   {
//     valt = convertToMetres(m_tube_rad->text(), ShapeDetails::Unit(m_tunits->currentIndex()));
//   }
//   if( !m_inner_rad->text().isEmpty() )
//   {
//     vali = convertToMetres(m_inner_rad->text(), ShapeDetails::Unit(m_iunits->currentIndex())); 
//   }
//   QString xmldef = 
//     "<torus id=\"" + m_idvalue + "\" >\n"
//     "<radius-tube val=\"" + valt + "\" />\n"
//     "<radius-from-centre-to-tube val=\"" + vali + "\" />\n" +
//     m_centre->write3DElement("centre") +
//     m_axis->write3DElement("axis") +
//     "</torus>\n";
//   return xmldef;
// }
