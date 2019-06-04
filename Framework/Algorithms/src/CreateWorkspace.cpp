// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateWorkspace.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/InvisibleProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using namespace Indexing;

DECLARE_ALGORITHM(CreateWorkspace)

/// Init function
void CreateWorkspace::init() {

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.emplace_back("SpectraNumber");
  unitOptions.emplace_back("Text");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name to be given to the created workspace.");

  auto required = boost::make_shared<MandatoryValidator<std::vector<double>>>();
  declareProperty(std::make_unique<ArrayProperty<double>>("DataX", required),
                  "X-axis data values for workspace.");
  declareProperty(std::make_unique<ArrayProperty<double>>("DataY", required),
                  "Y-axis data values for workspace (measures).");
  declareProperty(std::make_unique<ArrayProperty<double>>("DataE"),
                  "Error values for workspace.");
  declareProperty(std::make_unique<PropertyWithValue<int>>("NSpec", 1),
                  "Number of spectra to divide data into.");
  declareProperty("UnitX", "", "The unit to assign to the XAxis");

  declareProperty("VerticalAxisUnit", "SpectraNumber",
                  boost::make_shared<StringListValidator>(unitOptions),
                  "The unit to assign to the second Axis (leave blank for "
                  "default Spectra number)");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("VerticalAxisValues"),
                  "Values for the VerticalAxis.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("Distribution", false),
      "Whether OutputWorkspace should be marked as a distribution.");
  declareProperty("YUnitLabel", "", "Label for Y Axis");

  declareProperty("WorkspaceTitle", "", "Title for Workspace");

  declareProperty(std::make_unique<WorkspaceProperty<>>("ParentWorkspace", "",
                                                   Direction::Input,
                                                   PropertyMode::Optional),
                  "Name of a parent workspace.");
  declareProperty(std::make_unique<ArrayProperty<double>>("Dx"),
                  "X error values for workspace (optional).");
  std::vector<std::string> propOptions{
      Parallel::toString(Parallel::StorageMode::Cloned),
      Parallel::toString(Parallel::StorageMode::Distributed),
      Parallel::toString(Parallel::StorageMode::MasterOnly)};
  declareProperty(
      "ParallelStorageMode", Parallel::toString(Parallel::StorageMode::Cloned),
      boost::make_shared<StringListValidator>(propOptions),
      "The parallel storage mode of the output workspace for MPI builds");
  setPropertySettings("ParallelStorageMode",
                      std::make_unique<InvisibleProperty>());
}

/// Input validation
std::map<std::string, std::string> CreateWorkspace::validateInputs() {
  std::map<std::string, std::string> issues;

  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  if (vUnit == "SpectraNumber" && !vAxis.empty())
    issues["VerticalAxisValues"] =
        "Axis values cannot be provided when using a spectra axis";

  return issues;
}

/// Exec function
void CreateWorkspace::exec() {
  // Contortions to get at the vector in the property without copying it
  const Property *const dataXprop = getProperty("DataX");
  const Property *const dataYprop = getProperty("DataY");
  const Property *const dataEprop = getProperty("DataE");
  const Property *const errorDxprop = getProperty("Dx");

  const ArrayProperty<double> *pCheck = nullptr;

  pCheck = dynamic_cast<const ArrayProperty<double> *>(dataXprop);
  if (!pCheck)
    throw std::invalid_argument("DataX cannot be casted to a double vector");
  const std::vector<double> &dataX = *pCheck;

  pCheck = dynamic_cast<const ArrayProperty<double> *>(dataYprop);
  if (!pCheck)
    throw std::invalid_argument("DataY cannot be casted to a double vector");
  const std::vector<double> &dataY = *pCheck;

  pCheck = dynamic_cast<const ArrayProperty<double> *>(dataEprop);
  if (!pCheck)
    throw std::invalid_argument("DataE cannot be casted to a double vector");
  const std::vector<double> &dataE = *pCheck;

  pCheck = dynamic_cast<const ArrayProperty<double> *>(errorDxprop);
  if (!pCheck)
    throw std::invalid_argument("Dx cannot be casted to a double vector");
  const std::vector<double> &dX = *pCheck;

  const int nSpec = getProperty("NSpec");
  const std::string xUnit = getProperty("UnitX");
  const std::string vUnit = getProperty("VerticalAxisUnit");
  const std::vector<std::string> vAxis = getProperty("VerticalAxisValues");

  // Verify the size of the vertical axis.
  const int vAxisSize = static_cast<int>(vAxis.size());
  if (vUnit != "SpectraNumber") {
    // In the case of numerical axis, the vertical axis can represent either
    // point data or bin edges.
    if ((vUnit == "Text" && vAxisSize != nSpec) ||
        (vAxisSize != nSpec && vAxisSize != nSpec + 1)) {
      throw std::invalid_argument("The number of vertical axis values doesn't "
                                  "match the number of histograms.");
    }
  }

  // Verify length of vectors makes sense with NSpec
  if ((dataY.size() % nSpec) != 0) {
    throw std::invalid_argument("Length of DataY must be divisible by NSpec");
  }
  const std::size_t ySize = dataY.size() / nSpec;

  // Check whether the X values provided are to be re-used for (are common to)
  // every spectrum
  const bool commonX(dataX.size() == ySize || dataX.size() == ySize + 1);

  std::size_t xSize{dataX.size()};
  HistogramBuilder histogramBuilder;
  if (commonX) {
    histogramBuilder.setX(dataX);
  } else {
    if (xSize % nSpec != 0) {
      throw std::invalid_argument("Length of DataX must be divisible by NSpec");
    }
    xSize /= nSpec;
    histogramBuilder.setX(xSize);
  }
  histogramBuilder.setY(ySize);

  if (!dX.empty()) {
    if (dX.size() != dataY.size())
      throw std::runtime_error("Dx must have the same size as DataY");
    histogramBuilder.setDx(ySize);
  }

  histogramBuilder.setDistribution(getProperty("Distribution"));
  auto histogram = histogramBuilder.build();

  const bool dataE_provided = !dataE.empty();
  if (dataE_provided && dataY.size() != dataE.size()) {
    throw std::runtime_error(
        "DataE (if provided) must be the same size as DataY");
  }

  // Create the OutputWorkspace
  MatrixWorkspace_const_sptr parentWS = getProperty("ParentWorkspace");
  MatrixWorkspace_sptr outputWS;
  if (parentWS) {
    outputWS = create<HistoWorkspace>(*parentWS, nSpec, histogram);
  } else {
    auto storageMode = Parallel::fromString(getProperty("ParallelStorageMode"));
    IndexInfo indexInfo(nSpec, storageMode, communicator());
    outputWS = create<Workspace2D>(indexInfo, histogram);
  }

  Progress progress(this, 0.0, 1.0, nSpec);
  const auto &indexInfo = outputWS->indexInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < nSpec; i++) {
    PARALLEL_START_INTERUPT_REGION

    // In an MPI run the global index i is not necessarily on this rank, i.e.,
    // there might not be a corrsponding workspace index.
    const auto localIndices =
        indexInfo.makeIndexSet({static_cast<GlobalSpectrumIndex>(i)});
    if (localIndices.empty())
      continue;

    const std::vector<double>::difference_type xStart = i * xSize;
    const std::vector<double>::difference_type xEnd = xStart + xSize;
    const std::vector<double>::difference_type yStart = i * ySize;
    const std::vector<double>::difference_type yEnd = yStart + ySize;
    auto local_i = localIndices[0];

    // Just set the pointer if common X bins. Otherwise, copy in the right chunk
    // (as we do for Y).
    if (!commonX)
      outputWS->mutableX(local_i).assign(dataX.begin() + xStart,
                                         dataX.begin() + xEnd);

    outputWS->mutableY(local_i).assign(dataY.begin() + yStart,
                                       dataY.begin() + yEnd);

    if (dataE_provided)
      outputWS->mutableE(local_i).assign(dataE.begin() + yStart,
                                         dataE.begin() + yEnd);

    if (!dX.empty())
      outputWS->mutableDx(local_i).assign(dX.begin() + yStart,
                                          dX.begin() + yEnd);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Set the Unit of the X Axis
  try {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(xUnit);
  } catch (Exception::NotFoundError &) {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    Unit_sptr unit = outputWS->getAxis(0)->unit();
    boost::shared_ptr<Units::Label> label =
        boost::dynamic_pointer_cast<Units::Label>(unit);
    label->setLabel(xUnit, xUnit);
  }

  // Populate the VerticalAxis. A spectra one is there by default with a 1->N
  // mapping
  if (vUnit != "SpectraNumber") {
    if (vUnit == "Text") {
      auto const newAxis = new TextAxis(vAxis.size());
      outputWS->replaceAxis(1, newAxis);
      for (size_t i = 0; i < vAxis.size(); i++) {
        newAxis->setLabel(i, vAxis[i]);
      }
    } else {
      NumericAxis *newAxis(nullptr);
      if (vAxisSize == nSpec)
        newAxis = new NumericAxis(vAxisSize); // treat as points
      else if (vAxisSize == nSpec + 1)
        newAxis = new BinEdgeAxis(vAxisSize); // treat as bin edges
      else
        throw std::range_error("Invalid vertical axis length. It must be the "
                               "same length as NSpec or 1 longer.");

      newAxis->unit() = UnitFactory::Instance().create(vUnit);
      outputWS->replaceAxis(1, newAxis);
      for (size_t i = 0; i < vAxis.size(); i++) {
        try {
          newAxis->setValue(i,
                            boost::lexical_cast<double, std::string>(vAxis[i]));
        } catch (boost::bad_lexical_cast &) {
          throw std::invalid_argument("CreateWorkspace - YAxisValues property "
                                      "could not be converted to a double.");
        }
      }
    }
  }

  // Set Y Unit label
  if (!parentWS || !getPropertyValue("YUnitLabel").empty()) {
    outputWS->setYUnitLabel(getProperty("YUnitLabel"));
  }

  // Set Workspace Title
  if (!parentWS || !getPropertyValue("WorkspaceTitle").empty()) {
    outputWS->setTitle(getProperty("WorkspaceTitle"));
  }

  // Set OutputWorkspace property
  setProperty("OutputWorkspace", outputWS);
}

Parallel::ExecutionMode CreateWorkspace::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  const auto storageMode =
      Parallel::fromString(getProperty("ParallelStorageMode"));
  if (!storageModes.empty())
    if (storageModes.begin()->second != storageMode)
      throw std::invalid_argument("Input workspace storage mode differs from "
                                  "requested output workspace storage mode.");
  return Parallel::getCorrespondingExecutionMode(storageMode);
}

} // namespace Algorithms
} // namespace Mantid
