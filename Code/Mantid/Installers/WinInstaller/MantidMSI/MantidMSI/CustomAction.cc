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

// DllMain - Initialize and cleanup WiX custom action utils.
extern "C" BOOL WINAPI DllMain(
  __in HINSTANCE hInst,
  __in ULONG ulReason,
  __in LPVOID
  )
{
  switch(ulReason)
  {
  case DLL_PROCESS_ATTACH:
    WcaGlobalInitialize(hInst);
    break;

  case DLL_PROCESS_DETACH:
    WcaGlobalFinalize();
    break;
  }

  return TRUE;
}
