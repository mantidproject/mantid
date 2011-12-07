#ifndef MANTID_MD_CONVERT2_QXYZ_DE_H_
#define MANTID_MD_CONVERT2_QXYZ_DE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace MDAlgorithms
{

/** ConvertToDiffractionMDWorkspace :
   *  Transfrom processed inelastic workspace into MD(Event)Workspace with 3 components of momentum transfer plus energy transfer for Indirect?(should be expanded) geometry instrument
   * 
   * @modyfier Alex Buts, ISIS; copypasted with minimal variatuions from ConvertToDiffractionMDWorkspace;  @author Janik Zikovsky, SNS  @date 2011-03-01 
   * @date 07-09-2011

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
  /** the lightweight class below contain 3D uint vectors, pointing to the positions of the detectors
      This vector used to preprocess and catch the partial positions of the detectors in Q-space
      to avoid repetative calculations, and (possibly) to write these data as part of the physical compression scheme
      in a very common situation when the physical instrument does not change in all runs, contributed into MD workspace
   */
  struct preprocessed_detectors{
    double L1;
    Kernel::V3D   minDetPosition;    // minimal and
    Kernel::V3D   maxDetPosition;    // maxinal position for the detectors
    std::vector<Kernel::V3D>  det_dir; // unit vector pointing from the sample to the detector;
    std::vector<double>       L2;
    std::vector<double>       TwoTheta;
    std::vector<int32_t>      det_id;   // the detector ID;
    //
    bool is_defined(void)const{return det_dir.size()>0;}
    bool is_defined(size_t new_size)const{return det_dir.size()==new_size;}
    double  * const pL2(){return &L2[0];}
    double  * const pTwoTheta(){return &TwoTheta[0];}
    Kernel::V3D  *const pDetDir(){return &det_dir[0];}
  };
  class DLLExport ConvertToQ3DdE  : public API::Algorithm
  {
  public:
    ConvertToQ3DdE();
    ~ConvertToQ3DdE();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToQ3DdE";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Inelastic;MDAlgorithms";}

  /** the function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
    static void process_detectors_positions(const DataObjects::Workspace2D_const_sptr inWS2D);

  /** function provides the linear representation (9 elements) for the transformation matrix FROM: TO:  */
   std::vector<double> get_transf_matrix(API::MatrixWorkspace_sptr inWS2D,const Kernel::V3D &u, const Kernel::V3D &v)const;
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

      /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
 
  /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;

    // the variable which keeps preprocessed positions of the detectors if any availible;
    static preprocessed_detectors det_loc;  

   // helper function which does exatly what it says
   void check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max);
  };


} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
