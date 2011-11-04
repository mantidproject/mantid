#ifndef MANTID_MD_CONVERT2_Q_ND_ANY
#define MANTID_MD_CONVERT2_Q_ND_ANY
    
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
   *  Transfrom processed inelastic workspace into with components defined by user. 
   * 
   * @author Alex Buts, ISIS
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

  class DLLExport ConvertToQNDany  : public API::Algorithm
  {
  public:
    ConvertToQNDany();
    ~ConvertToQNDany();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToQNDany";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Inelastic;MDAlgorithms";}
    /// overload to the Algorithm property allowing to treat dependant properties;
    virtual void setPropertyValue(const std::string &name, const std::string &value);
 
  private:
    void init();
    void exec();
   /// Sets documentation strings for this algorithm
    virtual void initDocs();

      /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
 
  /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;

 
   /// helper function which does exatly what it says
   void check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max);
 
   /// function generates input properties from defaults
   bool build_default_properties(size_t max_nadd_dims=5);
   /// functon generates properties to build N-dimensional workspace from user selected workspace properties
   void build_ND_property_selector(size_t n_dims,const std::vector<std::string> & dim_ID_possible);
   /// the variable which describes the number of the dimensions, currently used by algorithm. Changes in input properties can change this number;
   size_t n_activated_dimensions;
   /// this variable describes default possible ID-s for Q-dimensions
   std::vector<std::string> Q_ID_possible;
  
  protected: //for testing
   /// function returns the list of names, which are possible dimensions for current matrix workspace
   std::vector<std::string > get_dimension_names(const std::vector<std::string> &default_prop,API::MatrixWorkspace_const_sptr inMatrixWS)const;

 };


} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
