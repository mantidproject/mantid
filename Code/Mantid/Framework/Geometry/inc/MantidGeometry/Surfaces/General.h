#ifndef General_h
#define General_h

#include "MantidKernel/System.h"
#include "MantidGeometry/Surfaces/Quadratic.h"

namespace Mantid
{

  namespace Kernel
  {
    class Logger;
  }

namespace Geometry
{

/**
  \class General
  \brief Holds a general quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a general surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  which has been defined as a gq surface in MCNPX.
  It is a realisation of the Surface object.

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

class DLLExport General : public Quadratic
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
 public:
    
  General();
  General(const General&);
  General* clone() const;
  General& operator=(const General&);
  ~General();
  
  int setSurface(const std::string&);
  void setBaseEqn();
  void getBoundingBox(double &xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin);

};

}  // NAMESPACE Geometry

}  // NAMESPACE Mantid

#endif
