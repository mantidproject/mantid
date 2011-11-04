/*
  This custom action searches for previous installed products and returns all that match
  regardless off their install context, i.e. per-machine and per-user.

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#include "stdafx.h"

/**
 * Helper to check if an installation has machine context, i.e. needs admin rights to touch
 */
BOOL isInstallContextPerMachine(const TCHAR * productNumber)
{
  WCHAR wszAssignmentType[10] = {0};
  DWORD cchAssignmentType = sizeof(wszAssignmentType)/sizeof(wszAssignmentType[0]);
  MsiGetProductInfo(productNumber,INSTALLPROPERTY_ASSIGNMENTTYPE,wszAssignmentType,&cchAssignmentType);
  BOOL perMachine(FALSE);
  if (L'1' == wszAssignmentType[0]) // This means per-machine
  {
    WcaLog(LOGMSG_STANDARD, "Related product %S was a per-machine installation.", productNumber);
    perMachine = TRUE;
  }
  else
  {
    WcaLog(LOGMSG_STANDARD, "Related product %S was a per-user installation.", productNumber);
  }
  return perMachine;
}

/**
 * This is the interface point for this action
 */
UINT __stdcall FindOldMachineContextInstall(MSIHANDLE hInstall)
{
  HRESULT hr = S_OK;
  UINT er = ERROR_SUCCESS;

  hr = WcaInitialize(hInstall, "FindOldMachineContextInstall");
  ExitOnFailure(hr, "Failed to initialize");
  WcaLog(LOGMSG_STANDARD, "Initialized.");

  DWORD guidBufferLength(39);
  TCHAR upgradeCodeBuf[39] = {0};
  er = MsiGetProperty(hInstall, TEXT("UpgradeCode"), upgradeCodeBuf, &guidBufferLength);
  if( er == ERROR_SUCCESS )
  {
    WcaLog(LOGMSG_STANDARD, "Found upgrade code of current product: %S",upgradeCodeBuf);
    TCHAR productNumberBuf[39] = {0};
    // At most there can be two previously installed packages. A per-machine
    // that could not have been removed and a per-user from a package after the
    // admin->non-admin switch. We need to find the one that was a per-machine one
    // as that is the one that has to be removed manually.
    for(int index = 0; index < 2; ++index)
    {
      UINT error = MsiEnumRelatedProducts(upgradeCodeBuf,0,index,productNumberBuf);
      if( error == ERROR_SUCCESS && isInstallContextPerMachine(productNumberBuf) )
      {
        // Set the name of the product so it can be shown to the user. The buffer needs
        // to be big enough
        TCHAR productName[50];
        DWORD nameBufferLength(50); // This will be altered to the correct size with the function below
        MsiGetProductInfo(productNumberBuf, INSTALLPROPERTY_PRODUCTNAME, productName, &nameBufferLength);
        er = MsiSetProperty(hInstall, TEXT("MACHINE_CONTEXT_NAME"), productName);
        break;
      }
      else if( error == ERROR_NO_MORE_ITEMS ) break;
      else continue;
    }
  }
  else
  {
    WcaLog(LOGMSG_STANDARD, "Error retrieving upgrade code");
  }

LExit:
  return WcaFinalize(er);
}
