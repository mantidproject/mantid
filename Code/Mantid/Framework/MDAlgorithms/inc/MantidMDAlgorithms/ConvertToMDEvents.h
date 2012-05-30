#ifndef MANTID_MD_CONVERT2_MDEVENTS
#define MANTID_MD_CONVERT2_MDEVENTS


#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/ConvToMDEventsBase.h"
//
#include "MantidMDEvents/ConvToMDPreprocDet.h"


namespace Mantid
{
namespace MDAlgorithms
{

/** ConvertToMDEvents :
   *  Transfrom a workspace into MD workspace with components defined by user. 
   *
   * Gateway for number of subalgorithms, some are very important, some are questionable 
   * Intended to cover wide range of cases; 

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
 
 
/// Convert to MD Events class itself:
  class DLLExport ConvertToMDEvents  : public MDEvents::BoxControllerSettingsAlgorithm
  {
  public:
    ConvertToMDEvents();
    ~ConvertToMDEvents();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToMDEvents";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}  


  private:
    void init();
    void exec();
   /// Sets documentation strings for this algorithm
    virtual void initDocs();
   /// pointer to the input workspace;
   Mantid::API::MatrixWorkspace_sptr inWS2D;
   /// the pointer to class which keeps output MD workspace and is responsible for adding data to N-dimensional workspace;
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper;
   /// the variable which keeps preprocessed positions of the detectors if any availible (TODO: should it be a table ws and separate algorithm?);
   static MDEvents::ConvToMDPreprocDet det_loc;  
  /// progress reporter
   std::auto_ptr<API::Progress > pProg;
    /// logger -> to provide logging, for MD dataset file operations
   static Mantid::Kernel::Logger& convert_log;
   /// pointer to the class, which does the particular conversion
   boost::shared_ptr<MDEvents::ConvToMDEventsBase> pConvertor;
  
   /// the class which knows about existing subalgorithms and generates alforithm ID as function of input parameters of this algorithm. 
    ///ConvertToMD::ConvertToMDEventsParams ParamParser;   
    /// The class which keeps map of all existing subalgorithms converting to MDEventWorkspace.
    /// It returns the pointer to the subalgorithm receiving alogID from ParamParser. Shoud be re-implemented through a singleton if used not only here. 
    //ConvertToMDEventsSubalgFactory  subAlgFactory;
  //------------------------------------------------------------------------------------------------------------------------------------------
    protected: //for testing
        static Mantid::Kernel::Logger & getLogger();

 };

} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
