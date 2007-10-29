#ifndef Detector_h
#define Detector_h

namespace MonteCarlo
{

/*!
  \class Detector
  \version 1.0
  \date Aug 2006
  \author S. Ansell
  \brief Detector class
  
  Class to keep track of single and multiple
  scattering to a detector. The detectors are considered
  as infinitly vertical (Z)
*/
  
class Detector
{
 private:
  
  typedef std::map<int,Flux> fMap;          ///< Storage of flux

  int nDet;           ///< Number of detector items 

  double nTotal;      ///< Number of events
  double nScat;       ///< Number of single scatter events
  double nInc;        ///< Number of incoherrent scatter events
  double nMult;       ///< Number of multiscatter events
  
  std::vector<double> Theta;          ///< Angle of the detetor (theta)
  std::vector<double> Phi;            ///< Phi (vertical angle)
  std::vector<Geometry::Vec3D> DPos;  ///< Position in space

  fMap Single;           ///< Single flux [INDEX Layer]
  fMap Incoh;            ///< Single flux [INDEX Layer]
  fMap Multiple;         ///< Multiple flux [INDEX Layer]

 public:

  Detector();
  Detector(const Detector&);
  Detector& operator=(const Detector&);
  ~Detector();

  void setDist(const double);
  void setAngles(const double,const double,const double);
  void addPosition(const Geometry::Vec3D&);
  
  void addNeutron() { nTotal+=1.0; }

  int getNDet() const { return nDet; }
  ///< Angle
  const std::vector<double>& getTheta() const { return Theta; } 
  /// Position in space
  const std::vector<Geometry::Vec3D>& getPos() const
    { return DPos; }

  Flux& getSingleFlux(const int);
  Flux& getIncohFlux(const int);
  Flux& getMultipleFlux(const int);

  std::vector<double> getSingle(const int) const;
  std::vector<double> getMultiple(const int) const;

  void addSingleCnt(const double);
  void addIncohCnt(const double);
  void addMultCnt(const double);

  void write(std::ostream&) const;

};

}   // NAMESPACE MonteCarlo

#endif
