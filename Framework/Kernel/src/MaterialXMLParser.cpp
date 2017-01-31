#include "MantidKernel/MaterialXMLParser.h"
#include "MantidKernel/MaterialBuilder.h"

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/SAX/SAXException.h"

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <type_traits>
#include <unordered_map>

namespace Mantid {
namespace Kernel {

// -----------------------------------------------------------------------------
// Anonymous methods
// -----------------------------------------------------------------------------
namespace {

/*
 * Define the list of known attributes along with a map to the relevant
 * MaterialBuilder method to call. This avoids a long list of if/else
 * statements when parsing the xml.
 *
 * The mapping uses a template type, TypedBuilderHandle, to store the method
 * on the MaterialBuilder that should be called and the template type indicates
 * the argument type of the method call. The struct automatically extracts
 * the expected value type from the given string based on the template type.
 */

// Known attributes
const char *ID_ATT = "id";
const char *FORMULA_ATT = "formula";
const char *ATOMNUM_ATT = "atomicnumber";
const char *MASSNUM_ATT = "massnumber";
const char *NDENSITY_ATT = "numberdensity";
const char *ZPARAM_ATT = "zparameter";
const char *CELLVOL_ATT = "unitcellvol";
const char *MASSDENS_ATT = "massdensity";
const char *TOTSC_ATT = "totalscatterxsec";
const char *COHSC_ATT = "cohscatterxsec";
const char *INCOHSC_ATT = "incohscatterxsec";
const char *ABSORB_ATT = "absorptionxsec";

// Base type to put in a hash
struct BuilderHandle {
  virtual void operator()(MaterialBuilder &builder,
                          const std::string &value) const = 0;
};
typedef std::unique_ptr<BuilderHandle> BuilderHandle_uptr;

// Pointer to member function on MaterialBuilder
template <typename ArgType>
using BuilderMethod = MaterialBuilder &(MaterialBuilder::*)(ArgType);

// Template type where ArgType indicates the expected argument type including
// const & ref qualifiers
template <typename ArgType>
struct TypedBuilderHandle final : public BuilderHandle {
  // Remove const/reference qualifiers from ArgType
  typedef typename std::remove_const<
      typename std::remove_reference<ArgType>::type>::type ValueType;

  explicit TypedBuilderHandle(BuilderMethod<ArgType> m)
      : BuilderHandle(), m_method(m) {}

  void operator()(MaterialBuilder &builder,
                  const std::string &value) const override {
    auto typedVal = boost::lexical_cast<ValueType>(value);
    (builder.*m_method)(typedVal);
  }

private:
  BuilderMethod<ArgType> m_method;
};

typedef std::unordered_map<std::string, BuilderHandle_uptr> Handlers;

// Insert a handle into the given map
template <typename ArgType>
void insertHandle(Handlers *hash, const std::string &name,
                  BuilderMethod<ArgType> m) {
  hash->insert(std::make_pair(
      name, BuilderHandle_uptr(new TypedBuilderHandle<ArgType>(m))));
}

// Find the appropriate handler for a given attribute
const BuilderHandle &findHandle(const std::string &name) {
  static Handlers handles;
  if (handles.empty()) {
    insertHandle(&handles, ID_ATT, &MaterialBuilder::setName);
    insertHandle(&handles, FORMULA_ATT, &MaterialBuilder::setFormula);
    insertHandle(&handles, ATOMNUM_ATT, &MaterialBuilder::setAtomicNumber);
    insertHandle(&handles, MASSNUM_ATT, &MaterialBuilder::setMassNumber);
    insertHandle(&handles, NDENSITY_ATT, &MaterialBuilder::setNumberDensity);
    insertHandle(&handles, ZPARAM_ATT, &MaterialBuilder::setZParameter);
    insertHandle(&handles, CELLVOL_ATT, &MaterialBuilder::setUnitCellVolume);
    insertHandle(&handles, MASSDENS_ATT, &MaterialBuilder::setMassDensity);
    insertHandle(&handles, TOTSC_ATT,
                 &MaterialBuilder::setTotalScatterXSection);
    insertHandle(&handles, COHSC_ATT, &MaterialBuilder::setCoherentXSection);
    insertHandle(&handles, INCOHSC_ATT,
                 &MaterialBuilder::setIncoherentXSection);
    insertHandle(&handles, ABSORB_ATT, &MaterialBuilder::setAbsorptionXSection);
  }
  auto iter = handles.find(name);
  if (iter != handles.end())
    return *(iter->second);
  else
    throw std::runtime_error("Unknown material attribute '" + name + "'");
}

/**
  * Set a value on the builder base on the attribute name and the defined
  * member function
  * @param builder A pointer to the builder to update
  * @param attr The attribute name
  * @param value The value in the attribute
  */
void addToBuilder(MaterialBuilder *builder, const std::string &attr,
                  const std::string &value) {
  // Find the appropriate member function on the builder and set the value
  // We need 3 maps for the 3 allowable value types for the builder member
  // functions
  const auto &setter = findHandle(attr);
  setter(*builder, value);
}
}

// -----------------------------------------------------------------------------
// Public methods
// -----------------------------------------------------------------------------

/**
 * Takes a stream that is assumed to contain a single complete material
 * definition,
 * reads the definition and produces a new Material object. If many definitions
 * are present then the first one is read
 * @param istr A reference to a stream
 * @return A new Material object
 */
Material MaterialXMLParser::parse(std::istream &istr) const {
  using namespace Poco::XML;
  typedef AutoPtr<Document> DocumentPtr;

  InputSource src(istr);
  DOMParser parser;
  // Do not use auto here or anywhereas the Poco API returns raw pointers
  // but in some circumstances requires AutoPtrs to manage the memory
  DocumentPtr doc;
  try {
    doc = parser.parse(&src);
  } catch (SAXParseException &exc) {
    std::ostringstream os;
    os << "MaterialXMLReader::read() - Error parsing stream as XML: "
       << exc.what();
    throw std::invalid_argument(os.str());
  }

  Element *rootElement = doc->documentElement();

  // Iterating is apparently much faster than getElementsByTagName
  NodeIterator nodeIter(rootElement, NodeFilter::SHOW_ELEMENT);
  Node *node = nodeIter.nextNode();
  Material matr;
  bool found(false);
  while (node) {
    if (node->nodeName() == MATERIAL_TAG) {
      matr = parse(static_cast<Element *>(node));
      found = true;
      break;
    }
    node = nodeIter.nextNode();
  }
  if (!found) {
    throw std::invalid_argument(
        "MaterialXMLReader::read() - No material tags found.");
  }
  return matr;
}

/**
 * Takes a pointer to an XML node that is assumed to point at a "material"
 * tag.
 * It reads the definition and produces a new Material object.
 * @param element A pointer to an Element node that is a "material" tag
 * @return A new Material object
 */
Material MaterialXMLParser::parse(Poco::XML::Element *element) const {
  using namespace Poco::XML;
  typedef AutoPtr<NamedNodeMap> NamedNodeMapPtr;
  NamedNodeMapPtr attrs = element->attributes();
  const auto id = attrs->getNamedItem(ID_ATT);
  if (!id || id->nodeValue().empty()) {
    throw std::invalid_argument("MaterialXMLReader::read() - No 'id' tag found "
                                "or emptry string provided.");
  }
  attrs->removeNamedItem(ID_ATT);

  MaterialBuilder builder;
  builder.setName(id->nodeValue());
  const auto nattrs = attrs->length();
  for (unsigned long i = 0; i < nattrs; ++i) {
    Node *node = attrs->item(i);
    addToBuilder(&builder, node->nodeName(), node->nodeValue());
  }
  return builder.build();
}

} // namespace Kernel
} // namespace Mantid
