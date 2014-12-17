#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDAlgorithms/LoadSQW.h"
#include "MantidAPI/RegisterFileLoader.h"
#include <iostream>
#include <cfloat>
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/BoxControllerNeXusIO.h"
#include "MantidKernel/Memory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

namespace {
//------------------------------------------------------------------------------------------------
/** Helper function allowing to typecast sequence of bytes into proper expected
*type.
* The input buffer is interpreted as the template type
*
* @param Buf -- the vector of characters, representing data to cast
* @param ind -- the starting position of first byte of data within the data
*buffer
* @returns the data type produced by type-casing proper sequence of bytes
*/
template <typename T> T interpretAs(std::vector<char> &Buf, size_t ind = 0) {
  return *((reinterpret_cast<T *>(&Buf[ind])));
}
}

DECLARE_FILELOADER_ALGORITHM(LoadSQW);

/// Constructor
LoadSQW::LoadSQW() : m_prog(new Mantid::API::Progress(this, 0.05, 0.95, 100)) {}
/**
* Return the confidence with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadSQW::confidence(Kernel::FileDescriptor &descriptor) const {

  // only .sqw can be considered
  const std::string &extn = descriptor.extension();
  if (extn.compare(".sqw") != 0)
    return 0;

  if (descriptor.isAscii()) {
    return 10; // Low so that others may try
  }
  return 80; // probably it is sqw indeed
}

/// Destructor
LoadSQW::~LoadSQW() { delete m_prog; }

/// Provide wiki documentation.

/// Initialize the algorithm
void LoadSQW::init() {
  std::vector<std::string> fileExtensions(1);
  fileExtensions[0] = ".sqw";
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
                                        fileExtensions),
                  "File of type SQW format");
  declareProperty(new API::WorkspaceProperty<API::IMDEventWorkspace>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "Output IMDEventWorkspace reflecting SQW data read-in.");
  declareProperty(new Kernel::PropertyWithValue<bool>("MetadataOnly", false),
                  "Load Metadata without events.");
  std::vector<std::string> fileExtensions2(1);
  fileExtensions2[0] = ".nxs";
  declareProperty(
      new API::FileProperty("OutputFilename", "",
                            API::FileProperty::OptionalSave, fileExtensions2),
      "If the input SQW file is too large to fit in memory, specify an output "
      "NXS file.\n"
      "The MDEventWorkspace will be create with this file as its back-end.");
}

/// Execute the algorithm
void LoadSQW::exec() {

  m_fileName = std::string(getProperty("Filename"));
  // Parse Extract metadata. Including data locations.
  parseMetadata(m_fileName);

  // Create a new output workspace.
  MDEventWorkspace<MDEvent<4>, 4> *pWs = new MDEventWorkspace<MDEvent<4>, 4>;
  Mantid::API::IMDEventWorkspace_sptr ws(pWs);

  // Add dimensions onto workspace.
  std::vector<Mantid::Geometry::MDHistoDimensionBuilder> DimVector;

  readDNDDimensions(DimVector, false);
  readSQWDimensions(DimVector);
  addDimsToWs(pWs, DimVector);
  // Set some reasonable values for the box controller
  BoxController_sptr bc = pWs->getBoxController();
  for (size_t i = 0; i < 4; i++) {
    bc->setSplitInto(i, m_nBins[i]);
  }
  bc->setMaxDepth(1);

  // Initialize the workspace.
  pWs->initialize();

  // Add oriented lattice.
  addLattice(pWs);

  // Start with a MDGridBox.
  pWs->splitBox();
  //
  readBoxSizes();

  // Save the empty WS and turn it into a file-backed MDEventWorkspace (on
  // option)
  m_outputFile = getPropertyValue("OutputFilename");
  if (!m_outputFile.empty()) {
    // set file backed;
    MemoryStats stat;
    if ((m_nDataPoints * sizeof(MDEvent<4>) * 2 / 1024) < stat.availMem())
      g_log.notice() << "You have enough memory available to load the "
                     << m_nDataPoints << " points into memory; this would be "
                                         "faster than using a file back-end."
                     << std::endl;

    IAlgorithm_sptr saver =
        this->createChildAlgorithm("SaveMD", 0.01, 0.05, true);
    saver->setProperty("InputWorkspace", ws);
    saver->setPropertyValue("Filename", m_outputFile);
    // should think about it.
    saver->setProperty("UpdateFileBackEnd", false);
    saver->setProperty("MakeFileBacked", false);
    saver->executeAsChildAlg();
    m_prog->resetNumSteps(100, 0.05, 0.75);

    // set file backed boxes
    auto Saver = boost::shared_ptr<API::IBoxControllerIO>(
        new MDEvents::BoxControllerNeXusIO(bc.get()));
    bc->setFileBacked(Saver, m_outputFile);
    pWs->getBox()->setFileBacked();
    bc->getFileIO()->setWriteBufferSize(1000000);

  } else {
    MemoryStats stat;
    if ((size_t(double(m_nDataPoints) * 1.5) * sizeof(MDEvent<4>) / 1024) >
        stat.availMem())
      g_log.warning()
          << "You may not have enough physical memory available to load the "
          << m_nDataPoints << " points into memory. You can cancel and specify "
                              "OutputFilename to load to a file back-end."
          << std::endl;
  }

  if (bc->isFileBacked()) {
    std::cout << "File backed? " << bc->isFileBacked() << ". Cache "
              << bc->getFileIO()->getMemoryStr() << std::endl;
  } else {
    bool ff(false);
    std::cout << "File backed? " << ff << ". Cache  0" << std::endl;
  }

  // Persist the workspace.
  API::IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
  if (i_out)
    throw std::runtime_error(
        "Cannot currently handle overwriting of an existing workspace.");
  setProperty("OutputWorkspace", ws);

  // Read events into the workspace.
  bool bMetadataOnly = getProperty("MetadataOnly");
  if (!bMetadataOnly)
    readEvents(pWs);

  progress(m_outputFile.empty() ? 0.96 : 0.76, "Refreshing cache");
  pWs->refreshCache();

  if (!m_outputFile.empty()) {
    g_log.notice() << "Starting SaveMD to update the file back-end."
                   << std::endl;
    IAlgorithm_sptr saver = this->createChildAlgorithm("SaveMD", 0.76, 1.00);
    saver->setProperty("InputWorkspace", ws);
    saver->setProperty("UpdateFileBackEnd", true);
    saver->executeAsChildAlg();
  }
}

/// Add events after reading pixels/datapoints from file.
void
    LoadSQW::readEvents(Mantid::MDEvents::MDEventWorkspace<MDEvent<4>, 4> *ws) {
  CPUTimer tim;

  size_t maxNPix = ~size_t(0);
  if (m_nDataPoints > maxNPix) {
    throw new std::runtime_error("Not possible to address all datapoints in "
                                 "memory using this architecture ");
  }

  const size_t ncolumns = 9; // qx, qy, qz, en, s, err, irunid, idetid, ien
  const size_t column_size =
      4; // types stored as either float32 or unsigned integer (both 4byte).
  const size_t column_size_2 = column_size * 2; // offset, gives qz
  const size_t column_size_3 = column_size * 3; // offset, gives en
  const size_t column_size_4 = column_size * 4; // offset, gives idet
  // const size_t column_size_5 = column_size * 5; //offset, gives ien
  const size_t column_size_6 = column_size * 6; // offset, gives irun
  const size_t column_size_7 = column_size * 7; // offset, gives signal
  const size_t column_size_8 = column_size * 8; // offset, gives error

  const size_t pixel_width = ncolumns * column_size;
  const size_t data_buffer_size = pixel_width * m_nDataPoints;
  g_log.information() << m_nDataPoints << " data points in this SQW file."
                      << std::endl;

  // Load from the input file is smallish blocks
  size_t blockSize = pixel_width * 1000000;

  // Report progress once per block
  int numBlocks = int((data_buffer_size + blockSize - 1) / blockSize);
  m_prog->setNumSteps(numBlocks);
  m_prog->setNotifyStep(0.1);

  // For tracking when to split boxes
  size_t eventsAdded = 0;
  BoxController_sptr bc = ws->getBoxController();
  DiskBuffer *dbuf(NULL);
  if (bc->isFileBacked())
    dbuf = bc->getFileIO();

  for (int blockNum = 0; blockNum < numBlocks; blockNum++) {
    // Start point in the file
    size_t inputFileOffset = size_t(blockNum) * blockSize;

    // Limit the size of the block
    size_t currentBlockSize = blockSize;
    if ((inputFileOffset + currentBlockSize) > data_buffer_size)
      currentBlockSize = data_buffer_size - inputFileOffset;

    // Load the block from the file
    std::vector<char> Buffer = std::vector<char>(currentBlockSize);
    this->m_fileStream.seekg(this->m_dataPositions.pix_start + inputFileOffset,
                             std::ios::beg);
    this->m_fileStream.read(&Buffer[0], currentBlockSize);

    // Go through each pixel in the input
    int currentNumPixels = int(currentBlockSize / pixel_width);
    eventsAdded += size_t(currentNumPixels);

    // Add the events in parallel
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < currentNumPixels; i++) {
      size_t current_pix = size_t(i * pixel_width);
      coord_t centers[4] = {
          interpretAs<float>(Buffer, current_pix),
          interpretAs<float>(Buffer, current_pix + column_size),
          interpretAs<float>(Buffer, current_pix + column_size_2),
          interpretAs<float>(Buffer, current_pix + column_size_3)};
      float error = interpretAs<float>(Buffer, current_pix + column_size_8);
      ws->addEvent(MDEvent<4>(
          interpretAs<float>(Buffer, current_pix + column_size_7), // Signal
          error * error,                                           // Error sq
          static_cast<uint16_t>(interpretAs<float>(
              Buffer, current_pix + column_size_6)), // run Index
          static_cast<int32_t>(interpretAs<float>(
              Buffer, current_pix + column_size_4)), // Detector Id
          centers));
    }

    //        MemoryStats stat;
    //        size_t bytesAvail = stat.availMem() * 1024;
    //        // Estimate how many extra bytes will (temporarily) be used when
    //        splitting events
    //        size_t bytesNeededToSplit = eventsAdded * sizeof(MDEvent<4>) / 2;

    // Split:
    // 1. When < 1 GB of memory is free
    // 2. When too little memory might be available for the splitting operation
    // 3. When enough events have been added that it makes sense
    // 4. At the last block being added

    //        if ((bytesAvail < 1000000000) || (bytesAvail < bytesNeededToSplit)
    //        ||
    //            bc->shouldSplitBoxes(eventsAdded*2, lastNumBoxes) || (blockNum
    //            == numBlocks-1) )

    if (eventsAdded > 19000000) {
      g_log.information() << "Splitting boxes after " << eventsAdded
                          << " events added." << std::endl;
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();

      // This splits up all the boxes according to split thresholds and sizes.
      Kernel::ThreadScheduler *ts = new ThreadSchedulerFIFO();
      ThreadPool tp(ts);
      ws->splitAllIfNeeded(ts);
      tp.joinAll();

      // Flush the cache - this will save things out to disk
      if (dbuf)
        dbuf->flushCache();
      // Flush memory
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();
      eventsAdded = 0;
    }

    // This will be correct optimized code after all.
    //
    //        if (false)
    //        {
    //          std::vector<MDBoxBase<MDEvent<4>,4>*> boxes;
    //          ws->getBox()->getBoxes(boxes, 100, true);
    //          size_t modified = 0;
    //          size_t inmem = 0;
    //          size_t ondisk = 0;
    //          size_t events = 0;
    //          for (size_t i=0; i<boxes.size(); i++)
    //          {
    //            MDBox<MDEvent<4>,4>* box =
    //            dynamic_cast<MDBox<MDEvent<4>,4>*>(boxes[i]);
    //            if (box)
    //            {
    //              //box->save();
    //              if (box->dataAdded() || box->dataModified())
    //                modified++;
    //              if (box->getInMemory())
    //                inmem++;
    //              if (box->getOnDisk())
    //                ondisk++;
    //              events += box->getEventVectorSize();
    //            }
    //          }
    //          g_log.information() << modified << " of " << boxes.size() << "
    //          MDBoxes have data added or modified." << std::endl;
    //          g_log.information() << inmem << " MDBoxes are in memory." <<
    //          std::endl;
    //          //g_log.information() << ondisk << " MDBoxes are on disk." <<
    //          std::endl;
    //          g_log.information() << double(events)/1e6 << " million events in
    //          memory." << std::endl;
    //        }

    // Report progress once per block.
    m_prog->report();
  }
  Kernel::ThreadScheduler *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts);
  ws->splitAllIfNeeded(ts);
  tp.joinAll();

  // Flush the cache - this will save things out to disk
  if (dbuf)
    dbuf->flushCache();
  // Flush memory
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();
}

/**
Extract the b-matrix from a SQW file. Create experiment info with oriented
lattice and add to workspace.
@param ws : Workspace to modify.
*/
void
    LoadSQW::addLattice(Mantid::MDEvents::MDEventWorkspace<MDEvent<4>, 4> *ws) {
  std::vector<char> buf(
      4 * (3 + 3)); // Where 4 = size_of(float) and 3 * 3 is size of b-matrix.
  this->m_fileStream.seekg(this->m_dataPositions.geom_start, std::ios::beg);
  this->m_fileStream.read(&buf[0], buf.size());

  double a = static_cast<double>(interpretAs<float>(buf, 0));
  double b = static_cast<double>(interpretAs<float>(buf, 4));
  double c = static_cast<double>(interpretAs<float>(buf, 8));
  double aa = static_cast<double>(interpretAs<float>(buf, 12));
  double bb = static_cast<double>(interpretAs<float>(buf, 16));
  double cc = static_cast<double>(interpretAs<float>(buf, 20));

  ExperimentInfo_sptr info(new ExperimentInfo());
  // set up the goniometer. All mdEvents (pixels) in Horace sqw file are in lab
  // frame,
  // Q units so general goniometer should provide unit rotation matrix
  info->mutableRun().mutableGoniometer().makeUniversalGoniometer();
  //
  OrientedLattice latt(a, b, c, aa, bb, cc);
  info->mutableSample().setOrientedLattice(&latt);
  ws->addExperimentInfo(info);
}

void LoadSQW::buildMDDimsBase(
    std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVector) {
  std::vector<std::string> dimID(4, "qx");
  std::vector<std::string> dimUnit(4, "A^-1");
  dimID[1] = "qy";
  dimID[2] = "qz";
  dimID[3] = "en";
  dimUnit[3] = "meV";
  DimVector.resize(4);
  for (size_t i = 0; i < 4; i++) {
    DimVector[i].setId(dimID[i]);
    DimVector[i].setUnits(dimUnit[i]);
    DimVector[i].setName(dimID[i]);
  }
}

/// Add a dimension after reading info from file.
void LoadSQW::readDNDDimensions(
    std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVectorOut,
    bool arrangeByMDImage) {
  // using Mantid::Geometry::MDHistoDimensionBuilder;
  std::vector<Mantid::Geometry::MDHistoDimensionBuilder> DimVectorIn;
  this->buildMDDimsBase(DimVectorIn);

  std::vector<char> buf(4 * (3 + 3 + 4 + 16 + 4 + 2));
  this->m_fileStream.seekg(this->m_dataPositions.geom_start, std::ios::beg);

  this->m_fileStream.read(&buf[0], buf.size());
  // skip allat and adlngldef
  // interpret shifts
  size_t i0 = 4 * (3 + 3);
  // for(size_t i=0;i<this->m_nDims;i++){
  // double val = (double)*((float*)(&buf[i0+i*4]));
  // dscrptn.pDimDescription(i)->data_shift = val;
  //}

  // TODO: how to use it in our framework? -> it is B^-1 matrix possibly
  // re-scaled
  std::vector<double> u_to_Rlu(
      4 * 4); // the matrix transforming from lab to crystal frame with scaling
  i0 += 4 * 4;
  // [data.u_to_rlu, count, ok, mess] = fread_catch(fid,[4,4],'float32'); if
  // ~all(ok); return; end;
  size_t ic = 0;
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      u_to_Rlu[ic] =
          static_cast<double>(interpretAs<float>(buf, i0 + 4 * (i * 4 + j)));
      ic++;
    }
  }
  i0 += ic * 4;
  Mantid::Kernel::DblMatrix UEmat(u_to_Rlu);
  Mantid::Kernel::DblMatrix Rot(3, 3);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      Rot[i][j] = UEmat[i][j];
    }
  }
  // dscrptn.setRotationMatrix(Rot);

  // axis labels size
  i0 += 4 * 4;
  unsigned int nRows = interpretAs<uint32_t>(buf, i0);
  unsigned int nCols = interpretAs<uint32_t>(buf, i0 + 4);

  // read axis labelsg
  buf.resize(nRows * nCols);
  // [ulabel, count, ok, mess] = fread_catch(fid,[n(1),n(2)],'*char'); if
  // ~all(ok); return; end;

  this->m_fileStream.read(&buf[0], buf.size());

  // data.ulabel=cellstr(ulabel)';
  std::string name;
  char symb;
  name.resize(nCols);
  for (unsigned int i = 0; i < nRows; i++) {
    for (unsigned int j = 0; j < nCols; j++) {
      symb = buf[i + j * nRows];
      name[j] = symb;
    }
    // Trim string.
    std::string sName(name);
    boost::erase_all(sName, " ");

    DimVectorIn[i].setName(sName);
  }

  // resize for iax and npax;
  buf.resize(4 * 4 * 3);
  this->m_fileStream.read(&buf[0], 4);

  unsigned int npax = interpretAs<uint32_t>(buf);
  unsigned int niax = 4 - npax;

  /*
  [npax, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return;
  end;
  niax=4-npax;
  if niax~=0
  [data.iax, count, ok, mess] = fread_catch(fid,[1,niax],'int32'); if ~all(ok);
  return; end;
  [data.iint, count, ok, mess] = fread_catch(fid,[2,niax],'float32'); if
  ~all(ok); return; end;
  else
  data.iax=zeros(1,0);    % create empty index of integration array in standard
  form
  data.iint=zeros(2,0);
  end
  */
  // axis counter
  ic = 0;
  ;
  std::vector<unsigned int> iax;
  if (niax > 0) {
    buf.resize(4 * (niax + 2 * niax));
    iax.resize(niax);
    this->m_fileStream.read(&buf[0], buf.size());

    for (unsigned int i = 0; i < niax; i++) {
      iax[i] = interpretAs<uint32_t>(buf, i * 4) - 1;
      float min = interpretAs<float>(buf, 4 * (niax + i * 2));
      float max =
          interpretAs<float>(buf, 4 * (niax + i * 2 + 1)) * (1 + FLT_EPSILON);

      DimVectorIn[ic].setNumBins(1);
      DimVectorIn[ic].setMax(max);
      DimVectorIn[ic].setMin(min);
      ic++;
    }
  }

  // processing projection axis;

  /*
  if npax~=0
  [data.pax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok);
  return; end;
  psize=zeros(1,npax);    % will contain number of bins along each dimension of
  plot axes
  for i=1:npax
  [np,count,ok,mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
  [data.p{i},count,ok,mess] = fread_catch(fid,np,'float32'); if ~all(ok);
  return; end;
  psize(i)=np-1;
  end
  [data.dax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok);
  return; end;
  if length(psize)==1
  psize=[psize,1];    % make size of a column vector
  end
  else
  data.pax=zeros(1,0);    % create empty index of plot axes
  data.p=cell(1,0);
  data.dax=zeros(1,0);    % create empty index of plot axes
  psize=[1,1];    % to hold a scalar
  end
  */
  std::vector<unsigned int> pax, dax;
  if (npax > 0) {
    //[data.pax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if
    //~all(ok); return; end;
    this->m_fileStream.read(&buf[0], 4 * npax);
    pax.resize(npax);
    dax.resize(npax);
    for (unsigned int i = 0; i < npax; i++) {
      // projection axis indexes
      pax[i] = interpretAs<uint32_t>(buf, 4 * i) - 1;

      std::vector<char> axis_buffer(101 * 4);
      this->m_fileStream.read(&axis_buffer[0], 4);
      unsigned int nAxisPoints = interpretAs<uint32_t>(axis_buffer);
      if (axis_buffer.size() < nAxisPoints * 4)
        axis_buffer.resize(nAxisPoints * 4);

      this->m_fileStream.read(&axis_buffer[0], 4 * nAxisPoints);

      float min = interpretAs<float>(axis_buffer, 0);
      float max = interpretAs<float>(axis_buffer, 4 * (nAxisPoints - 1)) *
                  (1 + FLT_EPSILON);

      DimVectorIn[ic].setNumBins(nAxisPoints - 1);
      DimVectorIn[ic].setMax(max);
      DimVectorIn[ic].setMin(min);
      ic++;
    }
    //[data.dax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if
    //~all(ok); return; end;
    this->m_fileStream.read(&buf[0], 4 * npax);
    for (unsigned int i = 0; i < npax; i++)
      dax[i] = interpretAs<uint32_t>(buf, 4 * i) - 1;
  }
  if (arrangeByMDImage) {

    // Place dimensions to output vector in the correct dimensions order;
    size_t ic = 0;
    DimVectorOut.resize(4);
    for (size_t i = 0; i < npax; i++) {
      DimVectorOut[ic] = DimVectorIn[pax[dax[i]]];
      ic++;
    }

    for (size_t i = 0; i < niax; i++) {
      DimVectorOut[ic] = DimVectorIn[iax[i]];
      ic++;
    }
  } else // arrange according to sqw
  {
    DimVectorOut.assign(DimVectorIn.begin(), DimVectorIn.end());
  }

  // set up proper dimension bin numbers to use in further calculations
  this->m_nBins.resize(4);
  for (size_t i = 0; i < 4; i++) {
    m_nBins[i] = DimVectorOut[i].getNumBins();
    if (m_nBins[i] < 1)
      m_nBins[i] = 1;
  }
}
/// add range of dimensions to the workspace;
void LoadSQW::addDimsToWs(
    Mantid::MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> *ws,
    std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVector) {
  // Add dimensions to the workspace by invoking the dimension builders.
  for (size_t i = 0; i < 4; i++) {
    ws->addDimension(DimVector[i].create());
  }
}

/// Add a dimension after reading info from file.
void LoadSQW::readSQWDimensions(
    std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVectorOut) {
  using Mantid::Geometry::MDHistoDimensionBuilder;

  this->buildMDDimsBase(DimVectorOut);

  std::vector<char> buf(4 * (4 + 4));
  this->m_fileStream.seekg(this->m_dataPositions.min_max_start, std::ios::beg);

  this->m_fileStream.read(&buf[0], buf.size());

  for (unsigned int i = 0; i < 4; i++) {
    float min = interpretAs<float>(buf, 4 * i * 2);
    float max = interpretAs<float>(buf, 4 * (i * 2 + 1)) * (1 + FLT_EPSILON);
    DimVectorOut[i].setNumBins(m_nBins[i]);
    DimVectorOut[i].setMax(max);
    DimVectorOut[i].setMin(min);
  }
}

/*==================================================================================
Region: Functions in the following region are candidates for refactoring. Copied
from MD_FileHoraceReader
==================================================================================*/

/** Function provides seam on to access axillary functions ported from
   MD_FileHoraceReader.

*/
void LoadSQW::parseMetadata(const std::string &fileName) {
  if (m_fileStream.is_open())
    m_fileStream.close();
  m_fileStream.open(fileName.c_str(), std::ios::binary);

  if (!m_fileStream.is_open())
    throw(
        Kernel::Exception::FileError("Can not open input sqw file", fileName));

  std::vector<char> data_buffer;
  m_fileStream.seekg(m_dataPositions.if_sqw_start);
  data_buffer.resize(3 * 4);

  m_fileStream.read(&data_buffer[0], 2 * 4);
  this->m_nDims = interpretAs<uint32_t>(data_buffer);

  m_dataPositions.parse_sqw_main_header(m_fileStream);

  // go through all component headers and read them (or calculate their length)
  std::streamoff next_position = m_dataPositions.component_headers_starts[0];
  size_t nFiles = m_dataPositions.component_headers_starts.size();
  for (size_t i = 0; i < nFiles; i++) {
    m_dataPositions.component_headers_starts[i] = next_position;
    next_position =
        m_dataPositions.parse_component_header(m_fileStream, next_position);
  }
  m_dataPositions.detectors_start = next_position;
  // get detectors
  m_dataPositions.data_start = m_dataPositions.parse_sqw_detpar(
      m_fileStream, m_dataPositions.detectors_start);
  // calculate all other data fields locations;
  m_dataPositions.parse_data_locations(m_fileStream, m_dataPositions.data_start,
                                       m_nBins, m_nDataPoints);
}

void LoadSQW::readBoxSizes() {

  m_boxSizes.resize(m_dataPositions.mdImageSize);
  m_fileStream.seekg(m_dataPositions.n_cell_pix_start, std::ios::beg);
  m_fileStream.read((char *)(&m_boxSizes[0]),
                    m_dataPositions.mdImageSize * sizeof(uint64_t));
}

namespace LoadSQWHelper {

// auxiliary functions
/**Block 1:  Main_header: Parse SQW main data header
*@param dataStream -- the open file handler responsible for IO operations
**/
void dataPositions::parse_sqw_main_header(
    std::ifstream &dataStream) { // we do not need this header  at the moment ->
                                 // just need to calculated its length;

  std::vector<char> data_buffer(4 * 3);
  dataStream.read(&data_buffer[0], 4);

  unsigned int file_name_length = *((uint32_t *)(&data_buffer[0]));
  // skip main header file name
  dataStream.seekg(file_name_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_path_length = *((uint32_t *)(&data_buffer[0]));

  // skip main header file path
  dataStream.seekg(file_path_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_title = *((uint32_t *)(&data_buffer[0]));

  // skip ws title
  dataStream.seekg(file_title, std::ios_base::cur);

  // identify number of file headers, contributed into the dataset
  dataStream.read(&data_buffer[0], 4);
  unsigned int nFiles = *((uint32_t *)(&data_buffer[0]));

  /// allocate space for the component headers positions;
  this->component_headers_starts.assign(nFiles, 0);

  std::streamoff last_location = dataStream.tellg();
  if (last_location < 0)
    throw("IO error for input  file at start of component headers; Can not "
          "seek to last location");
  if (nFiles > 0) {
    this->component_headers_starts[0] = last_location;
  }
}
/**Block 2: Header: Parse header of single SPE file
*@param dataStream -- the open file handler responsible for IO operations
*@param start_location -- initial file position of the header within the binary
*file
*
*@returns: the file location of the first byte behind this header
*/
std::streamoff dataPositions::parse_component_header(
    std::ifstream &dataStream,
    std::streamoff start_location) { // we do not need this header  at the
                                     // moment -> just calculating its length;
                                     // or may be we do soon?
  std::vector<char> data_buffer(8);

  std::streamoff shift = start_location - dataStream.tellg();
  // move to specified location, which should be usually 0;
  dataStream.seekg(shift, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);

  unsigned int file_name_length = *((uint32_t *)(&data_buffer[0]));
  // skip component header file name
  dataStream.seekg(file_name_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_path_length = *((uint32_t *)(&data_buffer[0]));

  // skip component header file path
  dataStream.seekg(file_path_length, std::ios_base::cur);

  // move to by specified number of bytes, see Matlab header above;
  dataStream.seekg(4 * (7 + 3 * 4), std::ios_base::cur);

  // read number of energy bins;
  dataStream.read(&data_buffer[0], 4);
  unsigned int nEn_bins = *((uint32_t *)(&data_buffer[0]));
  // skip energy values;
  dataStream.seekg(4 * (nEn_bins), std::ios_base::cur);

  // skip offsets and conversions;
  dataStream.seekg(4 * (4 + 4 * 4 + 4), std::ios_base::cur);

  // get labels matrix size;
  dataStream.read(&data_buffer[0], 8);

  unsigned int nRows = *((uint32_t *)(&data_buffer[0]));
  unsigned int nCols = *((uint32_t *)(&data_buffer[4]));

  // skip labels
  dataStream.seekg(nRows * nCols, std::ios_base::cur);

  std::streamoff end_location = dataStream.tellg();
  return end_location;
}
/**Block 3: Detpar: parse positions of the contributed detectors. These
*detectors have to be the same for all contributing spe files
*@param dataStream -- the open file handler responsible for IO operations
*@param start_location -- initial file position of the detectors data within the
*binary file
*
*@returns: the file location of the first byte behind this header   */
std::streamoff
dataPositions::parse_sqw_detpar(std::ifstream &dataStream,
                                std::streamoff start_location) { //
  std::vector<char> data_buffer(8);

  std::streamoff shift = start_location - dataStream.tellg();
  // move to specified location, which should be usually 0;
  dataStream.seekg(shift, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_name_length = *((uint32_t *)(&data_buffer[0]));
  // skip component header file name
  dataStream.seekg(file_name_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_path_length = *((uint32_t *)(&data_buffer[0]));
  // skip component header file path
  dataStream.seekg(file_path_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int num_detectors = *((uint32_t *)(&data_buffer[0]));
  // skip detector information
  dataStream.seekg(num_detectors * 6 * 4, std::ios_base::cur);

  std::streamoff end_location = dataStream.tellg();
  return end_location;
}
/**Block 4: Data: parse positions of the data fields
*@param dataStream -- the open file handler responsible for IO operations
*@param data_start -- Initial position of the data block4 within the data file
@param nBins -- the vector of bin sizes for MD image
@param nDataPoints -- number of pixels (MD events) contributing to the image
*/
void dataPositions::parse_data_locations(std::ifstream &dataStream,
                                         std::streamoff data_start,
                                         std::vector<size_t> &nBins,
                                         uint64_t &nDataPoints) {
  std::vector<char> data_buffer(12);

  std::streamoff shift = data_start - dataStream.tellg();
  // move to specified location, which should be usually 0;
  dataStream.seekg(shift, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);

  unsigned int file_name_length = *((uint32_t *)(&data_buffer[0]));
  // skip dummy file name
  dataStream.seekg(file_name_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int file_path_length = *((uint32_t *)(&data_buffer[0]));

  // skip dummy file path
  dataStream.seekg(file_path_length, std::ios_base::cur);

  dataStream.read(&data_buffer[0], 4);
  unsigned int data_title_length = *((uint32_t *)(&data_buffer[0]));

  // skip data title
  dataStream.seekg(data_title_length, std::ios_base::cur);

  this->geom_start = dataStream.tellg();

  dataStream.seekg(4 * (3 + 3 + 4 + 16 + 4), std::ios_base::cur);

  // get label information and skip labels;
  dataStream.read(&data_buffer[0], 8);

  unsigned int n_labels = *((uint32_t *)(&data_buffer[0]));
  unsigned int labels_length = *((uint32_t *)(&data_buffer[4]));
  dataStream.seekg(n_labels * labels_length, std::ios_base::cur);

  this->npax_start = dataStream.tellg();

  dataStream.read(&data_buffer[0], 4);

  unsigned int npax = *((uint32_t *)(&data_buffer[0]));
  unsigned int niax = 4 - npax;
  if (niax != 0) {
    dataStream.seekg(3 * niax * 4, std::ios_base::cur);
  }
  if (npax != 0) {
    nBins.resize(npax);

    // skip projection axis
    dataStream.seekg(npax * 4, std::ios_base::cur);

    mdImageSize = 1;
    for (unsigned int i = 0; i < npax; i++) {
      dataStream.read(&data_buffer[0], 4);

      unsigned int nAxisPoints = *((uint32_t *)(&data_buffer[0]));
      nBins[i] = nAxisPoints - 1;
      mdImageSize *= nBins[i];
      dataStream.seekg(nAxisPoints * 4, std::ios_base::cur);
    }
    // skip display indexes;
    dataStream.seekg(npax * 4, std::ios_base::cur);
  }
  // signal start:
  this->s_start = dataStream.tellg();
  // and skip to errors
  dataStream.seekg(mdImageSize * 4, std::ios_base::cur);

  // error start:
  this->err_start = dataStream.tellg();
  dataStream.seekg(mdImageSize * 4, std::ios_base::cur);

  // dnd data file.  we do not support this?
  if (dataStream.eof()) {
    nDataPoints = 0;
    return;
    // throw(std::invalid_argument("DND Horace datasets are not supported by
    // Mantid"));
  }

  this->n_cell_pix_start = dataStream.tellg();
  // skip to the end of pixels;
  dataStream.seekg(mdImageSize * 8, std::ios_base::cur);

  if (dataStream.eof()) {
    nDataPoints = 0;
    return;
    // throw(std::invalid_argument("DND b+ Horace datasets are not supported by
    // Mantid"));
  }
  this->min_max_start = dataStream.tellg();
  // skip min-max start
  //[data.urange,count,ok,mess] = fread_catch(fid,[2,4],'float32'); if ~all(ok);
  // return; end;
  dataStream.seekg(8 * 4, std::ios_base::cur);

  if (dataStream.eof()) {
    nDataPoints = 0;
    return;
    // throw(std::invalid_argument("SQW a- Horace datasets are not supported by
    // Mantid"));
  }
  // skip redundant field and read nPix (number of data points)
  dataStream.read(&data_buffer[0], 12);

  nDataPoints = (size_t)(*((uint64_t *)(&data_buffer[4])));
  this->pix_start = dataStream.tellg();
}

/*==================================================================================
EndRegion:
==================================================================================*/
} // endNamespace LoadSQWHelper
}
}
