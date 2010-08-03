#ifndef MANTID_DATAHANDLING_LOAD_H_
#define MANTID_DATAHANDLING_LOAD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a workspace from a data file. The algorithm tries to determine the actual type
    of the file (raw, nxs, ...) and use the specialized loading algorith to load it.

    @author Roman Tolchenov, Tessella plc
    @date 38/07/2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Load : public API::Algorithm
    {
    public:
      Load(){}
      ~Load() {}
      virtual const std::string name() const { return "Load"; }
      virtual const int version() const { return 1; }
      virtual const std::string category() const { return "DataHandling"; }

    private:
      void init();
      void exec();

      /// Run LoadRaw
      void runLoadRaw(API::IAlgorithm_sptr&);
      /// Run LoadNexus
      void runLoadNexus(API::IAlgorithm_sptr&);
      /// Run LoadAscii
      void runLoadAscii(API::IAlgorithm_sptr&);
      /// Run LoadSPE
      void runLoadSPE(API::IAlgorithm_sptr&);
      /// Run LoadSpice2D
      void runLoadSpice2D(API::IAlgorithm_sptr&);
      /// Set the output workspace(s)
      void setOutputWorkspace(API::IAlgorithm_sptr&);
      /// Set the output workspace
      void setOutputMatrixWorkspace(API::IAlgorithm_sptr&);

    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOAD_H_  */
