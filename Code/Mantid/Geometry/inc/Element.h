#ifndef Element_h
#define Element_h

/// Stores isotope data
struct Abundance
{
  int Z;                               ///< Z of the element
  std::vector<int> WList;              ///< Atomic Weight of the components
  std::vector<double> Frac;            ///< Fractional weight

  Abundance();
  explicit Abundance(const int);          ///< Constructor
  Abundance(const Abundance&);            ///< Copy Constructor
  Abundance& operator=(const Abundance&); ///< Copy assignment operator
  ~Abundance();                           ///< Destructor

  void addIso(const int,const double);    ///< Add an isotope
  
};

/*!
  \class Element
  \brief Class to store/recall elemental symbols
  \author Stuart Ansell
  \date April 2005
  \version 1.1

  Simple class to return information about elements
  by numbers or symbols .
  Modified in 1.1 to include isotopes and atomic abundancy

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

class Element
{
 private:

  const int Nelem;                 ///< Max number of elements (approx 94)
  std::map<std::string,int> Nmap;  ///< Map of names to Z
  std::vector<std::string> Sym;    ///< Vector of symbols
  std::vector<double> KEdge;       ///< Vector of k-Edge [keV]
  std::vector<Abundance> Isotopes;  ///< Vector of Isotopes and abunances

  void populate();        ///< Initialise the map and symbol table
  void populateEdge();    ///< Initialise the Edge information
  void populateIso();    ///< Set the abundance and isotopes

 public:

  Element();
  ~Element();
  int elm(const char*) const;             ///< Determine Z from string
  int elm(const std::string&) const;      ///< Determine Z from string
  std::string elmSym(const int) const;    ///< Element symbol from Z

  double Kedge(const std::string&) const;    ///< Get K-edge from Symbol
  double Kedge(const int) const;             ///< Get K-edge from Z
  /// Process cinder zaids
  void addZaid(const std::string&,const double,
	       std::vector<int>&,std::vector<double>&) const;
};

#endif
