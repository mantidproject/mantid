#ifndef MDALGORITHM_SIMULATEMDD_H_
#define MDALGORITHM_SIMULATEMDD_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDCell.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /** 
    SimulateMDD preforms a simulation of the data in the MDData workspace
    It inherits from the Algorithm class, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> InputMDData name      - The MDData object name </LI>
    <LI> Background model name - The background model name </LI>
    <LI> Foreground model name - The foreground model name </LI>
    <LI> OutputMDData name     - The output MDDdata object name </LI>
    </UL>

    @author Ron Fowler
    @date 22/11/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */
    class DLLExport SimulateMDD : public API::Algorithm
    {
    public:
      /// Default constructor
      SimulateMDD() : API::Algorithm() {};
      /// Destructor
      virtual ~SimulateMDD() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SimulateMDD";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      // Method to actually evaluate the bg model with the given parameters
      void SimBackground(std::string bgmodel,const double bgpara_p1,
             const double bgpara_p2, const double bgpara_p3);

    private:
      /// Initialisation code
      void init();
      ///Execution code
      void exec();
      /// Pointter to the cut data
      boost::shared_ptr<Mantid::API::IMDWorkspace> myCut;
    };

  } // namespace MDAlgorithm
} // namespace Mantid

#endif /*MDALGORITHM_SIMULATEMDD_H_*/
