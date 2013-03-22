/*
 * SelectionNotificationService.h
 *
 *  Created on: Mar 21, 2013
 *      Author: ruth
 */

#ifndef SELECTIONNOTIFICATIONSERVICE_H_
#define SELECTIONNOTIFICATIONSERVICE_H_


#include "MantidKernel/DataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include <Poco/AutoPtr.h>

namespace Mantid
{

namespace API
{

class  DLLExport SelectionNotificationServiceImpl : public Kernel::DataService<std::vector<double>>
{



private:

  /// Constructor
  SelectionNotificationServiceImpl();
  /// Private, unimplemented copy constructor
 // SelectionNotificationServiceImpl(const AnalysisDataServiceImpl&);

  /// Private destructor
  virtual ~SelectionNotificationServiceImpl(){}

};
///Forward declaration of a specialisation of SingletonHolder for AnalysisDataServiceImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
  template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<SelectionNotificationServiceImpl>;
#endif /* _WIN32 */
  typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<SelectionNotificationServiceImpl> SelectionNotificationService;

} // Namespace API
} // Namespace Mantid

#endif /* SELECTIONNOTIFICATIONSERVICE_H_ */
