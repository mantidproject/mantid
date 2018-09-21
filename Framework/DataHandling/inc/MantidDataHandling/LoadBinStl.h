#ifndef MANTID_DATAHANDLING_LOADBINSTL_H_
#define MANTID_DATAHANDLING_LOADBINSTL_H_
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <MantidKernel/V3D.h>
namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinStl : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override;
  /// Algorithm's version
  int version() const override;
  /// Algorithm's category for identification
  const std::string category() const override;

  const std::string summary() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const {return 0;};
  /// Returns a value indicating whether or not loader wants to load multiple
  /// files into a single workspace
  bool loadMutipleAsOne() { return false; }

private:
  void init() override;
  void exec() override;
  void readStl(std::string filename);
  void readTriangle(Kernel::BinaryStreamReader);
  std::vector<uint16_t> m_triangle;
  std::vector<Kernel::V3D> m_verticies;
};
} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBINSTL_H_ */