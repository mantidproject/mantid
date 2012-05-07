
#ifndef SCDCALIBRATEPANELS_H_
#define SCDCALIBRATEPANELS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Quat.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/lexical_cast.hpp>

using namespace Mantid::Kernel;
using namespace  Mantid::Geometry;

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
   * Calculate the Workspace2D associated with a Peaksworkspace for Composite functions.
   * the elements of parameter bounds can be used to calculate Xstart and Xend
   *
   * @param pwks        The PeaksWorkspace
   *
   * @param  bankNames  The list of bank names( from Peak.getBankName())
   *
   * @param tolerance   The maximum distance the h value, k value, and l value of a Peak is from
   *                    an integer, for a peak to be considered Indexed.
   *
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

  //const std::string OnePanelPerGroup;//("OnePanelPerGroup");
 // const std::string AllPanelsInOneGroup;//("AllPanelsInOneGroup");
  //const std::string SpecifyGroups;//("SpecifyGroups");

  private:
    void exec ();

    void  init ();

    void initDocs ();

    static Kernel::Logger & g_log;


    /**
     * Creates a new instrument when a calibration file( .xml or .detcal)
     * is loaded
     *
     * @param instrument   The old instrument
     * @param preprocessCommand  either "No PreProcessing",
     *                                  "Apply a ISAW.DetCal File",or
     *                                  "Apply a LoadParameter.xml type file"
     * @param preprocessFilename  Filename is one of the preprocessCommand
     *                            indicates use of  a file
     * @param timeOffset  The timeoffset to use
     * @param L0          The initial flight path
     * @param AllBankNames  The names of all the banks that wiil be processed.
     */
    boost::shared_ptr<const Instrument> GetNewCalibInstrument(
                              boost::shared_ptr<const Instrument>   instrument,
                              std::string preprocessCommand,
                              std::string preprocessFilename,
                              double &timeOffset, double &L0,
                              std::vector<std::string>  & AllBankNames);

    /**
     * Calculates the initial values for all the parameters.  This is needed if
     * when preprocessing is done( load in a calibration file before starting)
     *
     * @param  bank_rect   a bank in the instrument
     * @param instrument   The old instrument
     * @param PreCalibinstrument  the precalibrated instrument
     * @param detWidthScale0  The initial scaling on the panel width
     * @param detHeightScale0  The initial scaling on the panel height
     * @param Xoffset0         The initial X offset of the center of the panel
     * @param Yoffset0         The initial Y offset of the center of the panel
     * @param Zoffset0         The initial Z offset of the center of the panel
     * @param Xrot0            The initial relative rotation about the  x-axis
     *                                  around the center of the panel
     * @param Yrot0            The initial relative rotation about the  y-axis
     *                                  around the center of the panel
     * @param Zrot0            The initial relative rotation about the  z-axis
     *                                  around the center of the panel
     *
     */
    void CalcInitParams(  RectangularDetector_const_sptr bank_rect,
                            Instrument_const_sptr instrument,
                            Instrument_const_sptr  PreCalibinstrument,
                            double & detWidthScale0,double &detWidthHeight0,
                            double &Xoffset0,double &Yoffset0,double &Zoffset0,
                            double &Xrot0,double &Yrot0,double &Zrot0);

    /**
     *  Copies positional entries in pmapSv to pmap starting at bank_const
     *  and parents.
     *  @param  bank_const  the starting component for copying entries.
     *  @param pmap         the Parameter Map to be updated
     *  @param pmapSv       the original Parameter Map
     *
     */
    void updateBankParams( boost::shared_ptr<const Geometry::IComponent>  bank_const,
                  boost::shared_ptr<Geometry::ParameterMap> pmap,
                  boost::shared_ptr<const Geometry::ParameterMap>pmapSv) const;


    /**
     *  Copies positional entries in pmapSv to pmap starting at bank_const
     *  and parents.
     *  @param  bank_const  the starting component for copying entries.
     *  @param pmap         the Parameter Map to be updated
     *  @param pmapSv       the original Parameter Map
     *
     */
    void updateSourceParams(boost::shared_ptr<const Geometry::IObjComponent> bank_const,
         boost::shared_ptr<Geometry::ParameterMap> pmap,
         boost::shared_ptr<const Geometry::ParameterMap> pmapSv) const;

  };

}//namespace Crystal
}//namespace Mantid

#endif /* SCDCALIBRATEPANELS_H_ */
