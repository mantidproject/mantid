#ifndef H_ICONVERT_TO_MDEVENTS_METHODS
#define H_ICONVERT_TO_MDEVENTS_METHODS

#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidAPI/MatrixWorkspace.h"
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
       NoQ,     //< no Q transformatiom, just copying values along X axis (may be with units transformation)
       modQ,    //< calculate mod Q
       Q3D,      //< calculate 3 component of Q in fractional coordinate system.
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
        Axis       // typical for events
    };

 class ConvertToMDEvents;
 /// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm. 
  struct MDWSDescription
  {
  public:
      /// constructor
      MDWSDescription():n_activated_dimensions(0),emode(-1){};
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    size_t n_activated_dimensions;
    /// conversion mode (see its description below)
    int emode;
    /// minimal and maximal values for the workspace dimensions:
    std::vector<double>      dim_min,dim_max;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> dim_names;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dim_units;
    /// the matrix to transform momentums of the workspace into notional target coordinate system
    std::vector<double> rotMatrix;  // should it be the Quat?
    /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent(Mantid::Kernel::Logger& log)const;
 
  }; 


 class IConvertToMDEventMethods
 {
 public:
    ///method which initates all main class variables (constructor in fact)
    virtual void setUPConversion(ConvertToMDEvents *pAlgo){
        TWS   = pAlgo->TWS;
        inWS2D= pAlgo->inWS2D;
    };
    /// method which starts the conversion procedure
    virtual void runConversion()=0;
    /// virtual destructor
    virtual ~IConvertToMDEventMethods(){};
 protected:
   /// pointer to the input workspace;
    Mantid::API::MatrixWorkspace_sptr inWS2D;
    /// the properties of the requested target MD workpsace:
    MDWSDescription TWS;
 
   /** function extracts the coordinates from additional workspace porperties and places them to proper position within the vector of MD coodinates */
   bool fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties);
 private:
    /// internal function which do one peace of work, which should be performed by one thread
   virtual void conversionChunk()=0;
};

 

} // end namespace MDAlgorithms
} // end namespace Mantid
#endif