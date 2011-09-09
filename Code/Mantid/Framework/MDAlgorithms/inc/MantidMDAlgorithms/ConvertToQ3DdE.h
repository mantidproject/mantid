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

/** MakeDiffractionMDEventWorkspace :
   *  Transfrom processed inelastic workspace into MD(Event)Workspace with 3 components of momentum transfer plus energy transfer for Indirect?(should be expanded) geometry instrument
   * 
   * @author Alex Buts, ISIS
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
    std::vector<Kernel::V3D>  det_dir; // unit vector pointing from the sample to the detector;
    std::vector<size_t>       det_id;   // the detector ID;
    //
    bool is_defined(void)const{return det_dir.size()>0;}
    bool is_defined(size_t new_size)const{return det_dir.size()==new_size;}
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
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

    /// Map of all the detectors in the instrument
    detid2det_map allDetectors;
    /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
    /// Matrix. Multiply this by the lab frame Qx, Qy, Qz to get the desired Q or HKL.
    Kernel::Matrix<double> mat;

  /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;

    // the variable which keeps preprocessed positions of the detectors;
    static preprocessed_detectors det_loc;
    // the function, which calculates these positions;
    static void process_detectors_positions(const DataObjects::Workspace2D_const_sptr inWS2D);

   template <class T>
   void convertEventList(int workspaceIndex);
  };


} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
