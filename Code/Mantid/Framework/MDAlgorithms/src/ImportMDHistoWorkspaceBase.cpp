#include "MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

/**
Functor to compute the product of the set.
*/
struct Product : public std::unary_function<size_t, void> {
  Product() : result(1) {}
  size_t result;
  void operator()(size_t x) { result *= x; }
};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ImportMDHistoWorkspaceBase::ImportMDHistoWorkspaceBase() : m_bin_product(0) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ImportMDHistoWorkspaceBase::~ImportMDHistoWorkspaceBase() {}

//----------------------------------------------------------------------------------------------
/** Initalise generic importing properties.
 */
void ImportMDHistoWorkspaceBase::initGenericImportProps() {
  auto validator = boost::make_shared<CompositeValidator>();
  validator->add(boost::make_shared<BoundedValidator<int>>(1, 9));
  validator->add(boost::make_shared<MandatoryValidator<int>>());

  declareProperty(new PropertyWithValue<int>("Dimensionality", -1, validator,
                                             Direction::Input),
                  "Dimensionality of the data in the file.");

  declareProperty(new ArrayProperty<double>("Extents"),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension.");

  declareProperty(new ArrayProperty<int>("NumberOfBins"),
                  "Number of bin in each dimension.");

  declareProperty(new ArrayProperty<std::string>("Names"),
                  "A comma separated list of the name of each dimension.");

  declareProperty(new ArrayProperty<std::string>("Units"),
                  "A comma separated list of the units of each dimension.");

  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace reflecting the input text file.");
}

//----------------------------------------------------------------------------------------------
/** Create an empty output workspace from the generic properies. This gives a
new workspace with dimensions provided, but signal and
error arrays will not yet be set.
*/
MDHistoWorkspace_sptr ImportMDHistoWorkspaceBase::createEmptyOutputWorkspace() {
  // Fetch input properties
  size_t ndims;
  {
    int ndims_int = getProperty("Dimensionality");
    ndims = ndims_int;
  }
  std::vector<double> extents = getProperty("Extents");
  std::vector<int> nbins = getProperty("NumberOfBins");
  std::vector<std::string> names = getProperty("Names");
  std::vector<std::string> units = getProperty("Units");

  // Perform all validation on inputs
  if (extents.size() != ndims * 2)
    throw std::invalid_argument("You must specify twice as many extents "
                                "(min,max) as there are dimensions.");
  if (nbins.size() != ndims)
    throw std::invalid_argument(
        "You must specify as number of bins as there are dimensions.");
  if (names.size() != ndims)
    throw std::invalid_argument(
        "You must specify as many names as there are dimensions.");
  if (units.size() != ndims)
    throw std::invalid_argument(
        "You must specify as many units as there are dimensions.");

  // Fabricate new dimensions from inputs
  std::vector<MDHistoDimension_sptr> dimensions;
  for (size_t k = 0; k < ndims; ++k) {
    dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension(
        names[k], names[k], units[k], static_cast<coord_t>(extents[k * 2]),
        static_cast<coord_t>(extents[(k * 2) + 1]), nbins[k])));
  }

  // Calculate the total number of bins by multiplying across each dimension.
  Product answer = std::for_each(nbins.begin(), nbins.end(), Product());
  m_bin_product = answer.result;

  MDHistoWorkspace_sptr ws(new MDHistoWorkspace(dimensions));
  return ws;
}

} // namespace Mantid
} // namespace MDAlgorithms
