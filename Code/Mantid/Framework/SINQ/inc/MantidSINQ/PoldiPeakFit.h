#ifndef MANTID_DATAHANDLING_PoldiPeakFit_H_
#define MANTID_DATAHANDLING_PoldiPeakFit_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

#include <vector>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
namespace Poldi
{

class DLLExport PoldiPeakFit : public API::Algorithm
{
public:
	/// Default constructor
	PoldiPeakFit(){};
	/// Destructor
	virtual ~PoldiPeakFit() {}
	/// Algorithm's name for identification overriding a virtual method
	virtual const std::string name() const { return "PoldiPeakFit"; }
	/// Algorithm's version for identification overriding a virtual method
	virtual int version() const { return 4; }
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
//	inline int min(int a, int b){return (a<b)?a:b;}
	inline double max(double a, double b){return (a>b)?a:b;}
//	inline int max(int a, int b){return (a>b)?a:b;}

	double getTableValue(DataObjects::TableWorkspace_sptr tableWS, std::string colname, size_t index);
	double getTableValueFromLabel(DataObjects::TableWorkspace_sptr tableWS, std::string colNameLabel, std::string colNameValue, std::string label);


	DataObjects::Workspace2D_sptr localWorkspace;
	DataObjects::Workspace2D_sptr ws_auto_corr ;

	std::vector<MantidVec*>* poldi_nhe3;
	inline double nhe3(int a, int b);
	double dblSqrt(double in);

	static const double PI    = 3.1415926535897932384626433832795;
	static const double TWOPI = 2*3.1415926535897932384626433832795;
	double rad2deg;
	double deg2rad;

	const static double hbar = 1.0545717253362894e-34;   // J.s
	const static double m_n = 1.674927351e-27;           // kg

//	const double CONVLAMV = 3956.034*1000.;
//	const double CONVKV = CONVLAMV / TWOPI;
//	***     convkv=hquer/(Masse Neutron)
	double CONVKV;               // m²/s == mm²/µs
	double CONVLAMV;       // = 3.956034e-07   unit pb
///TODO solve the unit pb!!!!



	double functn(double xi);
	double fderiv(Mantid::MantidVec& X, std::vector<int> &listfree, int nterms,  int npts);

	double a1;
	double a2;
	double a3;
	double a4;
	double a5;
	double a6;
	std::vector<int> listfree;
	std::vector<std::vector<double>> deriv;


};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_PoldiPeakFit_H_*/
