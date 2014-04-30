/*
 * Helper file to gather common routines to the Loaders
 * */

#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

LoadHelper::LoadHelper() :
		g_log(Kernel::Logger::get("Algorithm")) {
}

LoadHelper::~LoadHelper() {
}


/**
 * Finds the path for the instrument name in the nexus file
 * Usually of the form: entry0/\<NXinstrument class\>/name
 */
std::string LoadHelper::findInstrumentNexusPath(
		const NeXus::NXEntry &firstEntry) {
	std::string insNamePath = "";
	std::vector<NeXus::NXClassInfo> v = firstEntry.groups();
	for (auto it = v.begin(); it < v.end(); it++) {
		if (it->nxclass == "NXinstrument") {
			insNamePath = it->nxname;
			break;
		}
	}
	return insNamePath;
}

std::string LoadHelper::getStringFromNexusPath(const NeXus::NXEntry &firstEntry,
		const std::string &nexusPath) {
	return firstEntry.getString(nexusPath);
}

double LoadHelper::getDoubleFromNexusPath(const NeXus::NXEntry &firstEntry,
		const std::string &nexusPath) {
	return firstEntry.getFloat(nexusPath);
}

/**
 * Gets the time binning from a Nexus float array
 * Adds an extra bin at the end
 */
std::vector<double> LoadHelper::getTimeBinningFromNexusPath(
		const NeXus::NXEntry &firstEntry, const std::string &nexusPath) {

	NeXus::NXFloat timeBinningNexus = firstEntry.openNXFloat(nexusPath);
	timeBinningNexus.load();

	size_t numberOfBins = static_cast<size_t>(timeBinningNexus.dim0()) + 1; // boundaries

	float* timeBinning_p = &timeBinningNexus[0];
	std::vector<double> timeBinning(numberOfBins);
	timeBinning.assign(timeBinning_p, timeBinning_p + numberOfBins);
	// calculate the extra bin at the end
	timeBinning[numberOfBins - 1] = timeBinning[numberOfBins - 2]
			+ timeBinning[1] - timeBinning[0];

	return timeBinning;
}
/**
 * Calculate Neutron Energy from wavelength: \f$ E = h^2 / 2m\lambda ^2 \f$
 *  @param wavelength :: wavelength in \f$ \AA \f$
 *  @return tof in seconds
 */
double LoadHelper::calculateEnergy(double wavelength) {
	double e = (PhysicalConstants::h * PhysicalConstants::h)
			/ (2 * PhysicalConstants::NeutronMass * wavelength * wavelength
					* 1e-20) / PhysicalConstants::meV;
	return e;
}

/**
 * Calculate TOF from distance
 *  @param distance :: distance in meters
 *  @param wavelength :: wavelength to calculate TOF from
 *  @return tof in seconds
 */
double LoadHelper::calculateTOF(double distance,double wavelength) {
	if (wavelength <= 0) {
		g_log.error("Wavelenght is <= 0");
		throw std::runtime_error("Wavelenght is <= 0");
	}

	double velocity = PhysicalConstants::h
			/ (PhysicalConstants::NeutronMass * wavelength * 1e-10); //m/s

	return distance / velocity;
}

double LoadHelper::getL1(const API::MatrixWorkspace_sptr& workspace) {
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	Geometry::IObjComponent_const_sptr sample = instrument->getSample();
	double l1 = instrument->getSource()->getDistance(*sample);
	return l1;
}

double LoadHelper::getL2(const API::MatrixWorkspace_sptr& workspace, int detId) {
	// Get a pointer to the instrument contained in the workspace
	Geometry::Instrument_const_sptr instrument =
			workspace->getInstrument();
	// Get the distance between the source and the sample (assume in metres)
	Geometry::IObjComponent_const_sptr sample = instrument->getSample();
	// Get the sample-detector distance for this detector (in metres)
	double l2 = workspace->getDetector(detId)->getPos().distance(
			sample->getPos());
	return l2;
}

/*
 * Get instrument property as double
 * @s - input property name
 *
 */
double LoadHelper::getInstrumentProperty(const API::MatrixWorkspace_sptr& workspace, std::string s) {
	std::vector<std::string> prop =
			workspace->getInstrument()->getStringParameter(s);
	if (prop.empty()) {
		g_log.debug("Property <" + s + "> doesn't exist!");
		return EMPTY_DBL();
	} else {
		g_log.debug() << "Property <" + s + "> = " << prop[0] << std::endl;
		return boost::lexical_cast<double>(prop[0]);
	}
}


/**
   * Recursively add properties from a nexus file to
   * the workspace run.
   *
   * @param nxfileID    :: The Nexus file to be parsed
   * @param runDetails  :: where to add properties
   * @param parent_name :: nexus caller name
   * @param parent_class :: nexus caller class
   * @param level       :: current level in nexus tree
   *
   */
void LoadHelper::AddNexusFieldsToWsRun(NXhandle nxfileID,
					API::Run& runDetails,
					std::string& parent_name,
					std::string& parent_class,
		    		int level) {

	std::string indent_str(level*2, ' ');// Two space by indent level

	// Link ?

	// Attributes ?
	//dump_attributes(nxfileID, indent_str);

	// Classes
	NXstatus stat;       ///< return status
	int datatype;        ///< NX data type if a dataset, e.g. NX_CHAR, NX_FLOAT32, see napi.h
	char nxname[NX_MAXNAMELEN],nxclass[NX_MAXNAMELEN];

	while(NXgetnextentry(nxfileID,nxname,nxclass,&datatype) != NX_EOD)
	{
		g_log.debug()<<indent_str<<parent_name<<"."<<nxname<<" ; "<<nxclass<<std::endl;

		if((stat=NXopengroup(nxfileID,nxname,nxclass))==NX_OK){

			// Go down to one level
			std::string p_nxname(nxname);//current names can be useful for next level
			std::string p_nxclass(nxclass);

			AddNexusFieldsToWsRun(nxfileID, runDetails, p_nxname, p_nxclass, level+1);

			NXclosegroup(nxfileID);
		}// if(NXopengroup
		else if ((stat=NXopendata (nxfileID, nxname))==NX_OK)
		{
			//dump_attributes(nxfileID, indent_str);
			g_log.debug()<<indent_str<<nxname<<" opened."<<std::endl;

			if (parent_class=="NXData") {
				g_log.debug()<<indent_str<<"skipping NXData"<<std::endl;
				/* nothing */
			} else if (parent_class=="NXMonitor") {
				g_log.debug()<<indent_str<<"skipping NXMonitor"<<std::endl;
				/* nothing */
			} else { // create a property
				int rank;
				int dims[4];
				int type;

				std::string property_name;
				// Exclude "entry0" from name for level 1 property
				if (parent_name == "entry0")
					property_name = nxname;
				else
					property_name = parent_name+"."+nxname;

				g_log.debug()<<indent_str<<"considering property "<<property_name<<std::endl;

				// Get the value
				NXgetinfo(nxfileID, &rank, dims, &type);

				// Note, we choose to ignore "multidim properties
				if (rank!=1) {
					g_log.debug()<<indent_str<<"ignored multi dimension data on "<<property_name<<std::endl;
				} else {
					void *dataBuffer;
					NXmalloc (&dataBuffer, rank, dims, type);
					if (NXgetdata(nxfileID, dataBuffer) != NX_OK) {
						NXfree(&dataBuffer);
						throw std::runtime_error("Cannot read data from NeXus file");
					}

					if (type==NX_CHAR) {
						runDetails.addProperty(property_name, std::string((const char *)dataBuffer));
					} else if ((type==NX_FLOAT32)
								||(type==NX_FLOAT64)
								||(type==NX_INT16)
								||(type==NX_INT32)
								||(type==NX_UINT16)
								) {

						// Look for "units"
						NXstatus units_status;
						char units_sbuf[NX_MAXNAMELEN];
						int units_len=NX_MAXNAMELEN;
						int units_type=NX_CHAR;

						units_status=NXgetattr(nxfileID,const_cast<char*>("units"),(void *)units_sbuf,&units_len,&units_type);
						if(units_status!=NX_ERROR)
						{
							g_log.debug()<<indent_str<<"[ "<<property_name<<" has unit "<<units_sbuf<<" ]"<<std::endl;
						}


						if ((type==NX_FLOAT32)||(type==NX_FLOAT64)) {
							// Mantid numerical properties are double only.
							double property_double_value=0.0;
							if (type==NX_FLOAT32) {
								property_double_value = *((float*)dataBuffer);
							} else if (type==NX_FLOAT64) {
								property_double_value = *((double*)dataBuffer);
							}

							if(units_status!=NX_ERROR)
								runDetails.addProperty(property_name, property_double_value, std::string(units_sbuf));
							else
								runDetails.addProperty(property_name, property_double_value);

						} else {
							// int case
							int property_int_value=0;
							if (type==NX_INT16) {
								property_int_value = *((short int*)dataBuffer);
							} else if (type==NX_INT32) {
								property_int_value = *((int*)dataBuffer);
							}else if (type==NX_UINT16) {
								property_int_value = *((short unsigned int*)dataBuffer);
							}

							if(units_status!=NX_ERROR)
								runDetails.addProperty(property_name, property_int_value, std::string(units_sbuf));
							else
								runDetails.addProperty(property_name, property_int_value);

						}// if (type==...



					} else {
						g_log.debug()<<indent_str<<"unexpected data on "<<property_name<<std::endl;
					}

					NXfree(&dataBuffer);
				}
			}

			NXclosedata(nxfileID);
		} else {
			g_log.debug()<<indent_str<<"unexpected status ("<<stat<<") on "<<nxname<<std::endl;
		}

	}// while NXgetnextentry


}// RecurseForProperties


/**
 * Parses the date as formatted at the ILL:
 * 29-Jun-12 11:27:26
 * and converts it to the ISO format used in Mantid:
 * ISO8601 format string: "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"
 *
 *  @param dateToParse :: date as string
 *  @return date as required in Mantid
 */
std::string LoadHelper::dateTimeInIsoFormat(std::string dateToParse) {
	namespace bt = boost::posix_time;
	// parsing format
	const std::locale format = std::locale(std::locale::classic(),
			new bt::time_input_facet("%d-%b-%y %H:%M:%S"));

	bt::ptime pt;
	std::istringstream is(dateToParse);
	is.imbue(format);
	is >> pt;

	if (pt != bt::ptime()) {
		// Converts to ISO
		std::string s = bt::to_iso_extended_string(pt);
		return s;
	} else {
		return "";
	}
}

} // namespace DataHandling
} // namespace Mantid
