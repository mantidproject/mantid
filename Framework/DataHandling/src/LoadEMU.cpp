#include <math.h>
#include <stdio.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadANSTOEventFile.h"
#include "MantidDataHandling/LoadEMU.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <boost/math/special_functions/round.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Util/PropertyFileConfiguration.h>

namespace Mantid {
namespace DataHandling {

	using namespace Kernel;
	using ANSTO::EventVector_pt;

	// Detector consts 
	static const size_t DETECTORS = 51;
	static const size_t HISTO_BINS_X = DETECTORS * 2;	// direct and indirect detectors
	static const size_t HISTO_BINS_Y = 1024;
	static const size_t HISTO_BINS_Y_DENUMERATOR = 16;
	static const size_t HISTOGRAMS = HISTO_BINS_X * HISTO_BINS_Y / HISTO_BINS_Y_DENUMERATOR;
	static const size_t PIXELS_PER_TUBE = HISTO_BINS_Y / HISTO_BINS_Y_DENUMERATOR;
	static const size_t HORIZONTAL_TUBES = 16;
	static const size_t VERTICAL_TUBES = 35;

	using TimeLimits = std::pair<double, double>;

namespace EMU {

	// map the comma separated range of indexes to the vector via a lambda function
	// throws an exception if it is outside the vector range
	//
	template <typename T, typename F>
	void mapRangeToIndex(const std::string& line, std::vector<T>& result, const F& fn) {

		std::stringstream ss(line);
		std::string item;
		size_t index = 0;
		while (std::getline(ss, item, ',')) {
			auto k = item.find('-');

			size_t p0, p1;
			if (k != std::string::npos) {
				p0 = boost::lexical_cast<size_t>(item.substr(0, k));
				p1 = boost::lexical_cast<size_t>(
					item.substr(k + 1, item.size() - k - 1));
			}
			else {
				p0 = boost::lexical_cast<size_t>(item);
				p1 = p0;
			}

			if (p1 < result.size() && p0 <= p1) {
				while (p0 <= p1) {
					result[p0++] = fn(index);
					index++;
				}
			}
			else if (p0 < result.size() && p1 < p0) {
				do {
					result[p0] = fn(index);
					index++;
				} while (p1 < p0--);
			}
			else
				throw std::invalid_argument("invalid range specification");
		}
	}

	// secant invert fucntion
	//
	template <typename F>
	double invert(double y, const F &f, double x0 = 0.0, const double eps = 1e-16) {
		// secant method
		double e0 = f(x0) - y;

		double x1 = x0 + eps;
		double e1 = f(x1) - y;
		int loop = 16;
		while (abs(e0) > eps && loop-- > 0) {
			double x = (x1 * e0 - x0 * e1) / (e0 - e1);

			x1 = x0;
			e1 = e0;

			x0 = x;
			e0 = f(x0) - y;
		}

		return x0;
	}

	// Need to convert two different methods for direct and analysed data
	// provide two distinct methods that may be called but the distances between
	// the detectors is not constant so it needs to store the distance for each
	// Note that observation time and TOF are in usec
	//
	class ConvertTOF {
		const double m_w;
		const double m_phi;
		const double m_L0;
		const double m_v2;
		const double m_A;
		std::vector<double>	m_L2;

		inline double L1(double t) const {
			return m_L0 + m_A * sin(m_w * t + m_phi);
		}

		inline double v1(double t) const {
			return m_v2 - m_A * m_w * cos(m_w * t + m_phi);
		}

	public:
		ConvertTOF(double Amp, double freq, double phase, double L1, double v2, std::vector<double>& L2)
			: m_w(2 * M_PI * freq),
			m_phi(phase),
			m_L0(L1),
			m_v2(v2),
			m_A(Amp),
			m_L2(L2)
		{}

		double directTOF(size_t detID, double tobs) const {

			// observation time and tof are in usec 
			auto tn = [=](double t) {
				return t + (L1(t) + m_L2[detID]) / v1(t);
			};

			double tsec = tobs * 1.0e-6;
			double t0 = tsec - (m_L0 + m_L2[detID]) / m_v2;
			double tinv = invert(tsec, tn, t0);
			double tof = m_L0 / v1(tinv) + m_L2[detID] / m_v2;

			return tof * 1.0e6;
		}

		double analysedTOF(size_t detID, double tobs) const {
			// observation time and tof are in usec 
			auto tn = [=](double t) {
				return t + L1(t) / v1(t) + m_L2[detID] / m_v2;
			};

			double tsec = tobs * 1.0e-6;
			double t0 = tsec - (m_L0 + m_L2[detID]) / m_v2;
			double t = invert(tsec, tn, t0);
			double tof = m_L0 / v1(t) + m_L2[detID] / m_v2;

			return tof * 1.0e6;
		}
	};

	// Implement emu specific handlers for the more general EMU events. The main differences with the 
	// current impementation of handlers:
	// - tof needs to be derived from the observed time because of the doppler drive
	// - emu includes direct and indirect virtual detectors that account for alternate paths
	// - the loader returns returns the doppler and auxillary time along with the absolute time rather 
	//   than just the primary observed time that is equivalent to tof
	//
	// In the future the ANSTO helper and event file loader will be generalized to handle the instruments 
	// consistently.

	class EventProcessor {
	protected:
		// fields
		const std::vector<bool> &m_roi;
		const std::vector<size_t> &m_mapIndex;
		const size_t m_stride;
		const double m_framePeriod;
		const double m_gatePeriod;

		// number of frames
		size_t m_frames;
		size_t m_framesValid;

		// time boundaries
		const TimeLimits	m_timeBoundary;		// seconds
		const TimeLimits	m_directTaux;		// microsec
		const TimeLimits	m_analysedTaux;		// microsec

		virtual void addEventImpl(size_t id, size_t x, size_t y, double tof) = 0;

	public:
		EventProcessor(const std::vector<bool> &roi, const std::vector<size_t>& mapIndex, 
			const size_t stride, const double framePeriod, const double gatePeriod,
			const TimeLimits& timeBoundary, const TimeLimits& directLimits,
			const TimeLimits& analysedLimits)
			:   m_roi(roi), m_mapIndex(mapIndex), m_stride(stride), m_frames(0), m_framesValid(0), 	
				m_framePeriod(framePeriod), m_gatePeriod(gatePeriod),
				m_timeBoundary(timeBoundary),m_directTaux(directLimits),
				m_analysedTaux(analysedLimits) {
		}

		void newFrame() {
			m_frames++; 
			if (validFrame())
				m_framesValid++;
		}

		inline bool validFrame() const {
			double frameTime = m_framePeriod * m_frames * 1.0e-6;
			return (frameTime >= m_timeBoundary.first && frameTime <= m_timeBoundary.second);
		}

		double duration() const {
			// length test in seconds			
			return m_framePeriod * m_frames * 1.0e-6;
		}
		void addEvent(size_t x, size_t p, double tdop, double taux) {

			// check if in time boundaries
			if (!validFrame())
				return;


			// group pixels
			size_t y = static_cast<size_t>(p / HISTO_BINS_Y_DENUMERATOR);

			// determine detector id and check limits
			if (x >= DETECTORS || y >= m_stride)
				return;

			// map the raw detector index to the physical model 
			size_t xid = m_mapIndex[x];

			// take the modules of the taux time to account for the 
			// longer background chopper rate 
			double ptaux = fmod(taux, m_gatePeriod);
			if (ptaux >= m_directTaux.first && ptaux <= m_directTaux.second)
				xid = xid + DETECTORS;
			else if (!(ptaux >= m_analysedTaux.first && ptaux <= m_analysedTaux.second))
				return;

			size_t id = m_stride * xid + y;
			if (id >= m_roi.size() || (id < 0))
				return;

			// check if neutron is in region of interest
			if (!m_roi[id])
				return;

			// finally pass to specific handler
			addEventImpl(id, xid, y, tdop);	
		}
	};

	class EventCounter : public EventProcessor {
	protected:
		// fields
		std::vector<size_t> &m_eventCounts;

		void EventCounter::addEventImpl(size_t id, size_t x, size_t y, double tof) override {
			m_eventCounts[id]++;
		}	
	
	public:
		// construction
		EventCounter(const std::vector<bool> &roi, const std::vector<size_t>& mapIndex, 
			const size_t stride, const double framePeriod, const double gatePeriod,
			const TimeLimits& timeBoundary, const TimeLimits& directLimits,
			const TimeLimits& analysedLimits, std::vector<size_t> &eventCounts)
			:	EventProcessor(roi, mapIndex, stride, framePeriod, gatePeriod,
				timeBoundary, directLimits, analysedLimits),
				m_eventCounts(eventCounts) {
		}

		size_t numFrames() const { return m_framesValid; }
	};

	class EventAssigner : public EventProcessor {
	protected:
		// fields
		std::vector<EventVector_pt> &m_eventVectors;
		const ConvertTOF&			m_convertTOF;
		double		m_tofMin;
		double		m_tofMax;
		bool		m_saveAsTOF;

		void addEventImpl(size_t id, size_t x, size_t y, double tobs) override {
			
			// convert observation time to tof 
			double tof = tobs;
			if (m_saveAsTOF)
				tof = x < DETECTORS ? m_convertTOF.analysedTOF(id, tobs) : m_convertTOF.directTOF(id, tobs);

			if (m_tofMin > tof)
				m_tofMin = tof;
			if (m_tofMax < tof)
				m_tofMax = tof;

			m_eventVectors[id]->push_back(tof);
		}

	public:
		EventAssigner(const std::vector<bool> &roi, const std::vector<size_t>& mapIndex, 
			const size_t stride, const double framePeriod, const double gatePeriod,
			const TimeLimits& timeBoundary, const TimeLimits& directLimits,
			const TimeLimits& analysedLimits, ConvertTOF& convert,
			std::vector<EventVector_pt> &eventVectors, bool saveAsTOF)
			:	EventProcessor(roi, mapIndex, stride, framePeriod, gatePeriod, 
				timeBoundary, directLimits, analysedLimits),
				m_convertTOF(convert),
				m_saveAsTOF(saveAsTOF),
				m_eventVectors(eventVectors), 
				m_tofMin(std::numeric_limits<double>::max()),
				m_tofMax(std::numeric_limits<double>::min()) {
		}

		double tofMin() const {
			return m_tofMin <= m_tofMax ? m_tofMin : 0.0;
		}
		double tofMax() const {
			return m_tofMin <= m_tofMax ? m_tofMax : 0.0;
		}
	};
}

	// register the algorithm into the AlgorithmFactory
	DECLARE_FILELOADER_ALGORITHM(LoadEMU)
														
	static const size_t Progress_LoadBinFile = 48;
	static const size_t Progress_ReserveMemory = 4;
	static const size_t Progress_Total =
		2 * Progress_LoadBinFile + Progress_ReserveMemory;

	static char const *const FilenameStr = "Filename";
	static char const *const MaskStr = "Mask";
	static char const *const SelectDetectorTubesStr = "SelectDetectorTubes";

	static char const *const OverrideDopplerPhaseStr = "OverrideDopplerPhase";
	static char const *const FilterByTimeStartStr = "FilterByTimeStart";
	static char const *const FilterByTimeStopStr = "FilterByTimeStop";

	static char const *const RawDopplerTimeStr = "LoadAsRawDopplerTime";

	// Single value properties only support int, double, string and bool
	template <typename Type>
	Type GetNeXusValue(NeXus::NXEntry &entry, const std::string &path, const Type &defval) {
		try {
			NeXus::NXDataSetTyped<Type> dataSet = entry.openNXDataSet<Type>(path);
			dataSet.load();

			return *dataSet();
		}
		catch (std::runtime_error &) {
			return defval;
		}
	}

	// string and double are special cases
	template <>
	double GetNeXusValue<double>(NeXus::NXEntry &entry, const std::string &path, const double &defval) {
		try {
			NeXus::NXDataSetTyped<float> dataSet = entry.openNXDataSet<float>(path);
			dataSet.load();

			return *dataSet();
		}
		catch (std::runtime_error &) {
			return defval;
		}
	}
	template <>
	std::string GetNeXusValue<std::string>(NeXus::NXEntry &entry, const std::string &path, const std::string &defval) {

		try {
			NeXus::NXChar dataSet = entry.openNXChar(path);
			dataSet.load();

			return std::string(dataSet(), dataSet.dim0());
		}
		catch (std::runtime_error &) {
			return defval;
		}
	}

	template <typename T>
	void MapNeXusToProperty(NeXus::NXEntry &entry, const std::string &path, const T &defval, 
		API::LogManager &logManager, const std::string &name, const T &factor) {

		T value = GetNeXusValue<T>(entry, path, defval);
		logManager.addProperty<T>(name, value * factor);
	}

	// sting is a special case
	template <>
	void MapNeXusToProperty<std::string>(NeXus::NXEntry &entry, const std::string &path, const std::string &defval, 
		API::LogManager &logManager,const std::string &name, const std::string &) {

		std::string	 value = GetNeXusValue<std::string>(entry, path, defval);
		logManager.addProperty<std::string>(name, value);
	}

	/**
	* Return the confidence value that this algorithm can load the file
	* @param descriptor A descriptor for the file
	* @returns An integer specifying the confidence level. 0 indicates it will not
	* be used
	*/
	int LoadEMU::confidence(Kernel::FileDescriptor &descriptor) const {
		if (descriptor.extension() != ".tar")
			return 0;

		ANSTO::Tar::File file(descriptor.filename());
		if (!file.good())
			return 0;

		size_t hdfFiles = 0;
		size_t binFiles = 0;
		const std::vector<std::string> &subFiles = file.files();
		for (const auto &subFile : subFiles) {
			auto len = subFile.length();
			if ((len > 4) &&
				(subFile.find_first_of("\\/", 0, 2) == std::string::npos)) {
				if ((subFile.rfind(".hdf") == len - 4) &&
					(subFile.compare(0, 3, "EMU") == 0))
					hdfFiles++;
				else if (subFile.rfind(".bin") == len - 4)
					binFiles++;
			}
		}

		return (hdfFiles == 1) && (binFiles == 1) ? 50 : 0;
	}

	/**
	* Initialise the algorithm. Declare properties which can be set before
	* execution (input) and
	* read from after the execution (output).
	*/
	void LoadEMU::init() {
		// Specify file extensions which can be associated with a specific file.
		std::vector<std::string> exts;

		// Declare the Filename algorithm property. Mandatory. Sets the path to the
		// file to load.
		exts.clear();
		exts.emplace_back(".tar");
		declareProperty(Kernel::make_unique<API::FileProperty>(
			FilenameStr, "", API::FileProperty::Load, exts),
			"The input filename of the stored data");

		// mask
		exts.clear();
		exts.emplace_back(".xml");
		declareProperty(Kernel::make_unique<API::FileProperty>(
			MaskStr, "", API::FileProperty::OptionalLoad, exts),
			"The input filename of the mask data");

		declareProperty(
			Kernel::make_unique<Kernel::PropertyWithValue<std::string>>(
				SelectDetectorTubesStr, "", Kernel::Direction::Input),
			"Optional: Comma separated range of detectors tubes to be loaded,"
			"  eg. 16,19-45,47");

		// OutputWorkspace
		declareProperty(
			Kernel::make_unique<API::WorkspaceProperty<API::IEventWorkspace>>(
				"OutputWorkspace", "", Kernel::Direction::Output));

		// OverrideDopplerPhase
		declareProperty(
			Kernel::make_unique<Kernel::PropertyWithValue<double>>(
				OverrideDopplerPhaseStr, EMPTY_DBL(), Kernel::Direction::Input),
			"Optional: Override the Doppler phase, in degrees.");

		// RawDopplerTime
		declareProperty(
			Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
				RawDopplerTimeStr, false, Kernel::Direction::Input),
			"Optional: Load file as observed time relative the Doppler drive, in microsecs.");

		// FilterByTimeStart
		declareProperty(
			Kernel::make_unique<Kernel::PropertyWithValue<double>>(
				FilterByTimeStartStr, 0.0, Kernel::Direction::Input),
			"Optional: To only include events after the provided start time, in "
			"seconds (relative to the start of the run).");

		// FilterByTimeStop
		declareProperty(
			Kernel::make_unique<Kernel::PropertyWithValue<double>>(
				FilterByTimeStopStr, EMPTY_DBL(), Kernel::Direction::Input),
			"Optional: To only include events before the provided stop time, in "
			"seconds (relative to the start of the run).");

		std::string grpOptional = "Filters";
		setPropertyGroup(FilterByTimeStartStr, grpOptional);
		setPropertyGroup(FilterByTimeStopStr, grpOptional);
	}

	/**
	 * Creates an event workspace and sets the title. 
	 */
	void LoadEMU::createWorkspace(ANSTO::Tar::File& tarFile) {

		// Clean up and check file
		// -----------------------
		// Delete the output workspace name if it existed
		std::string outName = getPropertyValue("OutputWorkspace");
		if (API::AnalysisDataService::Instance().doesExist(outName))
			API::AnalysisDataService::Instance().remove(outName);

		// Create the workspace
		// --------------------
		m_localWorkspace = boost::make_shared<DataObjects::EventWorkspace>();

		m_localWorkspace->initialize(HISTOGRAMS,
			2, // number of TOF bin boundaries
			1);

		// set the units
		m_localWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
		m_localWorkspace->setYUnit("Counts");

		// set title
		const std::vector<std::string> &subFiles = tarFile.files();
		for (const auto &subFile : subFiles)
			if (subFile.compare(0, 3, "EMU") == 0) {
				std::string title = subFile;

				if (title.rfind(".hdf") == title.length() - 4)
					title.resize(title.length() - 4);

				if (title.rfind(".nx") == title.length() - 3)
					title.resize(title.length() - 3);

				m_localWorkspace->setTitle(title);
				break;
			}
	}

	/**
	* Execute the algorithm. The steps involved are:
	*   Get the instrument properties and load options
	*   Create the workspace
	*   Load the instrument from the IDF
	*   Reposition the relevant neutronic values for model based on the parameters
	*   Load the data values and convert to TOF
	*/
	void LoadEMU::exec() {

		// Create workspace
		// 
		std::string filename = getPropertyValue(FilenameStr);
		ANSTO::Tar::File tarFile(filename);
		if (!tarFile.good())
			throw std::invalid_argument("invalid EMU file");
		createWorkspace(tarFile);
		API::LogManager &logManager = m_localWorkspace->mutableRun();
		API::Progress prog(this, 0.0, 1.0, Progress_Total);

		// Load instrument and workspace properties
		// 
		loadParameters(tarFile, logManager);

		// region of intreset and load filters
		std::string maskfile = getPropertyValue(MaskStr);
		std::string seltubes = getPropertyValue(SelectDetectorTubesStr);
		std::vector<bool> roi = createRoiVector(seltubes, maskfile);

		// absolute time limits when loading data
		double timeMaxBoundary = getProperty(FilterByTimeStopStr);
		if (isEmpty(timeMaxBoundary))
			timeMaxBoundary = std::numeric_limits<double>::infinity();
		TimeLimits	timeBoundary(getProperty(FilterByTimeStartStr), timeMaxBoundary);

		prog.doReport("creating instrument");

		// Load the instrument model 
		// 
		API::IAlgorithm_sptr loadInstrumentAlg =
			createChildAlgorithm("LoadInstrument");
		loadInstrumentAlg->setProperty("Workspace", m_localWorkspace);
		loadInstrumentAlg->setPropertyValue("InstrumentName", "EMUau");
		loadInstrumentAlg->setProperty("RewriteSpectraMap",
			Mantid::Kernel::OptionalBool(false));
		loadInstrumentAlg->executeAsChildAlg();


		// simplify loading instrument parameters
		auto instr = m_localWorkspace->getInstrument();		
		auto iparam = [&instr](std::string tag) {
			return instr->getNumberParameter(tag)[0];
		};		

		// Update the neutronic positions
		// 
		double sampleAnalyser = iparam("SampleAnalyser");

		// Indirect hz tubes idstart = 0
		for (int detID = 0; detID < HORIZONTAL_TUBES * PIXELS_PER_TUBE; detID++)
			updateNeutronicPostions(detID, sampleAnalyser);

		// Indirect vertical tubes idstart = 16 * 64
		int startID = 16 * PIXELS_PER_TUBE;
		for (int detID = 0; detID < VERTICAL_TUBES * PIXELS_PER_TUBE; detID++)
			updateNeutronicPostions(detID + startID, sampleAnalyser);


		// get the detector map from raw to physical detector
		std::string	dmapStr = instr->getParameterAsString("DetectorMap");
		std::vector<size_t>	detMapIndex = std::vector<size_t>(DETECTORS, 0);
		EMU::mapRangeToIndex(dmapStr, detMapIndex, [](size_t n) { return n; });

		// Collect the L2 distances, Doppler characteristics and initiate TOF converter 
		//			
		loadDetectorL2Values();		
		double dopplerFreq = logManager.getPropertyValueAsType<double>("DopplerFrequency");
		double dopplerAmpl = logManager.getPropertyValueAsType<double>("DopplerAmplitude");		
		double dopplerPhase = getProperty(OverrideDopplerPhaseStr);
		if (isEmpty(dopplerPhase)) {
			// sinusoidal motion crossing a threshold with a delay
			double doppThreshold = iparam("DopplerReferenceThreshold");
			double doppDelay = iparam("DopplerReferenceDelay");
			dopplerPhase = 180.0 - asin(0.001 * doppThreshold / dopplerAmpl) * 180.0 / M_PI + doppDelay * dopplerFreq;
		}
		logManager.addProperty<double>("DopplerPhase", dopplerPhase);
		
		double v2 = iparam("AnalysedV2");			// analysed velocity in metres per sec
		double framePeriod = 1.0e6 / dopplerFreq;						// period and max direct as microsec
		double sourceSample = iparam("SourceSample");	
		EMU::ConvertTOF	convertTOF(dopplerAmpl, dopplerFreq,
			dopplerPhase, sourceSample, v2, m_detectorL2);

		Types::Core::DateAndTime start_time(logManager.getPropertyValueAsType<std::string>("StartTime"));
		std::string time_str = start_time.toISO8601String();

		// Load the events file
		// --------------------
		// First count the number of events to reserve memory and then assign the events to the detectors

		// load events
		size_t numberHistograms = m_localWorkspace->getNumberHistograms();
		std::vector<EventVector_pt> eventVectors(numberHistograms, nullptr);
		std::vector<size_t> eventCounts(numberHistograms, 0);


		// Discriminating between direct and analysed is based on the auxillary time and is 
		// determined by the graphite chopper frequency and v2 which are stable so these
		// limits are kept in the instrument parameter file. 
		// Convert from milsec to microsec.		
		TimeLimits	directLimits(1000.0 * iparam("DirectTauxMin"), 1000.0 * iparam("DirectTauxMax"));
		TimeLimits	analysedLimits(1000.0 * iparam("AnalysedTauxMin"), 1000.0 * iparam("AnalysedTauxMax"));

		// fabs because the value can be negative
		double gatePeriod = 1.0e6 / fabs(logManager.getPropertyValueAsType<double>("GraphiteChopperFrequency"));

		// count total events per pixel to reserve necessary memory		
		EMU::EventCounter eventCounter(
			roi, detMapIndex, PIXELS_PER_TUBE, framePeriod, gatePeriod,
			timeBoundary, directLimits, analysedLimits, eventCounts);

		loadEvents(prog, "loading neutron counts", tarFile, eventCounter);

		// prepare event storage
		ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists",
			numberHistograms, Progress_ReserveMemory);

		for (size_t i = 0; i != numberHistograms; ++i) {
			DataObjects::EventList &eventList = m_localWorkspace->getSpectrum(i);

			eventList.setSortOrder(DataObjects::PULSETIME_SORT);
			eventList.reserve(eventCounts[i]);

			eventList.setDetectorID(static_cast<detid_t>(i));
			eventList.setSpectrumNo(static_cast<detid_t>(i));

			DataObjects::getEventsFrom(eventList, eventVectors[i]);

			progTracker.update(i);
		}
		progTracker.complete();

		bool saveAsTOF = !getProperty(RawDopplerTimeStr);
		EMU::EventAssigner eventAssigner(
			roi, detMapIndex, PIXELS_PER_TUBE, framePeriod, gatePeriod, 
			timeBoundary, directLimits, analysedLimits, 
			convertTOF, eventVectors, saveAsTOF);

		loadEvents(prog, "loading neutron events (TOF)", tarFile, eventAssigner);

		// just to make sure the bins hold it all
		m_localWorkspace->setAllX(
			HistogramData::BinEdges{ std::max(0.0, floor(eventAssigner.tofMin())),
			eventAssigner.tofMax() + 1 });

		// count total number of masked bins
		size_t maskedBins = 0;
		for (size_t i = 0; i != roi.size(); i++)
			if (!roi[i])
				maskedBins++;

		if (maskedBins > 0) {
			// create list of masked bins
			std::vector<size_t> maskIndexList(maskedBins);
			size_t maskIndex = 0;

			for (size_t i = 0; i != roi.size(); i++)
				if (!roi[i])
					maskIndexList[maskIndex++] = i;

			API::IAlgorithm_sptr maskingAlg = createChildAlgorithm("MaskDetectors");
			maskingAlg->setProperty("Workspace", m_localWorkspace);
			maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
			maskingAlg->executeAsChildAlg();
		}

		// set log values
		int frame_count = static_cast<int>(eventCounter.numFrames());

		logManager.addProperty("filename", filename);
		logManager.addProperty("frame_count", frame_count);

		Types::Core::time_duration duration = boost::posix_time::microseconds(
			static_cast<boost::int64_t>(eventCounter.duration() * 1.0e6));
		Types::Core::DateAndTime end_time(start_time + duration);
		logManager.addProperty("start_time", start_time.toISO8601String());
		logManager.addProperty("end_time", end_time.toISO8601String());

		setProperty("OutputWorkspace", m_localWorkspace);
	}

	/**
	 *	Recovers the L2 neutronic distance for each detector.  
	 */
	void LoadEMU::loadDetectorL2Values() {

		m_detectorL2 = std::vector<double>(HISTOGRAMS);
		const auto &detectorInfo = m_localWorkspace->detectorInfo();
		auto detIDs = detectorInfo.detectorIDs();
		for (const auto detID : detIDs) {
			auto ix = detectorInfo.indexOf(detID);
			double l2 = detectorInfo.l2(ix);
			m_detectorL2[detID] = l2;
		}
	}

	// update the neutronic positins
	void LoadEMU::updateNeutronicPostions(int detID, double sampleAnalyser) {

		// get the instrument 

		// get the list of indirect horizontal detectors

		// for each detector get the current position and scale 
		// the detectors 

		Geometry::Instrument_const_sptr instrument =
			m_localWorkspace->getInstrument();
		auto &compInfo = m_localWorkspace->mutableComponentInfo();

		try {
			auto component = instrument->getDetector(detID);
			const auto componentId = component->getComponentID();
			double rho, theta, phi;
			V3D position = component->getPos();
			position.getSpherical(rho, theta, phi);

			double scale = -(2 * sampleAnalyser + rho) / rho;
			position *= scale;

			const auto componentIndex = compInfo.indexOf(component->getComponentID());
			compInfo.setPosition(componentIndex, position);
		}
		catch (const std::runtime_error &) {
			//throw std::runtime_error("Runtime error moving detID " + detID);
		}
	}

	// region of interest is defined by the selected detectors and the mask
	//
	std::vector<bool> LoadEMU::createRoiVector(const std::string& selected, const std::string& maskfile) {

		std::vector<bool> result(HISTOGRAMS, true);

		// turn off pixels linked to missing tubes
		if (!selected.empty()) {
			std::vector<bool> tubes(HISTO_BINS_X, false);
			EMU::mapRangeToIndex(selected, tubes, [](size_t n) { return true; });
			for (int i = 0; i < HISTO_BINS_X; i++) {
				if (tubes[i] == false) {
					for (int j = 0; j < PIXELS_PER_TUBE; j++) {
						result[i * PIXELS_PER_TUBE + j] = false;
					}
				}
			}
		}

		if (maskfile.length() == 0)
			return result;

		std::ifstream input(maskfile.c_str());
		if (!input.good())
			throw std::invalid_argument("invalid mask file");

		std::string line;
		while (std::getline(input, line)) {
			auto i0 = line.find("<detids>");
			auto iN = line.find("</detids>");

			if ((i0 != std::string::npos) && (iN != std::string::npos) && (i0 < iN)) {
				line = line.substr(i0 + 8, iN - i0 - 8); // 8 = len("<detids>")
				EMU::mapRangeToIndex(line, result, [](size_t n) { return false; });
			}
		}

		return result;
	}

	// instrument creation
	void LoadEMU::loadParameters(ANSTO::Tar::File &tarFile, API::LogManager &logm) {

		// extract log and hdf file
		const std::vector<std::string> &files = tarFile.files();
		auto file_it =
			std::find_if(files.cbegin(), files.cend(), [](const std::string &file) {
			return file.rfind(".hdf") == file.length() - 4;
		});
		if (file_it != files.end()) {
			tarFile.select(file_it->c_str());
			// extract hdf file into tmp file
			Poco::TemporaryFile hdfFile;
			boost::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"), fclose);
			if (handle) {
				// copy content
				char buffer[4096];
				size_t bytesRead;
				while (0 != (bytesRead = tarFile.read(buffer, sizeof(buffer))))
					fwrite(buffer, bytesRead, 1, handle.get());
				handle.reset();

				NeXus::NXRoot root(hdfFile.path());
				NeXus::NXEntry entry = root.openFirstEntry();

				MapNeXusToProperty<std::string>(entry, "sample/name", "unknown", logm, "SampleName", "");
				MapNeXusToProperty<std::string>(entry, "sample/description", "unknown", logm, "SampleDescription", "");
				MapNeXusToProperty<std::string>(entry, "start_time", "2000-01-01T00:00:00", logm, "StartTime", "");

				MapNeXusToProperty<double>(entry, "instrument/doppler/ctrl/amplitude", 75.0, logm, "DopplerAmplitude", 0.001);
				double speedToFreq = 0.5 / (M_PI * m_localWorkspace->run().getPropertyValueAsType<double>("DopplerAmplitude"));
				MapNeXusToProperty<double>(entry, "instrument/doppler/ctrl/velocity", 4.7, logm, "DopplerFrequency", speedToFreq);

				MapNeXusToProperty<double>(entry, "instrument/chpr/background/actspeed", 1272.8, logm, "BackgroundChopperFrequency", 1.0 / 60);
				MapNeXusToProperty<double>(entry, "instrument/chpr/graphite/actspeed", 2545.6, logm, "GraphiteChopperFrequency", 1.0 / 60);
				MapNeXusToProperty<double>(entry, "instrument/hztubegap", 0.02, logm, "HorizontalTubesGap", 1.0);
				MapNeXusToProperty<int32_t>(entry, "monitor/bm1_counts", 0, logm, "MonitorCounts", 1);

				// temp fix for source position when loading IDF
				MapNeXusToProperty<double>(entry, "instrument/doppler/tosource", 2.035, logm, "SourceSample", 1.0);

			}
		}

		// patching
		file_it = std::find(files.cbegin(), files.cend(), "History.log");
		if (file_it != files.cend()) {
			tarFile.select(file_it->c_str());
			std::string logContent;
			logContent.resize(tarFile.selected_size());
			tarFile.read(&logContent[0], logContent.size());
			std::istringstream data(logContent);
			Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> conf(
				new Poco::Util::PropertyFileConfiguration(data));

			if (conf->hasProperty("SampleName")) {
				std::string name = conf->getString("SampleName");
				logm.addProperty("SampleName", name);
			}
		}
	}

	// read counts/events from binary file
	template <class EventProcessor>
	void LoadEMU::loadEvents(API::Progress &prog, const char *progMsg,
		ANSTO::Tar::File &tarFile,
		EventProcessor &eventProcessor) {

		using namespace ANSTO;

		prog.doReport(progMsg);

		// select bin file
		int64_t fileSize = 0;
		const std::vector<std::string> &files = tarFile.files();
		for (const auto &file : files)
			if (file.rfind(".bin") == file.length() - 4) {
				tarFile.select(file.c_str());
				fileSize = tarFile.selected_size();
				break;
			}

		// for progress notifications
		ANSTO::ProgressTracker progTracker(prog, progMsg, fileSize,
			Progress_LoadBinFile);

		ReadEventFile(tarFile, eventProcessor, progTracker, 100, false);

	}
}
}