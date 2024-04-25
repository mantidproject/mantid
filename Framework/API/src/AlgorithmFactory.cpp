// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <sstream>

#include "MantidKernel/StringTokenizer.h"

namespace Mantid::API {
namespace {
/// static logger instance
Kernel::Logger g_log("AlgorithmFactory");
} // namespace

AlgorithmFactoryImpl::AlgorithmFactoryImpl() : Kernel::DynamicFactory<Algorithm>(), m_vmap(), m_amap() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
  g_log.debug() << "Algorithm Factory created.\n";
}

AlgorithmFactoryImpl::~AlgorithmFactoryImpl() = default;

/** Creates an instance of an algorithm
 * @param name :: the name of the Algorithm to create
 * @param version :: the version of the algorithm to create
 * @returns a shared pointer to the created algorithm
 */
std::shared_ptr<Algorithm> AlgorithmFactoryImpl::create(const std::string &name, const int &version) const {
  int local_version = version;
  // Version not supplied
  if (version == -1) {
    local_version = highestVersion(name); // throws if not found
  }

  // Try create from given name
  try {
    return this->createAlgorithm(name, local_version);
  } catch (Kernel::Exception::NotFoundError &) {
  }

  // Fallback, name might be an alias
  // Try get real name and create from that instead
  if (const auto realNameAndVersion = getRealNameFromAlias(name)) {
    // Try create algorithm again with real name
    try {
      auto alg = this->createAlgorithm(realNameAndVersion->first, local_version);
      alg->calledByAlias = true;
      return alg;
    } catch (Kernel::Exception::NotFoundError &) {
      // Get highest registered version
      const auto hVersion = highestVersion(realNameAndVersion->first); // Throws if not found

      // The version registered does not match version supplied
      g_log.error() << "algorithm " << name << " version " << version << " is not registered \n";
      g_log.error() << "the latest registered version is " << hVersion << '\n';
      throw std::runtime_error("algorithm not registered " + createName(name, local_version));
    }
  } else {
    throw std::runtime_error("algorithm not registered " + name);
  }
}

/**
 * Override the unsubscribe method so that it knows how algorithm names are
 * encoded in the factory
 * @param algorithmName :: The name of the algorithm to unsubscribe
 * @param version :: The version number of the algorithm to unsubscribe
 */
void AlgorithmFactoryImpl::unsubscribe(const std::string &algorithmName, const int version) {
  std::string key = this->createName(algorithmName, version);
  try {
    Kernel::DynamicFactory<Algorithm>::unsubscribe(key);
    // Update version map accordingly
    auto it = m_vmap.find(algorithmName);
    if (it != m_vmap.end()) {
      int highest_version = it->second;
      if (highest_version > 1 && version == highest_version) // Decrement the highest version
      {
        it->second -= 1;
      } else
        m_vmap.erase(algorithmName);
    }
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.warning() << "Error unsubscribing algorithm " << algorithmName << " version " << version
                    << ". Nothing registered with this name and version.";
  }
}

/**
 * Does an algorithm of the given name and version exist already
 * @param algorithmName :: The name of the algorithm
 * @param version :: The version number. -1 checks whether anything exists
 * @returns True if a matching registration is found
 */
bool AlgorithmFactoryImpl::exists(const std::string &algorithmName, const int version) {
  if (version == -1) // Find anything
  {
    return (m_vmap.find(algorithmName) != m_vmap.end());
  } else {
    std::string key = this->createName(algorithmName, version);
    return Kernel::DynamicFactory<Algorithm>::exists(key);
  }
}

/** Creates a mangled name for interal storage
 * @param name :: the name of the Algrorithm
 * @param version :: the version of the algroithm
 * @returns a mangled name string
 */
std::string AlgorithmFactoryImpl::createName(const std::string &name, const int &version) const {
  std::ostringstream oss;
  oss << name << "|" << version;
  return (oss.str());
}

/** Decodes a mangled name for interal storage
 * @param mangledName :: the mangled name of the Algrorithm
 * @returns a pair of the name and version
 */
std::pair<std::string, int> AlgorithmFactoryImpl::decodeName(const std::string &mangledName) const {
  std::string::size_type seperatorPosition = mangledName.find('|');
  if (seperatorPosition == std::string::npos) {
    throw std::invalid_argument("Cannot decode a Name string without a \"|\" (bar) character ");
  }
  std::string name = mangledName.substr(0, seperatorPosition);
  int version;
  std::istringstream ss(mangledName.substr(seperatorPosition + 1));
  ss >> version;

  g_log.debug() << "mangled string:" << mangledName << " name:" << name << " version:" << version << '\n';
  return std::pair<std::string, int>(name, version);
}

/** Return the keys used for identifying algorithms. This includes those
 * within the Factory itself and any cleanly constructed algorithms stored
 * here. Hidden algorithms are excluded.
 * @returns The strings used to identify individual algorithms
 */
const std::vector<std::string> AlgorithmFactoryImpl::getKeys() const {
  /* We have a separate method rather than just a default argument value
     to the getKeys(bool) methods so as to avoid an intel compiler warning. */

  // Just call the 'other' getKeys method with the flag set to false
  return getKeys(false);
}

/**
 * Return the keys used for identifying algorithms. This includes those within
 * the Factory itself and
 * any cleanly constructed algorithms stored here.
 * @param includeHidden true includes the hidden algorithm names and is
 * faster, the default is false
 * @returns The strings used to identify individual algorithms
 */
const std::vector<std::string> AlgorithmFactoryImpl::getKeys(bool includeHidden) const {
  // Start with those subscribed with the factory and add the cleanly
  // constructed algorithm keys
  std::vector<std::string> names = Kernel::DynamicFactory<Algorithm>::getKeys();

  if (includeHidden) {
    return names;
  } else {
    // hidden categories
    std::unordered_set<std::string> hiddenCategories;
    fillHiddenCategories(&hiddenCategories);

    // strip out any algorithms names where all of the categories are hidden
    std::vector<std::string> validNames;
    std::vector<std::string>::const_iterator itr_end = names.end();
    for (std::vector<std::string>::const_iterator itr = names.begin(); itr != itr_end; ++itr) {
      std::string name = *itr;
      // check the categories
      std::pair<std::string, int> namePair = decodeName(name);
      std::shared_ptr<IAlgorithm> alg = create(namePair.first, namePair.second);
      std::vector<std::string> categories = alg->categories();
      bool toBeRemoved = true;

      // for each category
      std::vector<std::string>::const_iterator itCategoriesEnd = categories.end();
      for (std::vector<std::string>::const_iterator itCategories = categories.begin(); itCategories != itCategoriesEnd;
           ++itCategories) {
        // if the entry is not in the set of hidden categories
        if (hiddenCategories.find(*itCategories) == hiddenCategories.end()) {
          toBeRemoved = false;
        }
      }

      if (!toBeRemoved) {
        // just mark them to be removed as we are iterating around the vector
        // at the moment
        validNames.emplace_back(name);
      }
    }
    return validNames;
  }
}

/**
 * @param alias The name of the algorithm to look up in the alias map
 * @return Real name of algorithm if found
 */
std::optional<std::pair<std::string, int>>
AlgorithmFactoryImpl::getRealNameFromAlias(const std::string &alias) const noexcept {
  auto a_it = m_amap.find(alias);
  if (a_it == m_amap.end())
    return std::nullopt;
  else
    return a_it->second;
}

/**
 * @param algorithmName The name of an algorithm registered with the factory
 * @return An integer corresponding to the highest version registered
 * @throw std::invalid_argument if the algorithm cannot be found
 */
int AlgorithmFactoryImpl::highestVersion(const std::string &algorithmName) const {
  auto viter = m_vmap.find(algorithmName);
  if (viter != m_vmap.end())
    return viter->second;
  else {
    // Fall back, algorithmName might be an alias
    // Check alias map, then find version from real name
    if (const auto realNameAndVersion = getRealNameFromAlias(algorithmName)) {
      return realNameAndVersion->second;
    } else {
      throw std::runtime_error("AlgorithmFactory::highestVersion() - Unknown algorithm '" + algorithmName + "'");
    }
  }
}

/**
 * Return the categories of the algorithms. This includes those within the
 * Factory itself and
 * any cleanly constructed algorithms stored here.
 * @returns The map of the categories, together with a true false value
 * difining if they are hidden
 */
const std::map<std::string, bool> AlgorithmFactoryImpl::getCategoriesWithState() const {
  std::map<std::string, bool> resultCategories;

  // hidden categories - empty initially
  std::unordered_set<std::string> hiddenCategories;
  fillHiddenCategories(&hiddenCategories);

  // get all of the algorithm keys, including the hidden ones for speed
  // purposes we will filter later if required
  std::vector<std::string> names = getKeys(true);

  std::vector<std::string>::const_iterator itr_end = names.end();
  // for each algorithm
  for (std::vector<std::string>::const_iterator itr = names.begin(); itr != itr_end; ++itr) {
    std::string name = *itr;
    // decode the name and create an instance
    std::pair<std::string, int> namePair = decodeName(name);
    std::shared_ptr<IAlgorithm> alg = create(namePair.first, namePair.second);
    // extract out the categories
    std::vector<std::string> categories = alg->categories();

    // for each category of the algorithm
    std::vector<std::string>::const_iterator itCategoriesEnd = categories.end();
    for (std::vector<std::string>::const_iterator itCategories = categories.begin(); itCategories != itCategoriesEnd;
         ++itCategories) {
      bool isHidden = true;
      // check if the category is hidden
      if (hiddenCategories.find(*itCategories) == hiddenCategories.end()) {
        isHidden = false;
      }
      resultCategories[*itCategories] = isHidden;
    }
  }
  return resultCategories;
}

/**
 * Return the categories of the algorithms. This includes those within the
 * Factory itself and
 * any cleanly constructed algorithms stored here.
 * @param includeHidden true includes the hidden algorithm names and is
 * faster, the default is false
 * @returns The category strings
 */
const std::unordered_set<std::string> AlgorithmFactoryImpl::getCategories(bool includeHidden) const {
  std::unordered_set<std::string> validCategories;

  // get all of the information we need
  auto categoryMap = getCategoriesWithState();

  // iterate around the map
  for (auto const &category : categoryMap) {
    bool isHidden = (category).second;
    if (includeHidden || (!isHidden)) {
      validCategories.insert((category).first);
    }
  }

  return validCategories;
}

/**
 * Get a list of descriptor objects used to order the algorithms in the stored
 * map
 * where an algorithm has multiple categories it will be represented using
 * multiple descriptors.
 * @param includeHidden true includes the hidden algorithm names and is
 * faster, the default is false
 * @param includeAliases true includes alias names of algorithms which aren't
 * empty to the vector of descriptors, the default is true
 * @returns A vector of descriptor objects
 */
std::vector<AlgorithmDescriptor> AlgorithmFactoryImpl::getDescriptors(bool includeHidden, bool includeAliases) const {
  // algorithm names
  auto sv = getKeys(true);

  // hidden categories
  std::unordered_set<std::string> hiddenCategories;
  if (!includeHidden) {
    fillHiddenCategories(&hiddenCategories);
  }

  // results vector
  std::vector<AlgorithmDescriptor> res;

  for (const auto &s : sv) {
    if (s.empty())
      continue;
    AlgorithmDescriptor desc;
    size_t i = s.find('|');
    if (i == std::string::npos) {
      desc.name = s;
      desc.version = 1;
    } else if (i > 0) {
      desc.name = s.substr(0, i);
      std::string vers = s.substr(i + 1);
      desc.version = vers.empty() ? 1 : std::stoi(vers);
    } else
      continue;

    std::shared_ptr<IAlgorithm> alg = create(desc.name, desc.version);
    auto categories = alg->categories();
    desc.alias = alg->alias();

    // For each category
    auto itCategoriesEnd = categories.end();
    for (auto itCategories = categories.begin(); itCategories != itCategoriesEnd; ++itCategories) {
      desc.category = *itCategories;

      // Let's check if this category or any of its parents are hidden.
      bool categoryIsHidden = false;

      // Split the category into its components.
      std::vector<std::string> categoryLayers;
      boost::split(categoryLayers, desc.category, boost::is_any_of("\\"));

      // Traverse each parent category, working our way from the top down.
      std::string currentLayer;
      for (auto &categoryLayer : categoryLayers) {
        currentLayer.append(categoryLayer);

        if (hiddenCategories.find(currentLayer) != hiddenCategories.end()) {
          // Category is hidden, no need to check any others.
          categoryIsHidden = true;
          break;
        }

        // Add a separator in case we're going down another layer.
        currentLayer.append("\\");
      }

      if (!categoryIsHidden) {
        res.emplace_back(desc);
        // Add alias to results if included
        if (!desc.alias.empty() && includeAliases) {
          // Avoid adding lowercase aliases to the algorithm tab
          std::string lowerCaseName = desc.name;
          std::transform(desc.name.cbegin(), desc.name.cend(), lowerCaseName.begin(),
                         [](unsigned char c) { return std::tolower(c); });
          if (lowerCaseName != desc.alias) {
            res.emplace_back(AlgorithmDescriptor{desc.alias, desc.version, desc.category, ""});
          }
        }
      }
    }
  }
  return res;
}

void AlgorithmFactoryImpl::fillHiddenCategories(std::unordered_set<std::string> *categorySet) const {
  std::string categoryString = Kernel::ConfigService::Instance().getString("algorithms.categories.hidden");
  Mantid::Kernel::StringTokenizer tokenizer(categoryString, ";",
                                            Mantid::Kernel::StringTokenizer::TOK_TRIM |
                                                Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  std::copy(tokenizer.begin(), tokenizer.end(), std::inserter(*categorySet, categorySet->end()));
}

/** Extract the name of an algorithm
 * @param alg :: the Algrorithm to use
 * @returns the name of the algroithm
 */
const std::string AlgorithmFactoryImpl::extractAlgName(const std::shared_ptr<IAlgorithm> &alg) const {
  return alg->name();
}

/** Extract the version of an algorithm
 * @param alg :: the Algrorithm to use
 * @returns the version of the algroithm
 */
int AlgorithmFactoryImpl::extractAlgVersion(const std::shared_ptr<IAlgorithm> &alg) const { return alg->version(); }

/** Extract the alias of an algorithm
 * @param alg :: the Algorithm to use
 * @returns the alias of the algorithm
 */
const std::string AlgorithmFactoryImpl::extractAlgAlias(const std::shared_ptr<IAlgorithm> &alg) const {
  return alg->alias();
}

/**
 * Create a shared pointer to an algorithm object with the given name and
 * version. If the algorithm is one registered with a clean pointer rather
 * than an instantiator then a clone is returned.
 * @param name :: Algorithm name
 * @param version :: Algorithm version
 * @returns A shared pointer to the algorithm object
 */
std::shared_ptr<Algorithm> AlgorithmFactoryImpl::createAlgorithm(const std::string &name, const int version) const {
  return Kernel::DynamicFactory<Algorithm>::create(createName(name, version));
}

} // namespace Mantid::API
