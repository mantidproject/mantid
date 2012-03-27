
#ifndef SCDCALIBRATEPANELS_H_
#define SCDCALIBRATEPANELS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Quat.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"

using Mantid::Kernel::Quat;
using Mantid::Geometry::IComponent;

namespace Mantid
{
namespace Crystal
{

  class SCDCalibratePanels: public Mantid::API::Algorithm
  {
  public:

    SCDCalibratePanels();

    virtual ~SCDCalibratePanels();

    virtual const std::string name() const
    {
       return "SCDCalibratePanels";
    }

     /// Algorithm's version for identification overriding a virtual method
     virtual int version() const
    {
        return 1;
    }

     /// Algorithm's category for identification overriding a virtual method
     virtual const std::string category() const
     {
       return "Crystal";
     }

  /**
   *  Refactors a rotation Q as a Rotation in x dir by Rotx * a Rotation in the y dir by Roty
   *                                     * a rotation in the z direction by Rotz
   *  @param Quat  A rotation( a copy will be normalized)
   *  @ Rotx       The angle in degrees for the rotation in x direction
   *  @ Roty       The angle in degrees for the rotation in y direction
   *  @ Rotxz      The angle in degrees for the rotation in z direction
   */
  void Quat2RotxRotyRotz(const Quat Q, double &Rotx,double &Roty,double &Rotz);

  /**
   * Given a string representation of a set of groups( [] separated list of bank nums separated by
   * commas or colons( for ranges) , this method will produce a vector of "groups"( vector of bank names).
   * All bank names will be members of AllBankNames and only used one time.
   * @param AllBankNames  -The list of all banks to use
   * @param Grouping      -Grouping mode (  OnePanelPerGroup, AllPanelsInOneGroup, or SpecifyGroups)
   * @param bankPrefix    -For SpecifyGroups,the prefix to be affixed to each integer in the bankingCode
   * @param bankingCode   -A [] separated list of banknums. Between the [..], the bank nums can be separted by
   *                       commas or : for lists.
   * @param Groups       -Contains the result, a vector of vectors of bank names.
   *
   */
  void CalculateGroups(std::set<std::string> &AllBankNames,
                       std::string Grouping,
                       std::string bankPrefix,
                       std::string bankingCode,
                       std::vector<std::vector<std::string> > &Groups);

  /**
   * Calculate the Matrix workspace associated with a Peaksworkspace for Composite functions.
   * the elements of bounds can be used to calculate Xstart and X end
   * @param pwks        The PeaksWorkspace
   * @param  bankNames  The list of bank names( from Peak.getBankName())
   * @param tolerance   The maximum distance the h value, k value, and l value of a Peak is from
   *                    an integer, for a peak to be considered Indexed.
   * @param  bounds     bounds[i] is the starting index for the xvalues from the resultant workspace.
   *                    This can be used to determine startX and endX.
   */
  DataObjects::Workspace2D_sptr calcWorkspace( DataObjects::PeaksWorkspace_sptr & pwks,
                                               std::vector< std::string>& bankNames,
                                               double tolerance,
                                               std::vector<int>&bounds);


  /**
   * Copies some of the information from pmapSv to pmap
   * @param pmapSv  - The original ParameterMap
   * @param pmap    - The new map where some of the entries of pmapSv are transferred
   * @param component-The component that pmap will be associated with.  These maps
   *                   use component names, so this just give names.
   */
  void updateParams(  boost::shared_ptr<const Geometry::ParameterMap>    pmapSv,
                      boost::shared_ptr<Geometry::ParameterMap>   pmap,
                      boost::shared_ptr<const Geometry::IComponent>  component);

  private:
    void exec ();

    void  init ();

    void initDocs ();

    static Kernel::Logger & g_log;

  };

}//namespace Crystal
}//namespace Mantid

#endif /* SCDCALIBRATEPANELS_H_ */
