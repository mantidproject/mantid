#ifndef MUONNEXUSREADER_H
#define MUONNEXUSREADER_H

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"


//class MuonNexusReader - based on ISISRAW this class implements a simple
// reader for Nexus Muon data.
class MuonNexusReader
{
	private:


	public:
		MuonNexusReader();
		~MuonNexusReader();
		int readFromFile(const std::string& filename);
		int getTimeChannels(float* timechannels, int len);
		// from ISISRAW.h
		int t_nsp1;			///< number of spectra in time regime 1
		int t_ntc1;			///< number of time channels in time regime 1
		int t_nper;          ///< number of periods in file (=1 at present)
		// for nexus
		float* corrected_times; ///< temp store for corrected times
		int* counts;         ///< temp store of histogram data
};


#endif /* MUONNEXUSREADER_H */













