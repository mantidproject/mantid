#ifndef H_MDWS_SLICEDESCR
#define H_MDWS_SLICEDESCR

#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid
{
namespace MDEvents
{
 /***  The class responsible for building Momentums Transformation Matrix for ConvertToMDEvents algorithm
    *  from the input parameters of the algorithm and parameters, retrieved from input and 
    *  (if availible) output MD workspace
    *
    *   
      
    @date 2012-03-20

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MDWSTransfDescr
{
public:
    MDWSTransfDescr(){};
 
  /** helper function which verifies if projection vectors are specified and if their values are correct when present.
      * sets default values u and v to [1,0,0] and [0,1,0] if not present or any error. */
    void checkUVsettings(const std::vector<double> &ut,const std::vector<double> &vt,MDEvents::MDWSDescription &TargWSDescription)const;

   /** function provides the linear representation for the transformation matrix, which translate momentums from laboratory to crystal cartezian 
       (C)- Busing, Levi 1967 coordinate system */
   std::vector<double> getTransfMatrix(const std::string &inWsName,MDEvents::MDWSDescription &TargWSDescription)const;
   /**function returns the linear representation for the transformation matrix, which transforms momentums from laboratory to target coordinate system
     defined by existing workspace */
    std::vector<double> getTransfMatrix( API::IMDEventWorkspace_sptr spws,API::MatrixWorkspace_sptr inWS)const; 

   /// get transformation matrix currently defined for the algorithm
   std::vector<double> getTransfMatrix()const{return m_TargWSDescription.rotMatrix;}
   /// construct meaningful dimension names and :
   void buildDimensions(MDEvents::MDWSDescription &TargWSDescription);
private:

   MDEvents::MDWSDescription m_TargWSDescription;

  /// logger -> to provide logging, for MD dataset file operations
   static Mantid::Kernel::Logger& convert_log;

};

}
}

#endif