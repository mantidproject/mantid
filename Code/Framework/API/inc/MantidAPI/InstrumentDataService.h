#ifndef MANTID_API_INSTRUMENTDATASERVICE_
#define MANTID_API_INSTRUMENTDATASERVICE_

#include "MantidAPI/DllExport.h"
#include "MantidKernel/DataService.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
  namespace API
  {
    /** InstrumentDataService Class. Derived from DataService.
    Class to store shared_pointer to Instrument Objects.

    @author Laurent C Chapon, ISIS, Rutherford Appleton Laboratory
    @date 30/05/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTID_API InstrumentDataServiceImpl : public Mantid::Kernel::DataService<Mantid::Geometry::Instrument>
    {
    private:
      friend struct Mantid::Kernel::CreateUsingNew<InstrumentDataServiceImpl>;
      /// Constructor
      InstrumentDataServiceImpl();
      /// Private, unimplemented copy constructor
      InstrumentDataServiceImpl(const InstrumentDataServiceImpl&);
      /// Private, unimplemented copy assignment operator
      InstrumentDataServiceImpl& operator=(const InstrumentDataServiceImpl&);
      /// Destructor
      ~InstrumentDataServiceImpl();
    };

    ///Forward declaration of a specialisation of SingletonHolder for AnalysisDataServiceImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
    // this breaks new namespace declaraion rules; need to find a better fix
    template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<InstrumentDataServiceImpl>;
#endif /* _WIN32 */
    typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<InstrumentDataServiceImpl> InstrumentDataService;

  } // Namespace API
} // Namespace Mantid
#endif /*INSTRUMENTDATASERVICE_*/
