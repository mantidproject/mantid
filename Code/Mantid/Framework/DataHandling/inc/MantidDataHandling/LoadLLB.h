#ifndef MANTID_DATAHANDLING_LOADLLB_H_
#define MANTID_DATAHANDLING_LOADLLB_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IHDFFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadLLB : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport LoadLLB: public API::IHDFFileLoader  {
public:
	LoadLLB();
	virtual ~LoadLLB();

	virtual const std::string name() const;
	virtual int version() const;
	virtual const std::string category() const;

    /// Returns a confidence value that this algorithm can load a file
    virtual int confidence(Kernel::NexusDescriptor & descriptor) const;

private:
	virtual void initDocs();
	void init();
	void exec();
	void setInstrumentName(NeXus::NXEntry& entry);
	void initWorkSpace(NeXus::NXEntry&);
	void loadTimeDetails(NeXus::NXEntry& entry);
	void loadDataIntoTheWorkSpace(NeXus::NXEntry&);
	int getDetectorElasticPeakPosition(const NeXus::NXFloat&);
	std::vector<double> getTimeBinning(int, double);
	/// Calculate error for y
	static double calculateError(double in) {
		return sqrt(in);
	}
	void loadExperimentDetails(NeXus::NXEntry&);
	void loadRunDetails(NeXus::NXEntry &);
	void runLoadInstrument();

	std::vector<std::string> m_supportedInstruments;
	std::string m_instrumentName;
	std::string m_instrumentPath;///< Name of the instrument path

	API::MatrixWorkspace_sptr m_localWorkspace;
	size_t m_numberOfTubes; // number of tubes - X
	size_t m_numberOfPixelsPerTube; //number of pixels per tube - Y
	size_t m_numberOfChannels; // time channels - Z
	size_t m_numberOfHistograms;
	double m_wavelength;
	double m_channelWidth;

	LoadHelper m_loader;

};


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADLLB_H_ */
