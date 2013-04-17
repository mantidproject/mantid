/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetectorEfficiencyCorUser)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DetectorEfficiencyCorUser::DetectorEfficiencyCorUser() {
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DetectorEfficiencyCorUser::~DetectorEfficiencyCorUser() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DetectorEfficiencyCorUser::name() const {
	return "DetectorEfficiencyCorUser";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int DetectorEfficiencyCorUser::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string DetectorEfficiencyCorUser::category() const {
	return "CorrectionFunctions\\EfficiencyCorrections;Inelastic";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void DetectorEfficiencyCorUser::initDocs() {
	this->setWikiSummary(
			"This algorithm calculates the detector efficiency based on the user formulas set in the IDF file or parameters.");
	this->setOptionalMessage(
			"This algorithm calculates the detector efficiency based on the user formulas set in the IDF file or parameters.");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DetectorEfficiencyCorUser::init() {
	auto val = boost::make_shared<CompositeValidator>();
	val->add<WorkspaceUnitValidator>("Energy");
	val->add<HistogramValidator>();
	val->add<InstrumentValidator>();
	declareProperty(
			new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
					val), "The workspace to correct for detector efficiency");
	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"The name of the workspace in which to store the result.");
	auto checkEi = boost::make_shared<BoundedValidator<double> >();
	checkEi->setLower(0.0);
	declareProperty("IncidentEnergy", EMPTY_DBL(), checkEi,
			"The energy of neutrons leaving the source.");
}


//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DetectorEfficiencyCorUser::exec() {

	// get input properties
	retrieveProperties();

	// parse parameter.xml file
	double eff0 = calculateEff0();

	// get eff formula
	std::string effFormula = getValFromInstrumentDef("formula_eff");

	// correct ws input
	const size_t numberOfChannels = this->m_inputWS->blocksize();
	// Calculate the number of spectra in this workspace
	const int numberOfSpectra = static_cast<int>(this->m_inputWS->size()
			/ numberOfChannels);
	API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
	int64_t numberOfSpectra_i = static_cast<int64_t>(numberOfSpectra); // cast to make openmp happy

	// Loop over the histograms (detector spectra)
	PARALLEL_FOR2(m_inputWS,m_outputWS)
	for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
		PARALLEL_START_INTERUPT_REGION

		//MantidVec& xOut = m_outputWS->dataX(i);
		MantidVec& yOut = m_outputWS->dataY(i);
		MantidVec& eOut = m_outputWS->dataE(i);
		const MantidVec& xIn = m_inputWS->readX(i);
		const MantidVec& yIn = m_inputWS->readY(i);
		const MantidVec& eIn = m_inputWS->readE(i);
		m_outputWS->setX(i, m_inputWS->refX(i));

		MantidVec effVec = calculateEff(eff0, effFormula, xIn);
		// run this outside to benefit from parallel for
		applyDetEfficiency(numberOfChannels, yIn, eIn, effVec, yOut, eOut);

		prog.report("Detector Efficiency correction...");
	PARALLEL_END_INTERUPT_REGION
} //end for i
PARALLEL_CHECK_INTERUPT_REGION

this->setProperty("OutputWorkspace", this->m_outputWS);

}

/**
 * Apply the detector efficiency to a single spectrum
 * @param numberOfChannels Number of channels in a spectra (nbins - 1)
 * @param yIn spectrum counts
 * @param eIn spectrum errors
 * @param effVec efficiency values (to be divided by the counts)
 * @param yOut corrected spectrum counts
 * @param eOut corrected spectrum errors
 */
void DetectorEfficiencyCorUser::applyDetEfficiency(
		const size_t numberOfChannels, const MantidVec& yIn,
		const MantidVec& eIn, const MantidVec& effVec, MantidVec& yOut,
		MantidVec& eOut) {

	for (unsigned int j = 0; j < numberOfChannels; ++j) {
		//xOut[j] = xIn[j];
		yOut[j] = yIn[j] / effVec[j];
		eOut[j] = eIn[j] / effVec[j];
	}

}
/**
 * Calculate detector efficiency Eff0 : detector efficiency at the elastic peak
 * Parse the IDF formula_eff0 parameter name and  calculate the value
 * @return detector efficiency Eff0
 */
double DetectorEfficiencyCorUser::calculateEff0() {

// get formula: <parameter name="formula_eff0" type="string">
// <value val="exp(-0.0565/sqrt(e0))*(1.-exp(-3.284/sqrt(e0)))" />

std::string formula = getValFromInstrumentDef("formula_eff0");

try {
	mu::Parser p;
	p.DefineVar("e0", &m_Ei);
	p.SetExpr(formula);
	double eff0 = p.Eval();
	//g_log.debug() <<"e0 = " << m_Ei << " : eff0 = " << eff0 << "\n";
	return eff0;

} catch (mu::Parser::exception_type &e) {
	throw Kernel::Exception::InstrumentDefinitionError(
			"Error calculating Eff0 from IDF. Muparser error message is: "
					+ e.GetMsg());
}
}
/**
 * Calculate detector efficiency given a formula, the efficiency at the elastic line,
 * and a vector with energies.
 * Hope all compilers supports the NRVO (otherwise will copy the output vector)
 * @param eff0 :: calculated eff0
 * @param formula :: formula to calculate efficiency (parsed from IDF)
 * @param xIn :: Energy bins vector (X axis)
 * @return a vector with the efficiencies
 */
MantidVec DetectorEfficiencyCorUser::calculateEff(double eff0,
	std::string formula, const MantidVec& xIn) {

// get formula: <parameter name="formula_eff" type="string">
// <value val="0.951/eff0*exp(-0.0887/sqrt(e))*(1.0-exp(-5.597/sqrt(e)))" />

try {
	double e = 0; //dummyValue
	mu::Parser p;
	p.DefineVar("eff0", &eff0);
	p.DefineVar("e", &e);
	p.SetExpr(formula);

	MantidVec effOut(xIn.size() - 1); // x are bins and have more one value than y

	MantidVec::const_iterator xIn_it = xIn.begin();
	MantidVec::iterator effOut_it = effOut.begin();
	for (; effOut_it != effOut.end(); ++xIn_it, ++effOut_it) {
		e = (*xIn_it + *(xIn_it + 1)) / 2; // change the value of e in the expression (average value in a bin

		//g_log.debug() << i << " eff0=" << eff0 << " e=" << e <<  " xIn_it=" << *xIn_it << " *(xIn_it + 1)=" <<  *(xIn_it + 1);
		double eff = p.Eval(); // eval the expression
		//g_log.debug() << "eff = " << eff << "\n";
		*effOut_it = eff;
	}

//		for (auto i: effOut)
//			std::cout << i << " ";
//		std::cout << "\n";

	return effOut;

} catch (mu::Parser::exception_type &e) {
	throw Kernel::Exception::InstrumentDefinitionError(
			"Error calculating Eff0 from IDF. Muparser error message is: "
					+ e.GetMsg());
}
}

/**
 * Returns the value associated to a parameter name in the IDF
 * @param parameterName :: parameter name in the IDF
 * @return the value associated to the parameter name
 */
std::string DetectorEfficiencyCorUser::getValFromInstrumentDef(
	std::string parameterName) {

const ParameterMap& pmap = m_inputWS->constInstrumentParameters();
Instrument_const_sptr instrument = m_inputWS->getInstrument();
Parameter_sptr par = pmap.getRecursive(instrument->getChild(0).get(),
		parameterName);
if (par) {
	std::string ret = par->asString();
	//g_log.debug() << "Parsed parameter " << parameterName << ": " << ret << "\n";
	return ret;
} else {
	throw Kernel::Exception::InstrumentDefinitionError(
			"There is no <" + parameterName
					+ "> in the instrument definition!");
}

}

/** Loads and checks the values passed to the algorithm
 *
 *  @throw invalid_argument if there is an incapatible property value so the algorithm can't continue
 */
void DetectorEfficiencyCorUser::retrieveProperties() {

// Get the workspaces
m_inputWS = this->getProperty("InputWorkspace");

m_outputWS = this->getProperty("OutputWorkspace");

// If input and output workspaces are not the same, create a new workspace for the output
if (m_outputWS != this->m_inputWS) {
	m_outputWS = API::WorkspaceFactory::Instance().create(m_inputWS);
}

// these first three properties are fully checked by validators
m_Ei = this->getProperty("IncidentEnergy");
// If we're not given an Ei, see if one has been set.
if (m_Ei == EMPTY_DBL()) {
	Mantid::Kernel::Property* prop = m_inputWS->run().getProperty("Ei");
	double val;
	if (!prop || !Strings::convert(prop->value(), val)) {
		throw std::invalid_argument(
				"No Ei value has been set or stored within the run information.");
	}
	m_Ei = val;
	g_log.debug() << "Using stored Ei value " << m_Ei << "\n";
} else {
	g_log.debug() << "Using user input Ei value: " << m_Ei << "\n";
}
}

}
// namespace Algorithms
}// namespace Mantid
