#ifndef MANTID_KERNEL_ANALYSISDATASERVICE_H_
#define MANTID_KERNEL_ANALYSISDATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DataService.h"
#include "MantidAPI/DllExport.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{
/** The Analysis data service stores instances of the Workspace objects and
    anything that derives from template class DynamicFactory<Mantid::Kernel::IAlgorithm>.
    This is the primary data service that
    the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    This is the manager/owner of Workspace* when registered.

    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    @author L C Chapon, ISIS, Rutherford Appleton Laboratory

    Modified to inherit from DataService
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratory

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
class AnalysisDataServiceImpl : public Kernel::DataService<API::Workspace>
{
private:
  friend struct Mantid::Kernel::CreateUsingNew<AnalysisDataServiceImpl>;
  // Constructors
  AnalysisDataServiceImpl();
  /// Private, unimplemented copy constructor
  AnalysisDataServiceImpl(const AnalysisDataServiceImpl&);
  /// Private, unimplemented copy assignment operator
  AnalysisDataServiceImpl& operator=(const AnalysisDataServiceImpl&);
  ~AnalysisDataServiceImpl();
};

///Forward declaration of a specialisation of SingletonHolder for AnalysisDataServiceImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
  template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AnalysisDataServiceImpl>;
#endif /* _WIN32 */
  typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AnalysisDataServiceImpl> AnalysisDataService;

} // Namespace API
} // Namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICE_H_*/
