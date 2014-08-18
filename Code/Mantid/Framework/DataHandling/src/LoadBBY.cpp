#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidNexus/NexusClasses.h"

//#include <Poco/File.h>

//#ifndef fseek64
//#define fseek64 _fseeki64
//#endif
//#ifndef ftell64
//#define ftell64 _ftelli64
//#endif

namespace Mantid
{
  namespace DataHandling
  {
    // register the algorithm into the AlgorithmFactory
    DECLARE_FILELOADER_ALGORITHM(LoadBBY);

    // consts
    static const size_t HISTO_BINS_X = 240;
    static const size_t HISTO_BINS_Y = 256;
    // 100 = 40 + 20 + 40
    static const size_t Progress_LoadBinFile   = 40;
    static const size_t Progress_ReserveMemory = 20;
    
    // pointer to the vector of events
    typedef std::vector<DataObjects::TofEvent> *EventVector_pt;

    // helper class to keep track
    class ProgressTracker {
    private:
      // fields
      const std::string _msg;
      size_t _count;
      int64_t _step;
      int64_t _next;
      // matntid
      API::Progress &_progBar;

    public:
      // construction
      ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count);
      ~ProgressTracker();

      // methods
      void Update(int64_t position);
      void Complete();
    };
    
    class TmpFile {
    private:
      // fields
      bool _good;
      char _path[L_tmpnam];
      
      // not supported
      TmpFile(const TmpFile&);
      TmpFile& operator =(const TmpFile&);

    public:
      // construction
      TmpFile();
      ~TmpFile();

      // properties
      bool good() const;
      const char* path() const;

      // methods
      FILE* create(const char *mode);
      bool remove();
    };

    class DetectorBankFactory {
    private:
      // fields
      const Geometry::Instrument_sptr _instrument;
      const Geometry::Object_sptr     _pixelShape;
      const size_t _xPixelCount;
      const size_t _yPixelCount;
      const double _pixelWidth;
      const double _pixelHeight;
      const Kernel::V3D _center;

    public:
      // construction
      DetectorBankFactory(
        Geometry::Instrument_sptr instrument,
        Geometry::Object_sptr pixelShape,
        size_t xPixelCount,
        size_t yPixelCount,
        double pixelWidth,
        double pixelHeight,
        const Kernel::V3D &center);
      
      // methods
      void CreateAndAssign(size_t startIndex, const Kernel::V3D &pos, const Kernel::Quat &rot);
    };

    class EventCounter {
    private:
      // fields
      std::vector<size_t> &_eventCounts;
      double _tofMin;
      double _tofMax;

    public:
      // construction
      EventCounter(std::vector<size_t> &eventCounts);

      // properties
      double tofMin() const;
      double tofMax() const;

      // methods
      void AddEvent(size_t x, size_t y, double tof);
    };
    
    class EventAssigner {
    private:
      // fields
      std::vector<EventVector_pt> &_eventVectors;

    public:
      // construction
      EventAssigner(std::vector<EventVector_pt> &eventVectors);
      
      // methods
      void AddEvent(size_t x, size_t y, double tof);
    };

    class FastReadOnlyFile {
    private:
#ifdef WIN32
      HANDLE _handle;
#else
      FILE *_handle;
#endif
    public:
      // construction
      FastReadOnlyFile(const char *filename);
      ~FastReadOnlyFile();

      // properties
      void* handle() const;

      // methods
      bool Read(void *buffer, uint32_t size);
      bool Seek(int64_t offset, int whence, int64_t *newPosition = NULL);
    };

    namespace BbyTar {

      enum TypeFlag : char {
        NormalFile        = '0',
        HardLink          = '1',
        SymbolicLink      = '2',
        CharacterSpecial  = '3',
        BlockSpecial      = '4',
        Directory         = '5',
        FIFO              = '6',
        ContiguousFile    = '7',
      };
      
      struct EntryHeader {
        char FileName[100];	
        char FileMode[8];
        char OwnerUserID[8];
        char OwnerGroupID[8];
        char FileSize[12];          // in bytes (octal base)
        char LastModification[12];  // time in numeric Unix time format (octal)
        char Checksum[8];
        TypeFlag TypeFlag;
        char LinkedFileName[100];
        char UStar[8];
        char OwnerUserName[32];
        char OwnerGroupName[32];
        char DeviceMajorNumber[8];
        char DeviceMinorNumber[8];
        char FilenamePrefix[155];
      };
      
      template<size_t N>
      int64_t OctalToInt(char (&str)[N]);
      
      class File {

        static const auto BufferSize = 4096;
  
        struct FileInfo {
          int64_t Offset;
          int64_t Size;
        };

      private:
        // fields
        bool  _good;
        FastReadOnlyFile _file;
        std::vector<std::string> _fileNames;
        std::vector<FileInfo>    _fileInfos;
        // selected file
        size_t _selected; // index
        int64_t _position;
        int64_t _size;
        // buffer
        uint8_t _buffer[BufferSize];
        size_t _bufferPosition;
        size_t _bufferAvailable;

        // not supported
        File(const File&);
        File& operator =(const File&);

      public:
        // construction
        File(const std::string &path);

        // properties
        bool good() const;
        const std::vector<std::string>& files() const;
        // from selected file
        const std::string& selected_name() const;
        int64_t selected_position() const;
        int64_t selected_size() const;
  
        // methods
        bool select(const char *file);
        bool skip(uint64_t offset);
        size_t read(void *dst, size_t size);
        int read_byte();
      };
      
      template<size_t N>
      int64_t OctalToInt(char (&str)[N]) {
          int64_t result = 0;
          char *p = str;
          for (size_t n = N; n > 1; --n) { // last character is '\0'
            char c = *p++;
            if (('0' <= c) && (c <= '9'))
              result = result * 8 + (c - '0');
          }
          return result;
      }

      // construction
      File::File(const std::string &path) :
        _good(true),
        _selected((size_t)-1),
        _position(0),
        _size(0),
        _file(path.c_str()) {

        _good = _file.handle() != NULL;
        while (_good) {
          EntryHeader header;
          int64_t position;
          
          _good &= _file.Read(&header, sizeof(EntryHeader));
          _good &= _file.Seek(512 - sizeof(EntryHeader), SEEK_CUR, &position);
          if (!_good)
            break;

          std::string fileName(header.FileName);
          if (fileName.length() == 0)
            return;

          FileInfo fileInfo;
          fileInfo.Offset = position;
          fileInfo.Size   = OctalToInt(header.FileSize);
    
          if (header.TypeFlag == NormalFile) {
            _fileNames.push_back(std::move(fileName));
            _fileInfos.push_back(fileInfo);
          }
    
          size_t offset = (size_t)(fileInfo.Size % 512);
          if (offset != 0)
            offset = 512 - offset;
          
          _good &= _file.Seek(fileInfo.Size + offset, SEEK_CUR);
        }
      }

      // properties
      bool File::good() const {
        return _good;
      }
      const std::vector<std::string>& File::files() const {
        return _fileNames;
      }
      const std::string& File::selected_name() const {
        return _fileNames[_selected];
      }
      int64_t File::selected_position() const {
        return _position;
      }
      int64_t File::selected_size() const {
        return _size;
      }

      // methods
      bool File::select(const char *file) {
        if (!_good)
          return false;

        // reset buffer
        _bufferPosition  = 0;
        _bufferAvailable = 0;

        for (size_t i = 0; i != _fileNames.size(); i++)
          if (_fileNames[i] == file) {
            const FileInfo &info = _fileInfos[i];

            _selected = i;
            _position = 0;
            _size     = info.Size;
      
            return _good &= _file.Seek(info.Offset, SEEK_SET);
          }

        _selected = (size_t)-1;
        _position = 0;
        _size     = 0;
        return false;
      }
      bool File::skip(uint64_t offset) {
        if (!_good || (_selected == (size_t)-1))
          return false;
  
        bool overrun = offset > (uint64_t)(_size - _position);
        if (overrun)
          offset = _size - _position;

        _position += offset;

        uint64_t bufferPosition = (uint64_t)_bufferPosition + offset;
        if (bufferPosition <= _bufferAvailable)
          _bufferPosition = (size_t)bufferPosition;
        else {
          _good &= _file.Seek(bufferPosition - _bufferAvailable, SEEK_CUR);

          _bufferPosition  = 0;
          _bufferAvailable = 0;
        }

        return _good && !overrun;
      }
      size_t File::read(void *dst, size_t size) {
        if (!_good || (_selected == (size_t)-1))
          return 0;
        
        if (size > (_size - _position))
          size = _size - _position;

        auto ptr = (uint8_t*)dst;
        size_t result = 0;

        if (_bufferPosition != _bufferAvailable) {
          result = _bufferAvailable - _bufferPosition;
          if (result > size)
            result = size;

          memcpy(ptr, _buffer, result);
          ptr += result;

          size -= result;
          _position += result;
          _bufferPosition += result;
        }
        
        while (size != 0) {
          auto bytesToRead = (uint32_t)std::min<size_t>(
            size,
            std::numeric_limits<uint32_t>::max());

          _good &= _file.Read(ptr, bytesToRead);
          if (!_good)
            break;

          ptr += bytesToRead;
          
          size -= bytesToRead;
          result += bytesToRead;
          _position += bytesToRead;
        }

        return result;
      }
      int File::read_byte() {
        if (!_good || (_selected == (size_t)-1))
          return -1;

        if (_bufferPosition == _bufferAvailable) {
          if (_position >= _size)
            return -1;
    
          _bufferPosition  = 0;
          _bufferAvailable = 0;

          uint32_t size = (uint32_t)std::min<int64_t>(sizeof(_buffer), _size - _position);
          _good &= _file.Read(_buffer, size);

          if (_good)
            _bufferAvailable = size;
          else
            return -1;
        }

        _position++;
        return _buffer[_bufferPosition++];
      }
    }

    /**
     * Return the confidence value that this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadBBY::confidence(Kernel::FileDescriptor & descriptor) const {
      if (descriptor.extension() != ".tar")
        return 0;

      BbyTar::File file(descriptor.filename());
      if (!file.good())
        return 0;

      size_t hdfFiles = 0;
      size_t binFiles = 0;
      const std::vector<std::string> &subFiles = file.files();
      for (auto itr = subFiles.begin(); itr != subFiles.end(); ++itr) {
        auto len = itr->length();
        if ((len > 4) && (itr->find_first_of("\\/", 0, 2) == std::string::npos)) {
          if ((itr->rfind(".hdf") == len - 4) && (itr->compare(0, 3, "BBY") == 0))
            hdfFiles++;
          else if (itr->rfind(".bin") == len - 4)
            binFiles++;
        }
      }

      return (hdfFiles == 1) && (binFiles == 1) ? 50 : 0;
    }
    /**
     * Initialise the algorithm. Declare properties which can be set before execution (input) or 
     * read from after the execution (output).
     */
    void LoadBBY::init() {
      // Specify file extensions which can be associated with a BBY file.
      std::vector<std::string> exts;

      // Declare the Filename algorithm property. Mandatory. Sets the path to the file to load.
      exts.clear();
      exts.push_back(".tar");
      declareProperty(
        new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
        "The input filename of the stored data");
      
      declareProperty(
        new API::WorkspaceProperty<API::IEventWorkspace>("OutputWorkspace", "", Kernel::Direction::Output));
      
      declareProperty(
        new Kernel::PropertyWithValue<double>("FilterByTofMin", 0, Kernel::Direction::Input),
        "Optional: To exclude events that do not fall within a range of times-of-flight. "\
        "This is the minimum accepted value in microseconds. Keep blank to load all events.");
      declareProperty(
        new Kernel::PropertyWithValue<double>("FilterByTofMax", 50000000, Kernel::Direction::Input),
        "Optional: To exclude events that do not fall within a range of times-of-flight. "\
        "This is the maximum accepted value in microseconds. Keep blank to load all events." );
      declareProperty(
        new Kernel::PropertyWithValue<double>("FilterByTimeStart", EMPTY_DBL(), Kernel::Direction::Input),
        "Optional: To only include events after the provided start time, in seconds (relative to the start of the run).");
      declareProperty(
        new Kernel::PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(), Kernel::Direction::Input),
        "Optional: To only include events before the provided stop time, in seconds (relative to the start of the run).");

      std::string grpOptional = "Optional";
      setPropertyGroup("FilterByTofMin"   , grpOptional);
      setPropertyGroup("FilterByTofMax"   , grpOptional);
      setPropertyGroup("FilterByTimeStart", grpOptional);
      setPropertyGroup("FilterByTimeStop" , grpOptional);
    }
    /**
     * Execute the algorithm.
     */
    void LoadBBY::exec() {

      // Delete the output workspace name if it existed
      std::string outName = getPropertyValue("OutputWorkspace");
      if (API::AnalysisDataService::Instance().doesExist(outName))
        API::AnalysisDataService::Instance().remove(outName);

      // Get the name of the file.
      std::string filename = getPropertyValue("Filename");
      BbyTar::File file(filename);
      if (!file.good())
        return;
      
      size_t nBins  = 1;
      double tofMinBoundary = getProperty("FilterByTofMin");
      double tofMaxBoundary = getProperty("FilterByTofMax");
      
      // "loading neutron counts", "creating neutron event lists" and "loading neutron events"
      API::Progress prog(this, 0.0, 1.0, Progress_LoadBinFile + Progress_ReserveMemory + Progress_LoadBinFile);
      prog.doReport("creating instrument");

      // create workspace
      DataObjects::EventWorkspace_sptr eventWS = boost::make_shared<DataObjects::EventWorkspace>();

      eventWS->initialize(
          HISTO_BINS_Y * HISTO_BINS_X,
          nBins + 1,  // number of TOF bin boundaries
          nBins);

      // set the units
      eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
      eventWS->setYUnit("Counts"); 
      
      // set title
      const std::vector<std::string>& subFiles = file.files();
      for (auto itr = subFiles.begin(); itr != subFiles.end(); ++itr)
        if (itr->compare(0, 3, "BBY") == 0) {
          std::string title = *itr;

          if (title.rfind(".hdf") == title.length() - 4)
            title.resize(title.length() - 4);
          
          if (title.rfind(".nx") == title.length() - 3)
            title.resize(title.length() - 3);

          eventWS->setTitle(title);
          break;
        }
      
      // set auxiliaries
      eventWS->mutableRun().addProperty("Filename", filename);
      //eventWS->mutableRun().addProperty("run_number", 1);
      //eventWS->mutableRun().addProperty("run_start", "1991-01-01T00:00:00", true );
      //eventWS->mutableRun().addProperty("duration", duration[0], units);

      // create instrument
      Geometry::Instrument_sptr instrument = CreateInstrument(file);
      eventWS->setInstrument(instrument);

      // load events
                  
      size_t numberHistograms = eventWS->getNumberHistograms();

      std::vector<EventVector_pt> eventVectors(numberHistograms, NULL);
      std::vector<size_t>         eventCounts(numberHistograms, 0);
		  std::vector<detid_t>        detIDs = instrument->getDetectorIDs();

      // count total events per pixel to reserve necessary memory
      EventCounter eventCounter(eventCounts);
      LoadEvents<EventCounter>(prog, "loading neutron counts", file, tofMinBoundary, tofMaxBoundary, eventCounter);
      
      // prepare event storage
      ProgressTracker progTracker(prog, "creating neutron event lists", numberHistograms, Progress_ReserveMemory);
      for (size_t i = 0; i != numberHistograms; ++i) {
        DataObjects::EventList& eventList = eventWS->getEventList(i);

        eventList.setSortOrder(DataObjects::PULSETIME_SORT); // why not PULSETIME[TOF]_SORT ?
        eventList.reserve(eventCounts[i]);

        detid_t id = detIDs[i];
        eventList.setDetectorID(id);
			  eventList.setSpectrumNo(id);

        DataObjects::getEventsFrom(eventList, eventVectors[i]);

        progTracker.Update(i);
      }
      progTracker.Complete();
      
      LoadEvents<EventAssigner>(prog, "loading neutron events", file, tofMinBoundary, tofMaxBoundary, EventAssigner(eventVectors));
      
      Kernel::cow_ptr<MantidVec> axis;
      MantidVec &xRef = axis.access();
      xRef.resize(2, 0.0);
      xRef[0] = std::max(0.0, eventCounter.tofMin() - 1); // just to make sure the bins hold it all
      xRef[1] = eventCounter.tofMax() + 1;
      eventWS->setAllX(axis);

      setProperty("OutputWorkspace", eventWS);
    }

    // instrument creation
    Geometry::Instrument_sptr LoadBBY::CreateInstrument(BbyTar::File &tarFile) {
      // instrument
      Geometry::Instrument_sptr instrument = boost::make_shared<Geometry::Instrument>("BILBY");
      instrument->setDefaultViewAxis("Z-");

      // source
      Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
      instrument->add(source);
      instrument->markAsSource(source);
            
      //// chopper
      //Geometry::ObjComponent *chopperPoint = new Geometry::ObjComponent("Chopper", instrument.get());
      //instrument->add(chopper);
      //instrument->markAsChopperPoint(chopper);

      // sample
      Geometry::ObjComponent *samplePos = new Geometry::ObjComponent("Sample", instrument.get());
      instrument->add(samplePos);
      instrument->markAsSamplePos(samplePos);
      
      double L1_chopper_value   = 18.47258984375;
      double L1_source_value    =  9.35958984375;
      double L2_det_value       = 33.15616015625;

      double L2_curtainl_value  = 23.28446093750;
      double L2_curtainr_value  = 23.28201953125;
      double L2_curtainu_value  = 24.28616015625;
      double L2_curtaind_value  = 24.28235937500;
      
      double D_det_value = (8.4 + 2.0) / (2 * 1000);
      
      double D_curtainl_value   = 0.3816;
      double D_curtainr_value   = 0.4024;
      double D_curtainu_value   = 0.3947;
      double D_curtaind_value   = 0.3978;
      
      // extract hdf file
      int64_t fileSize = 0;
      const std::vector<std::string> &files = tarFile.files();
      for (auto itr = files.begin(); itr != files.end(); ++itr)
        if (itr->rfind(".hdf") == itr->length() - 4) {
          tarFile.select(itr->c_str());
          fileSize = tarFile.selected_size();
          break;
        }

      if (fileSize != 0) {
        // create tmp file
        TmpFile hdfFile;
        std::shared_ptr<FILE> handle(hdfFile.create("wb"), fclose);
        if (handle) {
          // copy content
          char buffer[4096];
          size_t bytesRead;
          while (0 != (bytesRead = tarFile.read(buffer, sizeof(buffer))))
            fwrite(buffer, bytesRead, 1, handle.get());
          handle.reset();

          NeXus::NXRoot root(hdfFile.path());
          NeXus::NXEntry entry = root.openFirstEntry();

          NeXus::NXFloat L1           = entry.openNXDataSet<float>("instrument/L1");
          NeXus::NXFloat L2_det       = entry.openNXDataSet<float>("instrument/L2_det");
          NeXus::NXFloat Ltof_det     = entry.openNXDataSet<float>("instrument/Ltof_det");
        
          L1.load();
          L2_det.load();
          Ltof_det.load();

          NeXus::NXFloat L2_curtainl  = entry.openNXDataSet<float>("instrument/L2_curtainl");
          NeXus::NXFloat L2_curtainr  = entry.openNXDataSet<float>("instrument/L2_curtainr");
          NeXus::NXFloat L2_curtainu  = entry.openNXDataSet<float>("instrument/L2_curtainu");
          NeXus::NXFloat L2_curtaind  = entry.openNXDataSet<float>("instrument/L2_curtaind");

          L2_curtainl.load();
          L2_curtainr.load();
          L2_curtainu.load();
          L2_curtaind.load();
          
          NeXus::NXFloat D_curtainl   = entry.openNXDataSet<float>("instrument/detector/curtainl");
          NeXus::NXFloat D_curtainr   = entry.openNXDataSet<float>("instrument/detector/curtainr");
          NeXus::NXFloat D_curtainu   = entry.openNXDataSet<float>("instrument/detector/curtainu");
          NeXus::NXFloat D_curtaind   = entry.openNXDataSet<float>("instrument/detector/curtaind");

          D_curtainl.load();
          D_curtainr.load();
          D_curtainu.load();
          D_curtaind.load();

          const double toMeters = 1.0 / 1000;

          L2_det_value     = *L2_det()   * toMeters;
          L1_chopper_value = *Ltof_det() * toMeters - L2_det_value;
          L1_source_value  = *L1()       * toMeters;
        
          L2_curtainl_value = *L2_curtainl() * toMeters;
          L2_curtainr_value = *L2_curtainr() * toMeters;
          L2_curtainu_value = *L2_curtainu() * toMeters;
          L2_curtaind_value = *L2_curtaind() * toMeters;
          
          D_curtainl_value = *D_curtainl() * toMeters;
          D_curtainr_value = *D_curtainr() * toMeters;
          D_curtainu_value = *D_curtainu() * toMeters;
          D_curtaind_value = *D_curtaind() * toMeters;
        }
      }

      source   ->setPos(0.0, 0.0, -L1_chopper_value);
      samplePos->setPos(0.0, 0.0, 0.0              );
      
      // create a component for the detector
      
      size_t xPixelCount = HISTO_BINS_X / 6;
      size_t yPixelCount = HISTO_BINS_Y;
      size_t pixelCount  = xPixelCount * yPixelCount;

      // dimensions of the detector (height is in y direction, width is in x direction)
      double width  = 336.0 / 1000; // meters
      double height = 640.0 / 1000; // meters
      double angle  =  10.0;        // degree
      
      // we assumed that individual pixels have the same size and shape of a cuboid with dimensions:
      double pixel_width  = width  / static_cast<double>(xPixelCount);
      double pixel_height = height / static_cast<double>(yPixelCount);
            
      // Create size strings for shape creation
      std::string pixel_width_str  = boost::lexical_cast<std::string>(pixel_width / 2);
      std::string pixel_height_str = boost::lexical_cast<std::string>(pixel_height / 2);
      std::string pixel_depth_str  = "0.00001"; // Set the depth of a pixel to a very small number

      // Define shape of a pixel as an XML string. See http://www.mantidproject.org/HowToDefineGeometricShape for details on shapes in Mantid.
      std::string detXML =
        "<cuboid id=\"pixel\">"
          "<left-front-bottom-point   x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
          "<left-front-top-point      x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\""+pixel_depth_str+"\"  />"
          "<left-back-bottom-point    x=\"-"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
          "<right-front-bottom-point  x=\"+"+pixel_width_str+"\" y=\"+"+pixel_height_str+"\" z=\"0\"  />"
        "</cuboid>";

      // Create a shape object which will be shared by all pixels.
      Geometry::Object_sptr pixelShape = Geometry::ShapeFactory().createShape(detXML);
      
      // create detector banks
      DetectorBankFactory factory(
        instrument,
        pixelShape,
        xPixelCount,
        yPixelCount,
        pixel_width,
        pixel_height,
        Kernel::V3D(0, (height - pixel_height) / 2, 0));

      // curtain l
      factory.CreateAndAssign(
        0 * pixelCount,
        Kernel::V3D(+D_curtainl_value, 0, L2_curtainl_value),
        Kernel::Quat(  0, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
      // curtain r
      factory.CreateAndAssign(
        1 * pixelCount,
        Kernel::V3D(-D_curtainr_value, 0, L2_curtainr_value),
        Kernel::Quat(180, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
      // curtain u
      factory.CreateAndAssign(
        2 * pixelCount,
        Kernel::V3D(0, +D_curtainu_value, L2_curtainu_value),
        Kernel::Quat( 90, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
      // curtain d
      factory.CreateAndAssign(
        3 * pixelCount,
        Kernel::V3D(0, -D_curtaind_value, L2_curtaind_value),
        Kernel::Quat(-90, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
      // back 1 (left)
      factory.CreateAndAssign(
        4 * pixelCount,
        Kernel::V3D(+D_det_value, 0, L2_det_value),
        Kernel::Quat(  0, Kernel::V3D(0, 0, 1)));
      
      // back 2 (right)
      factory.CreateAndAssign(
        5 * pixelCount,
        Kernel::V3D(-D_det_value, 0, L2_det_value),
        Kernel::Quat(180, Kernel::V3D(0, 0, 1)));
      
      return instrument;
    }

    // read counts/events from binary file
    template<class Counter>
    void LoadBBY::LoadEvents(
      API::Progress &prog,
      const char *progMsg,
      BbyTar::File &file,
      const double tofMinBoundary,
      const double tofMaxBoundary,
      Counter &counter) {
      prog.doReport(progMsg);
      
      // select bin file
      int64_t fileSize = 0;
      const std::vector<std::string> &files = file.files();
      for (auto itr = files.begin(); itr != files.end(); ++itr)
        if (itr->rfind(".bin") == itr->length() - 4) {
          file.select(itr->c_str());
          fileSize = file.selected_size();
          break;
        }
      
      // for progress notifications
      ProgressTracker progTracker(prog, progMsg, fileSize, Progress_LoadBinFile);

      unsigned int x = 0;   // 9 bits [0-239] tube number
      unsigned int y = 0;   // 8 bits [0-255] position along tube

      //uint v = 0; // 0 bits [     ]
      //uint w = 0; // 0 bits [     ] energy
      unsigned int dt = 0;
    
      if ((fileSize == 0) || !file.skip(128))
        return;

      int state = 0;
      unsigned int c;
      while ((c = file.read_byte()) != -1) {

        bool event_ended = false;
        switch (state) {
          case 0:
            x  = (c & 0xFF) << 0;   // set bit 1-8
            break;

          case 1:
            x |= (c & 0x01) << 8;   // set bit 9
            y  = (c & 0xFE) >> 1;   // set bit 1-7
            break;

          case 2:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            y |= (c & 0x01) << 7;   // set bit 8
            dt = (c & 0xFE) >> 1;   // set bit 1-5(7)
            break;

          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            dt |= (c & 0xFF) << (5 + 6 * (state - 3)); // set bit 6...
            break;
        }
        state++;

        if (event_ended || (state == 8)) {
          state = 0;

          if ((x == 0) && (y == 0) && (dt == 0xFFFFFFFF)) {
          }
          else if ((x >= HISTO_BINS_X) || (y >= HISTO_BINS_Y)) {
          }
          else {
            // conversion from 100 nanoseconds to 1 microseconds
            double tof = dt * 0.1;

            if ((tofMinBoundary <= tof) && (tof <= tofMaxBoundary))
              counter.AddEvent(x, y, tof);
          }
          
          progTracker.Update(file.selected_position());
        }
      }
    }

    // ProgressTracker
    ProgressTracker::ProgressTracker(API::Progress &progBar, const char *msg, int64_t target, size_t count) :
      _msg(msg), _count(count), _step(target / count), _next(_step),
      _progBar(progBar) {

      _progBar.doReport(_msg);
    }
    ProgressTracker::~ProgressTracker() {
      Complete();
    }
    void ProgressTracker::Update(int64_t position) {
        if (_next <= position) {
          if (_count != 0) {
            _progBar.report(_msg);
            _next += _step;
            _count--;
          }
          else {
            _next = -1;
          }
        }
    }
    void ProgressTracker::Complete() {
      if (_count != 0) {
        _progBar.reportIncrement(_count, _msg);
        _count = 0;
      }
    }
    
    // TmpFile
    TmpFile::TmpFile() :
      _good(false) {
    }
    TmpFile::~TmpFile() {
      remove();
    }
    bool TmpFile::good() const {
      return _good;
    }
    const char* TmpFile::path() const {
      return _good ? _path : NULL;
    }
    FILE* TmpFile::create(const char *mode) {
      remove();
      if (tmpnam(_path) != NULL) {
        FILE* file = fopen(_path, mode);
        _good = file != NULL;
        return file;
      }
      return NULL;
    }
    bool TmpFile::remove() {
      if (_good) {
        _good = false;
        return ::remove(_path) == 0;
      }
      return false;
    }
    
    // DetectorBankFactory
    DetectorBankFactory::DetectorBankFactory(
      Geometry::Instrument_sptr instrument,
      Geometry::Object_sptr pixelShape,
      size_t xPixelCount,
      size_t yPixelCount,
      double pixelWidth,
      double pixelHeight,
      const Kernel::V3D &center) :
      _instrument(instrument),
      _pixelShape(pixelShape),
      _xPixelCount(xPixelCount),
      _yPixelCount(yPixelCount),
      _pixelWidth(pixelWidth),
      _pixelHeight(pixelHeight),
      _center(center) {
    }
    void DetectorBankFactory::CreateAndAssign(size_t startIndex, const Kernel::V3D &pos, const Kernel::Quat &rot) {
      // create a RectangularDetector which represents a rectangular array of pixels
      Geometry::RectangularDetector* bank = new Geometry::RectangularDetector("bank", _instrument.get()); // ??? possible memory leak!? "new" without "delete"

      bank->initialize(
        _pixelShape,
        // x
        (int)_xPixelCount,
        0,
        _pixelWidth,
        // y
        (int)_yPixelCount,
        0,
        _pixelHeight,
        // indices
        (int)startIndex,
        true,
        (int)_yPixelCount);
      
      for (size_t x = 0; x < _xPixelCount; ++x)
        for (size_t y = 0; y < _yPixelCount; ++y)
          _instrument->markAsDetector(bank->getAtXY((int)x, (int)y).get());
      
      Kernel::V3D center(_center);
      rot.rotate(center);

      bank->rotate(rot);
      bank->translate(pos - center);
    }
    
    // EventCounter
    EventCounter::EventCounter(std::vector<size_t> &eventCounts) :
      _eventCounts(eventCounts),
      _tofMin(std::numeric_limits<double>::max()),
      _tofMax(std::numeric_limits<double>::min()) {
    }
    double EventCounter::tofMin() const {
      return _tofMin <= _tofMax ? _tofMin : 0.0;
    }
    double EventCounter::tofMax() const {
      return _tofMin <= _tofMax ? _tofMax : 0.0;
    }
    void EventCounter::AddEvent(size_t x, size_t y, double tof) {
        if (_tofMin > tof)
          _tofMin = tof;
        if (_tofMax < tof)
          _tofMax = tof;

        _eventCounts[HISTO_BINS_Y * (x) + y]++;
    }

    // EventList
    EventAssigner::EventAssigner(std::vector<EventVector_pt> &eventVectors) :
      _eventVectors(eventVectors) {
    }
    void EventAssigner::AddEvent(size_t x, size_t y, double tof) {
      // conversion from 100 nanoseconds to 1 microseconds
      _eventVectors[HISTO_BINS_Y * (x) + y]->push_back(tof);
    }

    // FastReadOnlyFile
#ifdef WIN32
    FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
      _handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    FastReadOnlyFile::~FastReadOnlyFile() {
      CloseHandle(_handle);
      _handle = NULL;
    }
    void* FastReadOnlyFile::handle() const {
      return _handle;
    }
    bool FastReadOnlyFile::Read(void *buffer, uint32_t size) {
        DWORD bytesRead;
        return (FALSE != ReadFile(_handle, buffer, size, &bytesRead, NULL)) && (bytesRead == size);
    }
    bool FastReadOnlyFile::Seek(int64_t offset, int whence, int64_t *newPosition) {
      return FALSE != SetFilePointerEx(
        _handle,
        *(LARGE_INTEGER*)&offset,
        (LARGE_INTEGER*)newPosition,
        whence);
    }
#else
    FastReadOnlyFile::FastReadOnlyFile(const char *filename) {
      _handle = fopen(filename, "rb");
    }
    FastReadOnlyFile::~FastReadOnlyFile() {
      fclose(_handle);
      _handle = NULL;
    }
    void* FastReadOnlyFile::handle() const {
      return _handle;
    }
    bool FastReadOnlyFile::Read(void *buffer, uint32_t size) {
      return 1 == fread(buffer, (size_t)size, 1, _handle);
    }
    bool FastReadOnlyFile::Seek(int64_t offset, int whence, int64_t *newPosition) {
      return
        (0 == fseek(_handle, offset, whence)) &&
        ((newPosition == NULL) || (0 <= (*newPosition = (int64_t)ftell(_handle))));
    }
#endif
  }//namespace
}//namespace