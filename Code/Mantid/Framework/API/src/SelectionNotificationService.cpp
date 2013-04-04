#include "MantidAPI/SelectionNotificationService.h"
#include "MantidKernel/DataService.h"
/*
 * SelectionNotificationService.cpp
 *
 *  Created on: Mar 21, 2013
 *      Author: ruth
 */

namespace Mantid
{
  namespace API
  {


    SelectionNotificationServiceImpl::SelectionNotificationServiceImpl():
              Mantid::Kernel::DataService<std::vector<double> >("SelectionNotificationService")
      {
      }

    SelectionNotificationServiceImpl:: ~SelectionNotificationServiceImpl()
    {

    }

  } // Namespace API
 } // Namespace Mantid



