#ifndef MANTID_DATAHANDLING_LOADPSI_H_
#define MANTID_DATAHANDLING_LOADPSI_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadPSI : TODO: DESCRIPTION

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
class DLLExport LoadPSI: public API::Algorithm {
public:
	LoadPSI();
	virtual ~LoadPSI();

	virtual const std::string name() const;
	virtual int version() const;
	virtual const std::string category() const;

private:
	virtual void initDocs();
	void init();
	void exec();
	NeXus::NXEntry openNexusFile();
	void setInstrumentName(NeXus::NXEntry& entry);
	void initWorkSpace(NeXus::NXEntry& entry);

	std::string m_instrumentName;
	API::MatrixWorkspace_sptr m_localWorkspace;

	size_t m_numberOfTubes; // number of tubes - X
	size_t m_numberOfPixelsPerTube; //number of pixels per tube - Y
	size_t m_numberOfChannels; // time channels - Z
	size_t m_numberOfHistograms;

};

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADPSI_H_ */
