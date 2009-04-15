#ifndef MANTIDQT_CUSTOMDIALOGS_SAMPLESHAPEHELPERS_H_
#define MANTIDQT_CUSTOMDIALOGS_SAMPLESHAPEHELPERS_H_

//--------------------------------------
// Includes
//--------------------------------------
#include <QWidget>
#include <QGroupBox>

//--------------------------------------
// Qt forward declarations
//--------------------------------------
class QLineEdit;
class QComboBox;
class QRadioButton;
class QLabel;

/**
 * The classes defined here encapuslate the layout and
 * parameters of the individual shapes within Mantid. Each is
 * a widget that is to be displayed within the CreateSampleShapeDialog.
 * The base class exists so that they can be stored in a container.
 */
namespace MantidQt
{
namespace CustomDialogs
{

/**
 * A custom group box for a 3D point
 */
class PointGroupBox : public QGroupBox
{
  Q_OBJECT;

public:
  //Default constructor
  PointGroupBox(QWidget* parent = 0);

  ///Write the element tag for a 3D point
  QString write3DElement(const QString & elem_name) const;

private slots:
  // Switch to cartesian coordinates
  void changeToCartesian();
  // Switch to spherical coordinates
  void changeToSpherical();
  
private:
  //Labels for fields
  QLabel *m_xlabel, *m_ylabel, *m_zlabel;
  // Edit fields (also used for r,theta,phi) if in spherical mode
  QLineEdit *m_midx, *m_midy, *m_midz;
  //Unit choice boxes (x is used for r in spherical mode)
  QComboBox *m_xunits, *m_yunits, *m_zunits;
  //Radio button selection for coordinates
  QRadioButton *m_cartesian, *m_spherical;
  //The current coordinate system (0 = cartesian, 1 = spherical)
  int m_icoord;
};


/**
 * A struct describing a binary operation
 * Note: The constructor takes an integer where 0 = intersection, 1 = union and 2 = difference
 */
struct Operation
{
  /// Default constructor
  Operation(int op = 0) : binaryop(op) {}
  
  /// Return the string that represnts the result of this operation
  QString toString(QString left, QString right) const;
  
  /// The stored operation
  int binaryop;
};

/**
 * The base class for the details widgets
 */
class ShapeDetails : public QWidget
{
  Q_OBJECT

public:
  ///Constructor
  ShapeDetails(QWidget *parent = 0) : QWidget(parent) {}
  ///Constructor
  virtual ~ShapeDetails() {}

  ///Write out the XML definition for this shape
  virtual QString writeXML() const = 0;

  /// Get the id string
  QString getShapeID() const
  { 
    return m_idvalue;
  }

  ///Create a new length units box
  static  QComboBox* createLengthUnitsCombo();
  // Units enum
  enum Unit { millimetre = 0, centimetre = 1, metre = 2 };
  // Convert a string value from the given unit to metres
  static QString convertToMetres(const QString & value, Unit start_unit);

protected:
  /// ID string of this object
  QString m_idvalue;

};

/**
 * A widget to define a sphere 
 */
class SphereDetails : public ShapeDetails
{
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_nspheres;

public:
  ///Default constructor
  SphereDetails(QWidget *parent = 0);

  ///Default destructor 
  ~SphereDetails() { --g_nspheres; }

  //Write the XML definition of a sphere
  QString writeXML() const;

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
class CylinderDetails : public ShapeDetails
{
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ncylinders;

public:
  ///Default constructor
  CylinderDetails(QWidget *parent = 0);

  ///Default destructor 
  ~CylinderDetails() { --g_ncylinders; }

  //Write the XML definition of a sphere
  QString writeXML() const;

private:
  /// Line edits to enter values
  QLineEdit *m_radius_box, *m_height_box;
  //Unit choice boxes
  QComboBox *m_runits, *m_hunits;
  /// Centre and axis point boxes
  PointGroupBox *m_lower_centre, *m_axis;
};

/**
 * A widget to define an infinite cylinder 
 */
class InfiniteCylinderDetails : public ShapeDetails
{
  Q_OBJECT

private:
  /// The number of objects that currently exist
  static int g_ninfcyls;

public:
  ///Default constructor
  InfiniteCylinderDetails(QWidget *parent = 0);

  ///Default destructor 
  ~InfiniteCylinderDetails() { --g_ninfcyls; }

  //Write the XML definition of a sphere
  QString writeXML() const;

private:
  /// Line edits to enter values
  QLineEdit *m_radius_box;
  //Unit choice boxes
  QComboBox *m_runits;
  /// Centre and axis point boxes
  PointGroupBox *m_centre, *m_axis;
};

/**
 * Base instantiator to store in a map
 */
struct BaseInstantiator
{
  /// Default constructor
  BaseInstantiator() {}
  ///Create an instance
  virtual ShapeDetails* createInstance() const = 0;
private:
  /// Private copy constructor
  BaseInstantiator(const BaseInstantiator&);
  /// Private assignment operator
  BaseInstantiator& operator =(const BaseInstantiator&);
};

/**
 * A structure used for holding the type of a details widget
 */
template<class T>
struct ShapeDetailsInstantiator : public BaseInstantiator
{
  /// Default constructor
  ShapeDetailsInstantiator() {}
  ///Create an instance of this type
  ShapeDetails* createInstance() const
  {
    return static_cast<ShapeDetails*>(new T);
  }

private:
  /// Private copy constructor
  ShapeDetailsInstantiator(const ShapeDetailsInstantiator&);
  /// Private assignment operator
  ShapeDetailsInstantiator& operator =(const ShapeDetailsInstantiator&);
};


}
}

#endif // MANTIDQT_CUSTOMDIALOGS_SAMPLESHAPEHELPERS_H_
