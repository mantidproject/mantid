#ifndef H_CONVERT_TO_MDEVENTS_WSTYPE_INTERFACE
#define H_CONVERT_TO_MDEVENTS_WSTYPE_INTERFACE

#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDAlgorithms/IConvertToMDEventsWS.h"
/** class describes templated inteface to the methods, dealing with workspaces while performing conversion from usual workspaces to MDEventWorkspace 
    the template below should never be instantiated and us used for debug purposes only
    (instantiating it is actually impossible unless in debug mode, when adding new code)


   TEMPLATES INSTANSIATION: Users are welcome to specialize its own specific algorithm 
  *e.g.
   template<> class ConvertToMDEventsWS<ModQ,Elastic,ConvertNo,Centered,CrystalType>::public IConvertToMDEventsMethods
   {
       User specific code for workspace  processed to obtain ModQ in elastic mode, without unit conversion, 
       implementing something user defined for calculating x-coord and coord transformation, which will be invoked
       on ws with oriented lattice by writing the templated class and 
       Overloading the methods:
   public:
       size_t setUPConversion(Mantid::API::MatrixWorkspace_sptr , const PreprocessedDetectors &,const MDEvents::MDWSDescription &, boost::shared_ptr<MDEvents::MDEventWSWrapper> );
       void runConversion(API::Progress *);
   private:
      virtual size_t conversionChunk(size_t job_ID);
   }


   *
   * @date 26-04-2012

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
#undef  EXCLUDE_CONVERT_WORKSPACE_INTERFACE
#define EXCLUDE_CONVERT_WORKSPACE_INTERFACE

namespace Mantid
{
namespace MDAlgorithms
{

/// Templated interface to the workspace conversion algorithm. Every template parameter refers to different conversion possibilities
#ifndef EXCLUDE_CONVERT_WORKSPACE_INTERFACE

template<ConvertToMD::InputWSType WS,ConvertToMD::QMode Q, ConvertToMD::AnalMode MODE, ConvertToMD::CnvrtUnits CONV,ConvertToMD::SampleType Sample>
class ConvertToMDEventsWS: public IConvertToMDEventsWS 
{ 
public:
    ConvertToMDEventsWS(){};
    /**templated virtual function to set up conversion*/
    size_t setUPConversion(Mantid::API::MatrixWorkspace_sptr , ConvToMDPreprocDetectors &,const MDEvents::MDWSDescription &, boost::shared_ptr<MDEvents::MDEventWSWrapper> )
    {return 0;}
    /**templated virtual function to run conversion itself*/
    void runConversion(API::Progress *){};
private:
    /**templated virtual function to run conversion chunk */
    virtual size_t conversionChunk(size_t job_ID)
    {return 0;}
};

#else
template<ConvertToMD::InputWSType WS,ConvertToMD::QMode Q, ConvertToMD::AnalMode MODE, ConvertToMD::CnvrtUnits CONV,ConvertToMD::SampleType Sample>
class ConvertToMDEventsWS: public IConvertToMDEventsWS 
{ 
public:
    ConvertToMDEventsWS(){};
    /**templated virtual function to set up conversion*/
    size_t setUPConversion(Mantid::API::MatrixWorkspace_sptr , ConvToMDPreprocDetectors &,const MDEvents::MDWSDescription &, boost::shared_ptr<MDEvents::MDEventWSWrapper> );
    /**templated virtual function to run conversion itself*/
    void runConversion(API::Progress *);
private:
    /**templated virtual function to run conversion chunk */
    virtual size_t conversionChunk(size_t job_ID);
};
#endif

} // end namespace MDAlgorithms
} // end namespace Mantid
#endif

