#ifndef MANTIDAPI_HISTDATAREF_H
#define MANTIDAPI_HISTDATAREF_H

#include "MantidKernel/System.h"
#include "MantidAPI/PointDataRef.h"
#include "MantidAPI/IHistData.h"

namespace Mantid
{

namespace API
{

  //forward declaration
  class IErrorHelper;

/**
  IDataItem of an X value, two error values E and E2 together with a pointer to an ErrorHelper and a specta number.
  Class maintians a type first/second/third triple
  similar to std::pair except all are identical

  \author N. Draper
    
  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
  class DLLExport HistDataRef : public PointDataRef, public IHistData
{
  public:
 
  const double& X2() const;
  double& X2();

  double* x2Pointer;       ///< Pointer to the value of X2
 
  HistDataRef();
  HistDataRef(const HistDataRef&);
  HistDataRef& operator=(const HistDataRef&);
  HistDataRef& operator=(const IHistData&);
  virtual ~HistDataRef();

  int operator<(const HistDataRef&) const;
  int operator>(const HistDataRef&) const;
  int operator==(const HistDataRef&) const;
  int operator!=(const HistDataRef&) const;
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_HISTDATAREF_H
