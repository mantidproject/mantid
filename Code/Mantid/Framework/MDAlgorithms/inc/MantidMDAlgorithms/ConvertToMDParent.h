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

/** ConvertToMD :
   *  Transfrom a workspace into MD workspace with components defined by user. 
   *
   * Gateway for number of ChildAlgorithms, some are very important, some are questionable 
   * Intended to cover wide range of cases; 
   *
   * The description of the algorithm is avalible at: <http://www.mantidproject.org/ConvertToMD> 
   * The detailed description of the algoritm is provided at: <http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation>

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

    static Mantid::Kernel::Logger & getLogger();

  private:
    void init();
    virtual void exec()=0;
   /// Sets documentation strings for this algorithm
    virtual void initDocs()=0;  
   //------------------------------------------------------------------------------------------------------------------------------------------
   protected: 
  /// logger -> to provide logging, for MD dataset file operations
   static Mantid::Kernel::Logger& g_Log;

 };

} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
