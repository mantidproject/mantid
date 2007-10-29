#ifndef Flux_h
#define Flux_h


namespace MonteCarlo
{
  /*!
    \class Flux
    \version 1.0
    \author S. Ansell
    \date July 2007
    \brief Holds flux count for the Detector group
    
    The vector is over the angles 
  */

class Flux
{
 private:

  int nCnt;                  ///< Number of events
  std::vector<double> I;     ///< Flux at each event 
  
 public:
  
  Flux();
  Flux(const Flux&);
  Flux& operator=(const Flux&);
  ~Flux();

  void zeroSize(const int);
  void addEvent(const int,const double);
  void addCnt() { nCnt++; }

  int getCnt() const { return nCnt; }         
  std::vector<double>& getEvents() { return I; }
  const std::vector<double>& getEvents() const 
    { return I; }

};

  
}

#endif
