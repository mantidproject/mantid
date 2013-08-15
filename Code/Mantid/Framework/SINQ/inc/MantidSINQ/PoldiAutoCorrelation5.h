#ifndef MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_
#define MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidKernel/PhysicalConstants.h"



namespace Mantid
{
namespace Poldi
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------



const double rad2deg = 180./M_PI;
const double deg2rad = M_PI/180.;



// N.B. PoldiAutoCorrelation5 is used to create the autocorrelation
// function from POLDI raw data
/** @class PoldiAutoCorrelation5 PoldiAutoCorrelation5.h Poldi/PoldiAutoCorrelation5.h

    Part of the Poldi scripts set, used to analyse Poldi data

    @author Christophe Le Bourlot, Paul Scherrer Institut - SINQ
    @date 05/06/2013

    Copyright © 2013 PSI-MSS

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
    Code Documentation is available at <http://doxygen.mantidproject.org>
*/

class DLLExport PoldiAutoCorrelation5 : public API::Algorithm
{
public:
	/// Default constructor
	PoldiAutoCorrelation5(){};
	/// Destructor
	virtual ~PoldiAutoCorrelation5() {}
	/// Algorithm's name for identification overriding a virtual method
	virtual const std::string name() const { return "PoldiAutoCorrelation"; }
	/// Algorithm's version for identification overriding a virtual method
	virtual int version() const { return 5; }
	/// Algorithm's category for identification overriding a virtual method
	virtual const std::string category() const { return "Poldi\\PoldiSet"; }



protected:
	/// Overwrites Algorithm method
	void exec();





private:
	/// Sets documentation strings for this algorithm
	virtual void initDocs();
	/// Overwrites Algorithm method.
	void init();

	inline double min(double a, double b){return (a<b)?a:b;}
	inline double max(double a, double b){return (a>b)?a:b;}

	double getTableValue(DataObjects::TableWorkspace_sptr tableWS, std::string colname, size_t index);
	double getTableValueFromLabel(DataObjects::TableWorkspace_sptr tableWS, std::string colNameLabel, std::string colNameValue, std::string label);


	DataObjects::Workspace2D_sptr localWorkspace;

	std::vector<MantidVec*>* poldi_nhe3;
	inline double nhe3(int a, int b);
	double dblSqrt(double in);


//	static const double hbar = 1.0545717253362894e-34;   // J.s
//	static const double m_n = 1.674927351e-27;           // kg

//	***     convkv=hquer/(Masse Neutron)
    double CONVKV;
	double CONVLAMV; 

};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_*/
