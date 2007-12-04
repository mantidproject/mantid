#ifndef LineIntersectVisit_h
#define LineIntersectVisit_h


namespace Mantid
{

namespace Geometry
{

/*!
  \class LineIntersectionVisis
  \author S. Ansell
  \version 1.0
  \date September 2007 
  \brief Interset of Line with a surface 

  Creates interaction with a line
 */

class LineIntersectVisit : public BaseVisit
  {
  private:

  static Kernel::Logger& PLog;           ///< The official logger
    
    Line ATrack;                         ///< The line
    std::vector<Geometry::Vec3D> PtOut;  ///< The intersection point
    std::vector<double> DOut;            ///< The distance
    
    void procTrack();

  public:
    
    LineIntersectVisit(const Geometry::Vec3D&,
		       const Geometry::Vec3D&);
    /// Destructor
    virtual ~LineIntersectVisit() {};

    void Accept(const Surface&);
    void Accept(const Plane&);
    void Accept(const Sphere&);
    void Accept(const Cone&);
    void Accept(const Cylinder&);
    void Accept(const General&);
    
    // Accessor
    /// Get the distance
    const std::vector<double>& getDistance() const 
      { return DOut; }
    /// Get the intersection points
    const std::vector<Geometry::Vec3D>& getPoints() const 
      { return PtOut; }
    /// Get the number of intersection points
    int getNPoints() const { return PtOut.size(); }

    /// Re-set the line
    void setLine(const Geometry::Vec3D&,const Geometry::Vec3D&);

  };



}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
