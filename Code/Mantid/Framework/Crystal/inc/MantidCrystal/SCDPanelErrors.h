#ifndef MANTID_CRYSTAL_SCDPANELERRORS_H_
#define MANTID_CRYSTAL_SCDPANELERRORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
//#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace Crystal
{


   /**
   * @author Ruth Mikkelson, SNS ORNL
   * @date 1/27/2012
   *
   *
   * Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
   *
   * This file is part of Mantid.
   *
   * Mantid is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License as published by
   * the Free Software Foundation; either version 3 of the License, or
   * (at your option) any later version.
   *
   * Mantid is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
   *
   * File change history is stored at: <https://github.com/mantidproject/mantid>
   * Code Documentation is available at: <http://doxygen.mantidproject.org>
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


   Kernel::Matrix<double> CalcDiffDerivFromdQ(  Kernel::Matrix<double>const &  DerivQ,
                                                Kernel::Matrix<double>const &  Mhkl,
                                                Kernel::Matrix<double>const &  MhklT,
                                                Kernel::Matrix<double>const &  InvhklThkl,
                                                Kernel::Matrix<double>const &  UB )const;



   size_t   nAttributes () const
    {
      return (size_t)13;
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
      V.push_back("RotateCenters");
      V.push_back("SampleOffsets");
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
          } else if( attName == "RotateCenters")
          {
            if(RotateCenters)
              return Attribute(1);
            else
              return Attribute(0);
          }
          else if( attName== "SampleOffsets")
          {
            if( SampleOffsets)

              return Attribute(1);
            else
             return Attribute(0);
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

          else if( attName == "RotateCenters")
            return true;

          else if( attName== "SampleOffsets")
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

  /**
     * Creates a new peak, matching the old peak except for a different instrument.
     *
     * The Time of flightis the same except offset by T0. L0 should be the L0 for the new instrument.
     * It is added as a parameter in case the instrument will have the initial flight path adjusted later.
     *  NOTE: the wavelength is changed.
     *
     * @param peak_old - The old peak
     * @param instrNew -The new instrument
     * @param T0 :
     * @param L0 :
     * @return The new peak with the new instrument( adjusted with the parameters) and time adjusted.
     */
    static DataObjects::Peak  createNewPeak( const API::IPeak & peak_old, Geometry::Instrument_sptr  instrNew,
        double T0, double L0);

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
    Geometry::Instrument_sptr getNewInstrument( const API::IPeak & peak)const;


    /**
     * Even though constrains are used. Often very illogical parameters have to be processed.
     * This checks for these conditions.
     */
    double checkForNonsenseParameters() const;

    boost::shared_ptr< DataObjects::PeaksWorkspace> peaks;

    double a,b,c,alpha,beta,gamma;
    int NGroups;
    bool RotateCenters, SampleOffsets;

    std::string PeakName;//< SCDPanelErrors{PeakName} is name in the Analysis Data Service where the PeaksWorkspace is stored

    bool a_set,b_set,c_set,alpha_set,beta_set,gamma_set,PeakName_set, BankNames_set,
        startX_set,endX_set, NGroups_set;
    int SampOffsetDeclareStatus;//0 no action, 1- init called no declare
                                //2- init Called and samp offset parameters are declared

    int NLatticeParametersSet;


    double tolerance;

    Kernel::DblMatrix B0;
    std::string BankNames;

    int startX, endX; //start and end indicies in xValues array in functionMW. -1 use all.
    static Kernel::Logger & g_log;



  };

}
}

#endif /*MANTID_CRYSTAL_SCDPANELERRORS_H_*/
