// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadNGEM.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BinaryStreamReader.h"

#include <sys/stat.h>

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadNGEM)

/**
 * @brief Byte swap a 64 bit word as the nGEM detector outputs in big endian
 * format. So must be swapped to have correct values on x86 and x64
 * architectures.
 *
 * @param word The 64 bit word to swap.
 * @return uint64_t The swapped word.
 */
uint64_t LoadNGEM::swapUint64(uint64_t word) {
  word = ((word << 8) & 0xFF00FF00FF00FF00ULL) |
         ((word >> 8) & 0x00FF00FF00FF00FFULL);
  word = ((word << 16) & 0xFFFF0000FFFF0000ULL) |
         ((word >> 16) & 0x0000FFFF0000FFFFULL);
  return (word << 32) | (word >> 32);
}

/**
 * @brief The confidence that a file can be loaded.
 *
 * @param descriptor A description of the file.
 * @return int The level of certainty that the file can be loaded with this
 * loader.
 */
int LoadNGEM::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() == ".edb") {
    return 90;
  } else {
    return 0;
  }
}

/**
 * @brief Initialisation of the algorithm, setting up the properties.
 */
void LoadNGEM::init() {
  // Filename property.
  const std::vector<std::string> extentions{".edb"};
  declareProperty(
      std::make_unique<Mantid::API::FileProperty>(
          "Filename", "", Mantid::API::FileProperty::Load, extentions),
      "The name of the nGEM file to load.");
  // Output workspace
  declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The output workspace");
}

/**
 * @brief Execute the algorithm.
 */
void LoadNGEM::exec() {
  const std::string filename = getPropertyValue("Filename");

  // Create file reader
  FILE *file = fopen(filename.c_str(), "rb");
  if (file == NULL) {
    g_log.error() << "Invalid input filename.\n";
    throw std::runtime_error("File could not be found.");
  }

  // Check that the file fits into 16 byte sections.
  struct stat fileStatus;
  if (fstat(fileno(file), &fileStatus) != 0 || fileStatus.st_size % 16 != 0) {
    g_log.error() << "Invalid file size.\n" << fileStatus.st_size << "\n";
    throw std::runtime_error("File size error.");
  }

  // int totalEvents = int(fileStatus.st_size / 16);
  // int *events = new int[totalEvents];

  EventUnion event, eventBigEndian;
  size_t loadStatus;

  int maxToF = -1;
  int minToF = 2147483647;
  const int binWidth(10);

  std::vector<DataObjects::EventList> histograms;
  histograms.resize(16384);

  while (!feof(file)) {
    // Load an event into the variable.
    loadStatus = fread(&eventBigEndian, sizeof(eventBigEndian), 1, file);

    // Check that the load was successful.
    if (loadStatus == 0) {
      break;
    } else if (loadStatus != 1) {
      g_log.error() << "Error while reading file.";
      throw std::runtime_error("Error while reading file.");
    }

    // Correct for the big endian format.
    correctForBigEndian(eventBigEndian, event);

    if (event.coincidence.check()) {
      int pixel = event.coincidence.getPixel();
      int tof =
          event.coincidence.timeOfFlight / 1000; // Convert to microseconds (us)

      if (tof > maxToF) {
        maxToF = tof;
      } else if (tof < minToF) {
        minToF = tof;
      }

      histograms[pixel].addEventQuickly(Mantid::Types::Event::TofEvent(tof));
      histograms[pixel].setDetectorID(pixel + 1);
    }
  }
  fclose(file);

  std::vector<double> xAxis;
  for (auto i = 0; i < (maxToF / binWidth); i++) {
    xAxis.push_back(i * binWidth);
  }

  m_dataWorkspace = DataObjects::create<DataObjects::EventWorkspace>(
      16384,
      Mantid::HistogramData::Histogram(Mantid::HistogramData::BinEdges(xAxis)));

  for (auto spectrumNo = 0u; spectrumNo < histograms.size(); ++spectrumNo) {
    m_dataWorkspace->getSpectrum(spectrumNo) = histograms[spectrumNo];
  }

  m_dataWorkspace->setAllX(HistogramData::BinEdges{xAxis});

  setProperty("OutputWorkspace", m_dataWorkspace);
}

/**
 * @brief Correct an event to be compatible with x64 and x86.
 *
 * @param bigEndian The big endian to be converted.
 * @param smallEndian The small endian to be "filled"
 */
void LoadNGEM::correctForBigEndian(const EventUnion &bigEndian,
                                   EventUnion &smallEndian) {
  smallEndian.splitWord.words[0] = swapUint64(bigEndian.splitWord.words[1]);
  smallEndian.splitWord.words[1] = swapUint64(bigEndian.splitWord.words[0]);
}

} // namespace DataHandling
} // namespace Mantid