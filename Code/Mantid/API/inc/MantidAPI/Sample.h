#ifndef MANTID_API_SAMPLE_H_
#define MANTID_API_SAMPLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Objects/Object.h"

namespace Mantid
{
namespace API
{
/** This class stores information about the sample used in a particular experimental run,
    and some of the run parameters.
    This is mainly garnered from logfiles.

    @author Russell Taylor, Tessella Support Services plc
    @date 26/11/2007

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Sample
{
public:
  /// Default constructor
  Sample();
  /// Virtual destructor
  virtual ~Sample();

  /// Set the name of the sample
  void setName( const std::string &name );
  /// Returns the name of the sample
  const std::string& getName() const;
  /// Set the geometrical shape of the sample
  void setShapeObject(const Geometry::Object & sample_shape);
  /// Returns the geometrical shape of the sample
  const Geometry::Object& getShapeObject() const;

  /// Sets the geometry flag
  void setGeometryFlag(int geom_id);
  /// Returns the geometry flag
  int getGeometryFlag() const;
  /// Sets the thickness
  void setThickness(double thick);
  /// Returns the thickness
  double getThickness() const;
  /// Sets the height
  void setHeight(double height);
  /// Returns the height
  double getHeight() const;
  /// Sets the width
  void setWidth(double width);
  /// Returns the width
  double getWidth() const;
  
  /// Copy constructor. 
  Sample(const Sample& copy);
  /// Copy assignment operator. 
  const Sample& operator=(const Sample& rhs);

private: 
  /// The name for the sample
  std::string m_name;
  /// The sample shape object
  Geometry::Object m_sample_shape;
  /// The sample geometry flag
  int m_geom_id;
  /// The sample thickness from the SPB_STRUCT in the raw file
  double m_thick;
  /// The sample height from the SPB_STRUCT in the raw file
  double m_height;
  /// The sample width from the SPB_STRUCT in the raw file
  double m_width;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_SAMPLE_H_*/
