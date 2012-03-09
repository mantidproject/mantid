#ifndef MANTID_CURVEFITTING_SCDPANELERRORS_H_
#define MANTID_CURVEFITTING_CURVEFITTING_SCDPANELERRORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ParamFunction.h"
/*
 * SCDPanelErrors.h
 *
 *  Created on: Feb 27, 2012
 *      Author: ruth
 */



namespace Mantid
{
namespace CurveFitting
{

  class SCDPanelErrors:  public API::ParamFunction, public API::IFunctionMW
  {
  public:

    SCDPanelErrors();

    SCDPanelErrors( DataObjects::PeaksWorkspace_sptr &pwk, std::string& Component_name ,
        double a, double b, double c, double alpha, double beta,
        double gamma, double tolerance );

    virtual ~SCDPanelErrors();

    std::string name()const  {return "SCDPanelErrors";}

    virtual const std::string category() const { return "Calibrate";}

    void functionMW  (double *out, const double *xValues, const size_t nData)const ;

/*    void  setMatrixWorkspace (boost::shared_ptr< const API::MatrixWorkspace > workspace, size_t wi, double startX=0.0, double endX=0.0)
    {
      API::IFunctionMW::setMatrixWorkspace( workspace,wi,startX,endX);
      setUpCommonData();
    }
    void  setWorkspace (boost::shared_ptr< const Workspace > ws, bool copyData)
    {
      API::IFunctionMW::setWorkspace( ws, copyData);
      setUpCommonData();
    }
*/
    /**
     *   IFitFunctions need to have a workspace set.  This creates one that is compatible
     *   with the underlying calculations. The y values are all 0. The number of x values is
     *   3 times the number of peaks with the given bankName, one for qx,qy and qz for that
     *   peak.  The x value is the index of the corresponding peak
     *
     *   @param  pwks  The peaks workspace
     *   @param  bankNames  the names of the bank( from pwks.getBankName) to calibrate.
     *
     *   @return A workspace to use with this class.
     *   NOTE: pwks must be the same( or clone of) the peaks workspace in the constructor.
     *   NOTE: This Workspace2D must still be set as the workspace for this class.
     */
  static  DataObjects::Workspace2D_sptr calcWorkspace( DataObjects::PeaksWorkspace_sptr pwks,
                                    std::vector<std::string> &bankNames , double tolerance);


  protected:

    void init();

    void setUpCommonData();

    bool CommonDataSetUp;


    DataObjects::PeaksWorkspace_sptr pwks;

    double tolerance;
    Kernel::DblMatrix B0;

    static Kernel::Logger & g_log;

    std::string ComponentName;

  };

}
}

#endif /*MANTID_CURVEFITTING_SCDPANELERRORS_H_*/
