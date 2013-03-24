/*
 * SelectionNotificationService.h
 *
 *  Created on: Mar 21, 2013
 *      Author: ruth
 */

/**
 * The singleton version of this class is intended to be used for messages between the
 * Single Crystal Reduction Interface and Visual elements of the data with peaks.  Some examples
 * of possible information that can be shared are points in space( in Qlab, hkl,...)and regions in
 * space.  This information could be selected or show selection.
 *
 */
#ifndef SELECTIONNOTIFICATIONSERVICE_H_
#define SELECTIONNOTIFICATIONSERVICE_H_


#include "MantidKernel/DataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"

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
 // SelectionNotificationServiceImpl(const SelectionNotificationServiceImpl&);
  friend struct Mantid::Kernel::CreateUsingNew<SelectionNotificationServiceImpl>;
  /// Private destructor
  virtual ~SelectionNotificationServiceImpl();

};


///Forward declaration of a specialisation of SingletonHolder for SelectionNotificationServiceImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
  template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<SelectionNotificationServiceImpl>;
#endif /* _WIN32 */
  typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<SelectionNotificationServiceImpl> SelectionNotificationService;

} // Namespace API
} // Namespace Mantid

#endif /* SELECTIONNOTIFICATIONSERVICE_H_ */
