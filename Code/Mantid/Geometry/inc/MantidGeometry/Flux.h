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
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
