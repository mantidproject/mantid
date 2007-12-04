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
  
  Flux();                        ///< Constructor
  Flux(const Flux&);             ///< Copy Constructor
  Flux& operator=(const Flux&);  ///< Copy assignment operator
  ~Flux();                       ///< Destructor

  void zeroSize(const int);                ///< Zero the vector?
  void addEvent(const int,const double);   ///< Add an event
  void addCnt() { nCnt++; }                ///< Increment the number of events

  int getCnt() const { return nCnt; }              ///< Get the number of events
  std::vector<double>& getEvents() { return I; }   ///< Get the vector of events
  const std::vector<double>& getEvents() const     ///< Get the vector of events (const version)
    { return I; }

};

  
}

#endif
