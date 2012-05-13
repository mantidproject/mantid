#ifndef MANTID_CURVEFITTING_SCDPANELERRORS_H_
#define MANTID_CURVEFITTING_SCDPANELERRORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace CurveFitting
{

  /**
    This fit function is used for calibrating RectangularDetectors by adjusting L0, time offset, panel width,
    panel height, panel center and orientation(so far). These parameters are adjusted so that the position of
    peaks in q-space match the best with the "theoretical" position.  The theoretical position is determined from the
    known lattice parameters for the sample  and an estimate of U from the UB matrix that best fits the
    given hkl's and current q positions of the peak.

    <UL>The Parameters
       <LI>l0- the initial Flight path in units from Peak.getL1</LI>
      <LI>t0-Time offset in the same units returned with Peak.getTOF);</LI>
      <LI>f*_detWidthScale-panel Width in the same units returned with Peak.getDetPos().norm().</LI>
      <LI>f*_detHeightScale-panel Height in the same units returned with Peak.getDetPos().norm()</LI>
      <LI>f*_Xoffset-Panel Center x offset in the same units returned with Peak.getDetPos().norm()</LI>
      <LI>f*_Yoffset-Panel Center y offset in the same units returned with Peak.getDetPos().norm()</LI>
      <LI>f*_Zoffset-Panel Center z offset in the same units returned with Peak.getDetPos().norm()</LI>
      <LI>f*_Xrot-Rotation(degrees) Panel Center in x axis direction</LI>
      <LI>f*_Yrot-Rotation(degrees) Panel Center in y axis direction</LI>
      <LI>f*_Zrot-Rotation(degrees) Panel Center in z axis direction</LI>

    </UL>
   <UL> Note that the order of rotations are z first, then y then x.</UL>

    <UL> Attributes
      <LI>a,b,c,alpha,beta,gamma- The lattice parameters. The angles are in degrees</LI>
      <LI>PeakWorkspaceName- The name of where the PeaksWorkspace is stored in the AnalysisDataService</LI>
      <LI>BankNames- The comma separated "list" of panel names that this IFitFunction uses. The parameters
                      apply uniformly to every bank. That is all panels will be moved, rotated the same</LI>.
      <LI>startX - -1 is default. If a composite function is used, startX is the index in the xValues( from functionMW)
                   of the starting xvalues that this function changes.</LI>
     <LI>endX-1 is default. If a composite function is used, endX is the index in xValues( from functionMW)
                   of the last xvalues that this function changes. This function only changes xValue between startX and
                   endX inclusive. See workspace information below</LI>.
      <LI> nGroups  The number of groups( determines and creates parameters
                      f*_xxxx. where * is 0,1,2,3, etc.)</LI>
     </UL>

     <UL> The workspace should be a Workspace2D where only one histogram is used.<P>
          A second or third histogram may have to be around to get things to work.<P>
          Each peak from the PeaksWorkspace that is used by this function, will have 3 consecutive
          x values from the xData( these correspond to the xValues's from functionMW).  The xvalues for all three
          will be the index into the PeaksWorkspace. Their yvalues will be 0. The order is not important except when
          using this as a part of a Composite fucntion, the x value indicies associatied with this function( peaks associated
          with this function) are consecutive.  The first of the 3 x values associated with one peak correspond to the components
          wrt xyz of the errors in the q values for this peak at the given parameters.

     </UL>
    @author Ruth Mikkelson, SNS ORNL
    @date 1/27/2012


    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  class DLLExport SCDPanelErrors :  public API::ParamFunction, public API::IFunction1D
  {
  public:

    SCDPanelErrors();

    /**
     * Constructor
     * @param pwk       - The PeaksWorkspace
     * @param BankNames - The comma separated list of bank names for which this Function
     *                     this Function calculates the associated errors in qx,qy,qz
     * @param a         - The a lattice parameter
     * @param b         - The b lattice parameter
     * @param c         - The c lattice parameter
     * @param alpha     - The alpha lattice parameter in degrees
     * @param beta      - The beta lattice parameter in degrees
     * @param gamma     - The gamma lattice parameter in degrees
     * @param tolerance - The maximum distance a peak's h value, k value and lvalue are
     *                    from an integer to be considered indexed. Outside of this
     *                    constructor, ALL PEAKS are considered INDEXED.
     *
     *
     */
    SCDPanelErrors( DataObjects::PeaksWorkspace_sptr &pwk, std::string& BankNames ,
        double a, double b, double c, double alpha, double beta,
        double gamma, double tolerance );

    virtual ~SCDPanelErrors();

    std::string name()const  {return "SCDPanelErrors";}

    virtual const std::string category() const { return "Calibrate";}

    void function1D  (double *out, const double *xValues, const size_t nData)const ;

   void functionDeriv1D (API::Jacobian* out, const double *xValues, const size_t nData);

   /**
    * Copies some of the information from pmapSv to pmap
    * @param bank_const-The component that pmap will be associated with.  These maps
    *                   use component names, so this just give names.
    * @param pmap    - The new map where some of the entries of pmapSv are transferred
    * @param pmapSv  - The original ParameterMap
    */
   void updateBankParams( boost::shared_ptr<const Geometry::IComponent>  bank_const,
                 boost::shared_ptr<Geometry::ParameterMap> pmap,
                 boost::shared_ptr<const Geometry::ParameterMap>pmapSv)const;

   /**
     * Copies some of the information from pmapSv to pmap
     * @param bank_const-The component that pmap will be associated with.  These maps
     *                   use component names, so this just give names.
     * @param pmap    - The new map where some of the entries of pmapSv are transferred
     * @param pmapSv  - The original ParameterMap
     */
   void updateSourceParams(boost::shared_ptr<const Geometry::IObjComponent> bank_const,
       boost::shared_ptr<Geometry::ParameterMap> pmap, boost::shared_ptr<const Geometry::ParameterMap> pmapSv) const;

   /**
    * Given the derivative of Qrot wrt to some parameter, this calculates the final derivative
    * for the Error in Qrot, by estimating the theoretical dQrot.
    * @param DerivQ -a 3xnpeaks matrix of derivatives of Qrot wrt to a parameter
    *
    * @param Mhkl  -a npeaksx3 matrix of the hkl values
    *
    * @param MhklT -The 3xnpeaks matrix that is the Transpoxe of Mhkl
    *
    * @param InvhklThkl - THE 3x3 matrix that is = inverse ( MhklT*Mhkl)
    *
    * @param UB          -THE 3x3 matrix that best maps the hkl values to their associated rot q values
    *                     divide bty 2PI.
    *
    * @return The derivative of the Error in Qrot
    */
   Kernel::Matrix<double> CalcDiffDerivFromdQ(  Kernel::Matrix<double>const &  DerivQ,
                                                Kernel::Matrix<double>const &  Mhkl,
                                                Kernel::Matrix<double>const &  MhklT,
                                                Kernel::Matrix<double>const &  InvhklThkl,
                                                Kernel::Matrix<double>const &  UB )const;



   size_t   nAttributes () const
    {
      return (size_t)11;
    }


    std::vector< std::string >  getAttributeNames () const
    {
      std::vector< std::string > V;
      V.push_back("a");
      V.push_back("b");
      V.push_back("c");
      V.push_back("alpha");
      V.push_back("beta");
      V.push_back("gamma");
      V.push_back("PeakWorkspaceName");
      V.push_back("BankNames");
      V.push_back("startX");
      V.push_back("endX");
      V.push_back("NGroups");
      return V;
    }

   Attribute getAttribute(const std::string &attName) const
      {
        if (!hasAttribute(attName))
          throw std::invalid_argument("Not a valid attribute nameb");

        if (attName.compare("beta") < 0)
        {
          if (attName.compare("a") == 0)
            return Attribute(a);

          else if (attName == ("b"))
            return Attribute(b);

          else if (attName == ("BankNames"))
            return Attribute(BankNames);

          else if (attName == ("alpha"))
            return Attribute(alpha);

          else if( attName == "PeakWorkspaceName")
            {
              return Attribute( PeakName );
            }

          else if( attName =="NGroups")
          {
             Attribute A(NGroups);

            return A;
          }

          else
            throw std::invalid_argument("Not a valid attribute namec");
        }
        else if (attName == "beta")
          return Attribute(beta);

        else if (attName == "c")
          return Attribute(c);

        else if (attName == "gamma")
          return Attribute(gamma);

        else if (attName == "startX")
          return Attribute(startX);

        else if (attName == "endX")
          return Attribute(endX);


        throw std::invalid_argument("Not a valid attribute named");
      }


    void setAttribute(const std::string &attName, const Attribute & value);

    bool hasAttribute(const std::string &attName) const
      {
        if (attName.compare("beta") < 0)
        {
          if (attName.compare("a") == 0)
            return true;

          else if (attName == ("b"))
            return true;

          else if (attName == ("alpha"))
            return true;

          else if (attName == ("BankNames"))
            return true;

          else if( attName =="PeakWorkspaceName")
                    return true;

          else if( attName == "NGroups")
                   return true;
          return false;

        }
        else if (attName == "beta")
          return true;

        else if (attName == "c")
          return true;

        else if (attName == "gamma")
          return true;

        else if (attName == "startX")
          return true;

        else if (attName == "endX")
          return true;



        return false;
      }

/**
 * A utility method that will set up the Workspace needed by this function.
 * @param pwks       -The peaksWorkspace.  All peaks indexed to the given tolerance and whose
 *                    associated bankName matches on of the strings in bankNames will be included.
 * @param bankNames  -A list of bankNames. See pwks
 * @param tolerance  -A measure of the maxiumum distance a peak's h value,k value, or l value is
 *                    from an integer to be considered indexed.
 * @return   The associated workspace
 *
 * NOTE: This method could be used if this FitFunction is part of a composite function, but an Xstart
 *        and Xend for each composite is needed and may be difficult to determine.
 */
  static  DataObjects::Workspace2D_sptr calcWorkspace( DataObjects::PeaksWorkspace_sptr & pwks,
                                    std::vector<std::string> &bankNames , double tolerance);


  protected:

    void init();

    boost::shared_ptr<DataObjects::PeaksWorkspace> getPeaks()const;

    /**
     *  Checks for out of bounds values ,  peaksWorkspace status
     */
    void Check( DataObjects::PeaksWorkspace_sptr &pkwsp, const double *xValues, const size_t nData)const;

    /**
     * Gets the new instrument by applying parameters values to the old instrument
     * @param peak  A peak.  Only used to get an old instrument from the 1st peak.
     *
     * @return A new instrument with the parameters applied.
     */
    Geometry::Instrument_sptr getNewInstrument( const DataObjects::Peak & peak)const;

    /**
     * Creates a new peak, matching the old peak except for a different instrument. The Time offset
     * parameter is also applied( not part of the instrument)
     * @param peak_old - The old peak
     * @param instrNew -The new instrument
     * @return The new peak with the new instrument( adjusted with the parameters) and time adjusted.
     */
    DataObjects::Peak  createNewPeak( const DataObjects::Peak & peak_old, Geometry::Instrument_sptr  instrNew)const;

    /**
     * Even though constrains are used. Often very illogical parameters have to be processed.
     * This checks for these conditions.
     */
    double checkForNonsenseParameters() const;
    boost::shared_ptr< DataObjects::PeaksWorkspace> peaks;

    double a,b,c,alpha,beta,gamma;
    int NGroups;

    std::string PeakName;//< SCDPanelErrors{PeakName} is name in the Analysis Data Service where the PeaksWorkspace is stored

    bool a_set,b_set,c_set,alpha_set,beta_set,gamma_set,PeakName_set, BankNames_set,
        startX_set,endX_set, NGroups_set;

    int NLatticeParametersSet;


    double tolerance;

    Kernel::DblMatrix B0;
    std::string BankNames;

    int startX, endX; //start and end indicies in xValues array in functionMW. -1 use all.
    static Kernel::Logger & g_log;



  };

}
}

#endif /*MANTID_CURVEFITTING_SCDPANELERRORS_H_*/
