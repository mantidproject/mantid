#ifndef MANTID_MDALGORITHMS_CONVERT_TO_MD_PARENT_H_
#define MANTID_MDALGORITHMS_CONVERT_TO_MD_PARENT_H_


#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/ConvToMDBase.h"

#include "MantidKernel/DeltaEMode.h"


namespace Mantid
{
namespace MDAlgorithms
{

/** ConvertToMDParent :
   *  Main part of two algorithms which use ConvertToMD factory to transform points from instrument space to physical MD space
   *
   * The description of the algorithm is available at: <http://www.mantidproject.org/ConvertToMD> 
   * The detailed description of the algorithm is provided at: <http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation>

   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
 
 
/// Convert to MD Events class itself:
  class DLLExport ConvertToMDParent  : public MDEvents::BoxControllerSettingsAlgorithm
  {
  public:
    ConvertToMDParent(){};
    ~ConvertToMDParent(){};
    
    /// Algorithm's name for identification 
    virtual const std::string name() const=0;
    /// Algorithm's version for identification 
    virtual int version() const=0;
    /// Algorithm's category for identification
    virtual const std::string category() const;

   protected: 
    void init();
    // 
    DataObjects::TableWorkspace_const_sptr preprocessDetectorsPositions(const Mantid::API::MatrixWorkspace_const_sptr &InWS2D,const std::string &dEModeRequested,bool updateMasks,
                                                                         const std::string &preproc_detectorsWSName);
    DataObjects::TableWorkspace_sptr runPreprocessDetectorsToMDChildUpdatingMasks(const Mantid::API::MatrixWorkspace_const_sptr &InWS2D,const std::string &OutWSName,
                                                                                  const std::string &dEModeRequested,Kernel::DeltaEMode::Type &Emode);

  /// logger -> to provide logging, for MD dataset file operations
   static Mantid::Kernel::Logger& g_Log;

  /// pointer to the class, which does the particular conversion
   boost::shared_ptr<MDEvents::ConvToMDBase> m_Convertor; 

  /// Template to check if a variable equal to NaN
   template <class T>
   inline bool isNaN(T val)
   {
     volatile T buf=val;
     return (val!=buf);
   }


 };

} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
