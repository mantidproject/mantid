#ifndef MANTID_ALGORITHMS_EXTRACTMASK_H_
#define MANTID_ALGORITHMS_EXTRACTMASK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms 
  {
    /**
      Extract the masking from a given workspace.

      The output workspce is a MaskWorkspace with a single X bin where:
      <UL>
      <LI>1 = masked;</LI>
      <LI>0 = unmasked.</LI>
      </UL>

      The spectra containing 0 are also marked as masked and the instrument 
      link is preserved so that the instrument view functions correctly.

      Required Properties:
      <UL>
      <LI> InputWorkspace  - The name of the input workspace. </LI>
      <LI> OutputWorkspace - The name of the output mask workspace </LI>
      </UL>
           
      Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
      
      File change history is stored at: <https://github.com/mantidproject/mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport ExtractMask : public Mantid::API::Algorithm
    {
    public:
      /// Constructor
      ExtractMask() : Mantid::API::Algorithm() {}
      /// Virtual destructor
      virtual ~ExtractMask() {}
      /// Algorithm's name
      virtual const std::string name() const { return "ExtractMask"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Extracts the masking from a given workspace and places it in a new workspace.";}

      /// Algorithm's version
      virtual int version() const { return 1; }
      /// Algorithm's category for identification
      virtual const std::string category() const { return "Transforms\\Masking"; }

    private:
      
      /// Initialisation code
      void init();
      ///Execution code
      void exec();
    };

  }
}

#endif // MANTID_ALGORITHMS_EXTRACTMASK_H_
