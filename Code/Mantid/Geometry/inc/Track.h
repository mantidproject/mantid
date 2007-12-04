#ifndef GeometryTrack_h
#define GeometryTrack_h

namespace Mantid
{

namespace Geometry
{

/*!
  \struct TUnit 
  \version 1.0
  \author S. Ansell
  \brief For a leg of a track
 */

struct TUnit
{
  Vec3D PtA;           ///< Init Point
  Vec3D PtB;           ///< Exit Point
  double Dist;         ///< Total distance from track begin
  double Length;       ///< Total distance Distance  [at end]
  int ObjID;           ///< ObjectID 


  TUnit(const Geometry::Vec3D& A,const Geometry::Vec3D& B,
	const double D,const int ID);

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
  Vec3D PtA;           ///< Point
  double Dist;         ///< Total distance from track begin

  TPartial(const int ID,const int flag,const Geometry::Vec3D& PVec,const double D);

  int operator<(const TPartial& A) const;

};

/*!
  \class Track
  \version 1.0
  \author S. Ansell
  \brief Order List of track units.
 */

class Track
{
 public:

  typedef std::vector<TUnit> LType;    ///< Type for the Link storage
  typedef std::vector<TPartial> PType;    ///< Type for the parital

 private:

  static Kernel::Logger& PLog;           ///< The official logger


  Geometry::Vec3D iPt;              ///< Start Point
  Geometry::Vec3D uVec;             ///< unit vector to direction
  int iObj;                         ///< Initial object
  LType Link;                       ///< Track units
  PType surfPoints;                 ///< Track units

 public:

  Track(const Geometry::Vec3D& StartPt,const Geometry::Vec3D& UV,
	const int initObj=0);
  Track(const Track&);
  Track& operator=(const Track&);
  ~Track();

  void addPoint(const int ID,const int Direct,const Geometry::Vec3D& Pt);
  int addTUnit(const int ID,const Geometry::Vec3D& Apt,const Geometry::Vec3D& Bpt);
  
  void removeCoJoins();
  void buildLink();   ///< Not implemented

  // get/set
  void setFirst(const Geometry::Vec3D&,const Geometry::Vec3D&);
  const Geometry::Vec3D& getInit() const { return iPt; }         ///< Get the start point
  const Geometry::Vec3D& getUVec() const { return uVec; }        ///< Get the direction

  LType::const_iterator begin() const { return Link.begin(); }   ///< Iterator pointing to start of collection
  LType::const_iterator end() const { return Link.end(); }       ///< Iterator pointing one-past-the-end of collection
    
  // tests
  int nonComplete() const;
  
};



} // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
