#ifndef MANTID_GEOMETRY_TRACK_H_
#define MANTID_GEOMETRY_TRACK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"

namespace Mantid
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace Geometry
{
/*!
  \struct TUnit
  \version 1.0
  \author S. Ansell
  \brief For a leg of a track

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
struct DLLExport TUnit
{
  V3D PtA;           ///< Init Point
  V3D PtB;           ///< Exit Point
  double Dist;         ///< Total distance from track begin
  double Length;       ///< Total distance Distance  [at end]
  int ObjID;           ///< ObjectID

  TUnit(const Geometry::V3D& A,const Geometry::V3D& B,const double D,const int ID);

  /// Less than operator
  int operator<(const TUnit& A) const
    { return Dist<A.Dist; }

  /// Less than operator
  int operator<(const double& A) const
    { return Dist<A; }
};

/*!
  Ordering for TPartial is special since we need
  that when dist is close that the +/- flag is taken into
  account.
 */
struct TPartial
{
  int ObjID;           ///< ObjectID
  int Direction;            ///< Flag direction
  V3D PtA;             ///< Point
  double Dist;         ///< Total distance from track begin

  TPartial(const int ID,const int flag,const Geometry::V3D& PVec,const double D);

  int operator<(const TPartial& A) const;
};

/*!
  \class Track
  \version 1.0
  \author S. Ansell
  \brief Order List of track units.
 */
class DLLExport Track
{
 public:
  typedef std::vector<TUnit> LType;       ///< Type for the Link storage
  typedef std::vector<TPartial> PType;    ///< Type for the partial

 private:
  static Kernel::Logger& PLog;           ///< The official logger

  Geometry::V3D iPt;                ///< Start Point
  Geometry::V3D uVec;               ///< unit vector to direction
  int iObj;                         ///< Initial object
  LType Link;                       ///< Track units
  PType surfPoints;                 ///< Track units

 public:

  Track(const Geometry::V3D& StartPt,const Geometry::V3D& UV,const int initObj=0);
  Track(const Track&);
  Track& operator=(const Track&);
  ~Track();

  void addPoint(const int ID,const int Direct,const Geometry::V3D& Pt);
  int addTUnit(const int ID,const Geometry::V3D& Apt,const Geometry::V3D& Bpt,
	       const double D);

  void removeCoJoins();
  void buildLink();

  // get/set
  void setFirst(const Geometry::V3D&,const Geometry::V3D&);
  const Geometry::V3D& getInit() const { return iPt; }         ///< Get the start point
  const Geometry::V3D& getUVec() const { return uVec; }        ///< Get the direction

  LType::const_iterator begin() const { return Link.begin(); }   ///< Iterator pointing to start of collection
  LType::const_iterator end() const { return Link.end(); }       ///< Iterator pointing one-past-the-end of collection
  int count() const { return static_cast<int>(Link.size()); }       ///< The number of link items in the track

  // tests
  int nonComplete() const;
};

}  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_TRACK_H_*/
