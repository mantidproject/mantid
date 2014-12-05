#ifndef MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_
#define MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** DetectorEfficiencyCorUser :

 This algorithm will calculate the detector efficiency according to the ILL INX program for time-of-flight
 data reduction.

 Formula_eff must be defined in the instrument parameters file.


 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport DetectorEfficiencyCorUser: public API::Algorithm {
public:
	DetectorEfficiencyCorUser();
	virtual ~DetectorEfficiencyCorUser();

	virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "This algorithm calculates the detector efficiency according the formula set in the instrument definition file/parameters.";}

	virtual int version() const;
	virtual const std::string category() const;

private:
	void init();
	void exec();
	void retrieveProperties();
	double calculateFormulaValue(const std::string&,double);
	MantidVec calculateEfficiency(double, const std::string&, const MantidVec&);

	std::string getValFromInstrumentDef(const std::string&);

	void applyDetEfficiency(const size_t numberOfChannels,
			const MantidVec& yIn, const MantidVec& eIn, const MantidVec& effVec,
			MantidVec& yOut, MantidVec& eOut);

	/// The user selected (input) workspace
	API::MatrixWorkspace_const_sptr m_inputWS;
	/// The output workspace, maybe the same as the input one
	API::MatrixWorkspace_sptr m_outputWS;
	/// stores the user selected value for incidient energy of the neutrons
	double m_Ei;
};

} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_ */
