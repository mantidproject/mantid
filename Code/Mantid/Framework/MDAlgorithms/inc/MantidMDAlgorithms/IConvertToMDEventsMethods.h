#ifndef H_ICONVERT_TO_MDEVENTS_METHODS
#define H_ICONVERT_TO_MDEVENTS_METHODS

#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
namespace Mantid
{
namespace MDAlgorithms
{
/** class describes the inteface to the methods, which perfoen the conversion from usual workspaces to MDEventWorkspace 
   *
   * @date 07-01-2012

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

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

  /// known sates for algorithms, caluclating momentums
  enum Q_state{
       modQ,    //< calculate mod Q
       Q3D,     //< calculate 3 component of Q in fractional coordinate system.
       NoQ,     //< no Q transformatiom, just copying values along X axis (may be with units transformation)
       NQStates  // number of various recognized Q-analysis modes used to terminate Q-state algorithms metalooop.
   };
  /**  known analysis modes, arranged according to emodes 
    *  It is importent to assign enums proper numbers, as direct correspondence between enums and their emodes 
    *  used by the external units conversion algorithms and this algorithm, so the agreement should be the stame     */
  enum AnalMode{  
      Elastic = 0,  //< int emode = 0; Elastic analysis
      Direct  = 1,  //< emode=1; Direct inelastic analysis mode
      Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
      ANY_Mode      //< couples with NoQ, means just copying existing data (may be doing units conversion), also used to terminate AnalMode algorithms initiation metaloop
  };
  /** enum describes if there is need to convert workspace units and different unit conversion modes 
   * this modes are identified by algorithm from workpace parameters and user input.   */
  enum CnvrtUnits   // here the numbers are specified to enable proper metaloop on conversion
  {
      ConvertNo,   //< no, input workspace has the same units as output workspace or in units used by Q-dE algorithms naturally
      ConvFast , //< the input workspace has different units from the requested and fast conversion is possible
      ConvByTOF,   //< conversion possible via TOF
      ConvFromTOF,  //< Input workspace units are the TOF 
      NConvUintsStates // number of various recognized unit conversion modes used to terminate CnvrtUnits algorithms initiation metalooop.
  };
  enum InputWSType  // Algorithm recognizes 2 input workspace types with different interface. 
  {
      Workspace2DType, //< 2D matirix workspace
      EventWSType,     //< Event worskapce
      NInWSTypes
  };
// way to treat the X-coorinate in the workspace:
    enum XCoordType
    {
        Histohram, // typical for Matrix workspace -- deploys central average 0.5(X[i]+X[i+1]); other types of averaging are possible if needed 
        Centered   // typical for events
    };

 
 class DLLExport IConvertToMDEventsMethods
 {
    template<Q_state Q,AnalMode MODE,CnvrtUnits CONV,XCoordType Type>
    friend struct COORD_TRANSFORMER;
 public:
     // constructor;
     IConvertToMDEventsMethods();
 
    ///method which initates all main class variables (constructor in fact)
    virtual size_t setUPConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc,const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper);
    /// method which starts the conversion procedure
    virtual void runConversion(API::Progress *)=0;
    /// virtual destructor
    virtual ~IConvertToMDEventsMethods(){};
/**> helper functions: To assist with units conversion done by separate class and get access to some important internal states of the subalgorithm */
    Kernel::Unit_sptr              getAxisUnits()const;
    PreprocessedDetectors const * pPrepDetectors()const{return pDetLoc;}
    double    getEi()const{return TWS.Ei;}
    int       getEMode()const{return TWS.emode;}
    API::NumericAxis *getPAxis(int nAaxis)const{return dynamic_cast<API::NumericAxis *>(this->inWS2D->getAxis(nAaxis));}
    std::vector<double> getTransfMatrix()const{return TWS.rotMatrix;}
//<------------------

   /** function extracts the coordinates from additional workspace porperties and places them to proper position within the vector of MD coodinates */
    bool fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties);
  protected:
   /// pointer to the input workspace;
    Mantid::API::MatrixWorkspace_sptr inWS2D;
    /// the properties of the requested target MD workpsace:
    MDEvents::MDWSDescription TWS;
    //
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper ;
   // pointer to the array of detector's directions in reciprocal space
    PreprocessedDetectors const *  pDetLoc;
   /// number of target ws dimesnions
    size_t n_dims;
    /// array of variables which describe min limits for the target variables;
    std::vector<double> dim_min;
    /// the array of variables which describe max limits for the target variables;
    std::vector<double> dim_max;
    /// index of current run(workspace) for MD WS combining
    uint16_t runIndex;
   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;
 private:
    /** internal function which do one peace of work, which should be performed by one thread 
      *
      *@param job_ID -- the identifier which specifies, what part of the work on the workspace this job has to do. 
                        Oftern it is a spectra number
      *
    */
   virtual size_t conversionChunk(size_t job_ID)=0;

};

 

} // end namespace MDAlgorithms
} // end namespace Mantid
#endif