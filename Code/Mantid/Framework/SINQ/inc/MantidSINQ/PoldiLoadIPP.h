#ifndef MANTID_POLDI_PoldiLoadIPP_H_
#define MANTID_POLDI_PoldiLoadIPP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/System.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
  namespace Poldi
  {

  // N.B. PoldiLoadIPP is used to load IPP data in a Poldi ws
  /** @class PoldiLoadIPP PoldiLoadIPP.h Poldi/PoldiLoadIPP.h

      Part of the Poldi scripts set, used to load Poldi IPP data

      @author Christophe Le Bourlot, Paul Scherrer Institut - SINQ
      @date 05/06/2013

      Copyright © 2013 PSI-MSS

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
      Code Documentation is available at <http://doxygen.mantidproject.org>
  */
    class DLLExport PoldiLoadIPP : public API::Algorithm
    {
    public:
      /// Default constructor
    	PoldiLoadIPP(){};
      /// Destructor
      virtual ~PoldiLoadIPP() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "PoldiLoadIPP"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Poldi\\PoldiSet"; }



    protected:
      /// Overwrites Algorithm method
      void exec();


    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();


    };

  } // namespace Poldi
} // namespace Mantid

#endif /*MANTID_POLDI_PoldiLoadIPP_H_*/
