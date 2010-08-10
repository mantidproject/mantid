#ifndef MANTIDAPI_LOCATEDDATAREF_H
#define MANTIDAPI_LOCATEDDATAREF_H

#include "MantidKernel/System.h"
#include "MantidAPI/ILocatedData.h"

namespace Mantid
{
namespace API
{
/**
  The LocatedDataRef class holds references to all of the data required to describe a single data item of a point dataset.
  All of the data items are held as pointers to data owned by classes other than itself.  
  This is done for performance reasons when iterating over workspaces.

  \author N. Draper
    
  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport LocatedDataRef : public ILocatedData
{
  public:
  const double& X() const;
  const double& E() const;

  double& X();
  double& E();

  const double& Y() const;
  double& Y();

  const double& X2() const; ///< Returns the X value of the end of the histogram bin
  double& X2();             ///< Returns the X value of the end of the histogram bin

  bool isHistogram() const; ///<Returns true if the data point is hastogram data and therefore has an X2.

  double* xPointer;        ///< Pointer to the value of X
  double* x2Pointer;        ///< Pointer to the value of X
  double* yPointer;        ///< Pointer to the value of Y
  double* ePointer;        ///< Pointer to the value of E

  LocatedDataRef* clone() const;

  LocatedDataRef();
  LocatedDataRef(const LocatedDataRef&);
  LocatedDataRef& operator=(const LocatedDataRef&);
  LocatedDataRef& operator=(const ILocatedData&);
  virtual ~LocatedDataRef();

  int operator<(const LocatedDataRef&) const;
  int operator>(const LocatedDataRef&) const;
  int operator==(const LocatedDataRef&) const;
  int operator!=(const LocatedDataRef&) const;
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_LOCATEDDATAREF_H
