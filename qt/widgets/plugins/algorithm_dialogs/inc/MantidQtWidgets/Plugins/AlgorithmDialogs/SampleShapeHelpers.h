// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//--------------------------------------
// Includes
//--------------------------------------
#include <QGroupBox>
#include <QLineEdit>
#include <QWidget>

//--------------------------------------
// Qt forward declarations
//--------------------------------------
class QComboBox;
class QRadioButton;
class QLabel;

/**
 * The classes defined here encapuslate the layout and
 * parameters of the individual shapes within Mantid. Each is
 * a widget that is to be displayed within the CreateSampleShapeDialog.
 * The base class exists so that they can be stored in a container.
 */
namespace MantidQt {
namespace CustomDialogs {

// Forward declartion
class ShapeDetails;

/**
 * A custom group box for a 3D point
 */
class PointGroupBox : public QGroupBox {
  Q_OBJECT

public:
  // Default constructor
  PointGroupBox(QWidget *parent = nullptr);

  /// Write the element tag for a 3D point
  QString write3DElement(const QString &elem_name) const;

private slots:
  // Switch to cartesian coordinates
  void changeToCartesian();
  // Switch to spherical coordinates
  void changeToSpherical();

private:
  // Labels for fields
  QLabel *m_xlabel, *m_ylabel, *m_zlabel;
  // Edit fields (also used for r,theta,phi) if in spherical mode
  QLineEdit *m_midx, *m_midy, *m_midz;
  // Unit choice boxes (x is used for r in spherical mode)
  QComboBox *m_xunits, *m_yunits, *m_zunits;
  // Radio button selection for coordinates
  QRadioButton *m_cartesian, *m_spherical;
  // The current coordinate system (0 = cartesian, 1 = spherical)
  int m_icoord;
};

/**
 * A struct describing a binary operation
 * Note: The constructor takes an integer where 0 = intersection, 1 = union and
 * 2 = difference
 */
struct Operation {
  /// Default constructor
  Operation(int op = 0) : binaryop(op) {}

  /// Return the string that represnts the result of this operation
  QString toString(const QString &left, const QString &right) const;

  /// The stored operation
  int binaryop;
};

/**
 * Base instantiator to store in a map
 */
struct BaseInstantiator {
  /// Default constructor
  BaseInstantiator() {}
  /// Virtual destructor
  virtual ~BaseInstantiator() = default;
  /// Create an instance
  virtual ShapeDetails *createInstance() const = 0;

private:
  /// Private copy constructor
  BaseInstantiator(const BaseInstantiator &);
  /// Private assignment operator
  BaseInstantiator &operator=(const BaseInstantiator &);
};

/**
 * A structure used for holding the type of a details widget
 */
template <class T> struct ShapeDetailsInstantiator : public BaseInstantiator {
  /// Default constructor
  ShapeDetailsInstantiator() {}
  /// Create an instance of this type
  ShapeDetails *createInstance() const override { return static_cast<ShapeDetails *>(new T); }

private:
  /// Private copy constructor
  ShapeDetailsInstantiator(const ShapeDetailsInstantiator &);
  /// Private assignment operator
  ShapeDetailsInstantiator &operator=(const ShapeDetailsInstantiator &);
};

/**
 * The base class for the details widgets
 */
class ShapeDetails : public QWidget {
  Q_OBJECT

public:
  /// Constructor
  ShapeDetails(QWidget *parent = nullptr) : QWidget(parent), m_idvalue(""), m_isComplement(false) {}
  /// Constructor
  ~ShapeDetails() override = default;

  /// Write out the XML definition for this shape
  virtual QString writeXML() const = 0;

  const QString &getShapeID() const { return m_idvalue; }

  /// Create a new length units box
  static QComboBox *createLengthUnitsCombo();
  // Units enum
  enum Unit { millimetre = 0, centimetre = 1, metre = 2 };
  // Convert a string value from the given unit to metres
  static QString convertToMetres(const QString &value, Unit start_unit);

  /// Set the complement flag
  void setComplementFlag(bool flag);
  /// Get complement flag
  bool getComplementFlag() const;

protected:
  /// ID string of this object
  QString m_idvalue;

private:
  /// Take the complement of the shape
  bool m_isComplement;
};

/**
 * A widget to define a sphere
 */
class SphereDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_nspheres;

public:
  /// Default constructor
  SphereDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~SphereDetails() override { --g_nspheres; }

  // Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edit for radius value
  QLineEdit *m_radius_box;
  /// Radius unit choice
  QComboBox *m_runits;
  /// Centre point group box
  PointGroupBox *m_centre;
};

/**
 * A widget to define a cylinder
 */
class CylinderDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ncylinders;

public:
  /// Default constructor
  CylinderDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~CylinderDetails() override { --g_ncylinders; }

  // Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edits to enter values
  QLineEdit *m_radius_box, *m_height_box;
  // Unit choice boxes
  QComboBox *m_runits, *m_hunits;
  /// Centre and axis point boxes
  PointGroupBox *m_lower_centre, *m_axis;
};

/**
 * A widget to define an infinite cylinder
 */
class InfiniteCylinderDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ninfcyls;

public:
  /// Default constructor
  InfiniteCylinderDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~InfiniteCylinderDetails() override { --g_ninfcyls; }

  // Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edits to enter values
  QLineEdit *m_radius_box;
  // Unit choice boxes
  QComboBox *m_runits;
  /// Centre and axis point boxes
  PointGroupBox *m_centre, *m_axis;
};

/**
 * A widget to define an infinite cylinder
 */
class SliceOfCylinderRingDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ncylrings;

public:
  /// Default constructor
  SliceOfCylinderRingDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~SliceOfCylinderRingDetails() override { --g_ncylrings; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edits to enter values
  QLineEdit *m_rinner_box, *m_router_box, *m_depth_box, *m_arc_box;
  // Unit choice boxes
  QComboBox *m_iunits, *m_ounits, *m_dunits;
};

/**
 * A widget to define a cone
 */
class ConeDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ncones;

public:
  /// Default constructor
  ConeDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~ConeDetails() override { --g_ncones; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edits to enter values
  QLineEdit *m_height_box, *m_angle_box;
  // Unit choice boxes
  QComboBox *m_hunits;
  /// Centre and axis point boxes
  PointGroupBox *m_tippoint, *m_axis;
};

/**
 * A widget to define an infinite cone
 */
class InfiniteConeDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ninfcones;

public:
  /// Default constructor
  InfiniteConeDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~InfiniteConeDetails() override { --g_ninfcones; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Line edits to enter values
  QLineEdit *m_angle_box;
  /// Centre and axis point boxes
  PointGroupBox *m_tippoint, *m_axis;
};

/**
 * A widget to define an infinite plane
 */
class InfinitePlaneDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ninfplanes;

public:
  /// Default constructor
  InfinitePlaneDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~InfinitePlaneDetails() override { --g_ninfplanes; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Centre and axis point boxes
  PointGroupBox *m_plane, *m_normal;
};

/**
 * A widget to define an infinite plane
 */
class CuboidDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ncuboids;

public:
  /// Default constructor
  CuboidDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~CuboidDetails() override { --g_ncuboids; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Corner points
  PointGroupBox *m_left_frt_bot, *m_left_frt_top, *m_left_bck_bot, *m_right_frt_bot;
};

/**
 * A widget to define a hexahedron
 */
class HexahedronDetails : public ShapeDetails {
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_nhexahedrons;

public:
  /// Default constructor
  HexahedronDetails(QWidget *parent = nullptr);

  /// Default destructor
  ~HexahedronDetails() override { --g_nhexahedrons; }

  /// Write the XML definition of a sphere
  QString writeXML() const override;

private:
  /// Corner points
  PointGroupBox *m_left_bck_bot, *m_left_frt_bot, *m_right_frt_bot, *m_right_bck_bot, *m_left_bck_top, *m_left_frt_top,
      *m_right_frt_top, *m_right_bck_top;
};

// /**
//  * A widget to define a torus
//  */
// class TorusDetails : public ShapeDetails
// {
//   Q_OBJECT

// private:
//   /// The number of objects that currently exist
//   static int g_ntori;

// public:
//   ///Default constructor
//   TorusDetails(QWidget *parent = 0);

//   ///Default destructor
//   ~TorusDetails() { --g_ntori; }

//   /// Write the XML definition of a sphere
//   QString writeXML() const;

// private:
//   /// Radius values
//   QLineEdit *m_tube_rad, *m_inner_rad;
//   //Unit choice boxes
//   QComboBox *m_tunits, *m_iunits;
//   /// Corner points
//   PointGroupBox *m_centre, *m_axis;
// };
} // namespace CustomDialogs
} // namespace MantidQt
