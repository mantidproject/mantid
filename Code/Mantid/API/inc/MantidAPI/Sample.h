#ifndef MANTID_API_SAMPLE_H_
#define MANTID_API_SAMPLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidGeometry/Object.h"

namespace Mantid
{
namespace API
{
/** This class stores information about the sample used in a particular experimental run,
    and some of the run parameters.
    This is mainly garnered from logfiles.

    @author Russell Taylor, Tessella Support Services plc
    @date 26/11/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
  Sample();
  virtual ~Sample();

  void setName( const std::string &name );
  const std::string& getName() const;

  void addLogData( Kernel::Property *p );
  Kernel::Property* getLogData( const std::string &name ) const;
  const std::vector< Kernel::Property* >& getLogData() const;

  void setProtonCharge( const double &charge);
  const double& getProtonCharge() const;

  void setShapeObject(boost::shared_ptr<Geometry::Object> sample_shape);
  boost::shared_ptr<Geometry::Object> getShapeObject() const;

  void setGeometryFlag(int geom_id);
  int getGeometryFlag() const;

  void setGeometry(double thick, double height, double width);
  void getGeometry(double &thick, double &height, double &width) const;

private:
  /// Private copy constructor. NO COPY ALLOWED!
  Sample(const Sample&);
  /// Private copy assignment operator. NO COPY ASSIGNMENT ALLOWED!
  Sample& operator=(const Sample&);

  /// The name for the sample
  std::string m_name;
  /// Stores the information read in from the logfiles.
  Kernel::PropertyManager m_manager;
  /// The good proton charge for this run in uA.hour
  double m_protonCharge;
  /// The sample shape object
  boost::shared_ptr<Geometry::Object> m_sample_shape;
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
