#ifndef Algebra_h
#define Algebra_h

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Math/BnId.h"
#include "MantidGeometry/Math/Acomp.h"

namespace Mantid
{

namespace Geometry
{

/*!
  \class  Algebra
  \brief Computes Boolean algebra for simplification
  \author S. Ansell
  \date August 2005
  \version 1.0

  A leveled algebra class for each
  level of factorisation. 

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

*/ 

class DLLExport Algebra
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger

  std::map<int,std::string> SurfMap;     ///< Internal surface map
  Acomp F;       ///< Factor
  
 public:

  Algebra();
  Algebra(const Algebra&);
  Algebra& operator=(const Algebra&);
  ~Algebra();

    /// Accessor
  const Acomp& getComp() const { return F; }  

  bool operator==(const Algebra&) const;
  bool operator!=(const Algebra&) const;
  Algebra& operator+=(const Algebra&);
  Algebra& operator*=(const Algebra&);
  Algebra operator+(const Algebra&) const;
  Algebra operator*(const Algebra&) const;
  int logicalEqual(const Algebra&) const;


  void Complement();
  void makeDNF() { F.makeDNFobject(); }  ///< assessor to makeDNFobj
  void makeCNF() { F.makeCNFobject(); }  ///< assessor to makeCNFobj
  std::pair<Algebra,Algebra> algDiv(const Algebra&) const;
  int setFunctionObjStr(const std::string&);
  int setFunction(const std::string&);
  int setFunction(const Acomp&);

  std::ostream& write(std::ostream&) const;
  std::string writeMCNPX() const;

  // Debug Functions::
  int countLiterals() const;
  ///Displays the algebra string
  std::string display() const;

};

std::ostream&
operator<<(std::ostream&,const Algebra&);

}  // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif 
