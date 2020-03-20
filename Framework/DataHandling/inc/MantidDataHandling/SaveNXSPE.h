// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Repository : https://github.com/mantidproject/mantid
//
//
** /

    class DLLExport SaveNXSPE : public API::Algorithm {
public:
  /// Constructor
  SaveNXSPE();
  const std::string name() const override { return "SaveNXSPE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a MatrixWorkspace to a file in the NXSPE format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNXSPE", "SaveSPE"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(DataHandling\Nexus;DataHandling\SPE;Inelastic\DataHandling)";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  // Some constants to be written for masked values.
  /// Value for data if pixel is masked
  static const double MASK_FLAG;
  /// Value for error if pixel is masked
  static const double MASK_ERROR;
  /// file format version
  static const std::string NXSPE_VER;
  /// The size in bytes of a chunk to accumulate to write to the file at once
  static const size_t MAX_CHUNK_SIZE;
};

} // namespace DataHandling
} // namespace Mantid
