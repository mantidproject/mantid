#ifndef MANTID_MDEVENTS_WS_DESCRIPTION_DEPR_H
#define MANTID_MDEVENTS_WS_DESCRIPTION_DEPR_H
/**TODO: FOR DEPRICATION */ 

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"



namespace Mantid
{
namespace MDEvents
{
 /*** Class wrapping together all parameters, related to conversion from a workspace to MDEventoWorkspace
*
* used to provide common interface for subclasses, dealing with creation of MD workspace and conversion of
* ws data into MDEvents
*
* It also defines some auxiliary functions, used for convenient description of MD workspace
*
@date 2011-28-12

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/


/// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm.
class DLLExport MDWSDescriptionDepricated : public MDWSDescription
{
public: 

    bool isDetInfoLost()const{return isDetInfoLost(m_InWS);}

    ConvToMDPreprocDet const * getDetectors(){return m_DetLoc;}
    ConvToMDPreprocDet const * getDetectors()const{return m_DetLoc;}

   /// constructor
    MDWSDescriptionDepricated(unsigned int nDimensions=0);

  /// method builds MD Event description from existing MD event workspace
   void buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS);  
  /// method builds MD Event ws description from a matrix workspace and the transformations, requested to be performed on the workspace
   void buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
                            const std::vector<std::string> &dimProperyNames = std::vector<std::string>());

   void setDetectors(const ConvToMDPreprocDet &g_DetLoc);
   /** checks if matrix ws has information about detectors*/
    static bool isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D);
private:
   // pointer to the array of detector's directions in the reciprocal space
    ConvToMDPreprocDet const * m_DetLoc;
    /// the vector of MD coordinates, which are obtained from workspace properties.

};

}
}
#endif