#include "MantidVatesAPI/Normalization.h"
#include "MantidAPI/IMDNode.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"

namespace Mantid {
namespace VATES {

/**
Choose and return the pointer to the member function for IMDNode to perform the
requested normalisation.
This is used for visualisation of IMDEventWorkspaces.
@param normalizationOption : Visual Normalization option desired
@param ws : workspace to fetch defaults from if needed
@param hasMask : true if the workspace has a mask
@return member function to use on IMDNodes
*/
NormFuncIMDNodePtr
makeMDEventNormalizationFunction(VisualNormalization normalizationOption,
                                 Mantid::API::IMDEventWorkspace const *const ws,
                                 const bool hasMask) {

  using namespace Mantid::API;

  // Fetch the default and continue
  if (normalizationOption == AutoSelect) {
    // enum to enum.
    normalizationOption =
        static_cast<VisualNormalization>(ws->displayNormalization());
  }

  NormFuncIMDNodePtr normalizationFunction;

  // Avoid checking every box for a mask if there is no mask in the workspace
  // by using different functions
  if (hasMask) {
    if (normalizationOption == Mantid::VATES::NumEventsNormalization) {
      normalizationFunction = &IMDNode::getSignalByNEventsWithMask;
    } else if (normalizationOption == Mantid::VATES::NoNormalization) {
      normalizationFunction = &IMDNode::getSignalWithMask;
    } else {
      normalizationFunction = &IMDNode::getSignalNormalizedWithMask;
    }
  } else {
    if (normalizationOption == Mantid::VATES::NumEventsNormalization) {
      normalizationFunction = &IMDNode::getSignalByNEvents;
    } else if (normalizationOption == Mantid::VATES::NoNormalization) {
      normalizationFunction = &IMDNode::getSignal;
    } else {
      normalizationFunction = &IMDNode::getSignalNormalized;
    }
  }

  return normalizationFunction;
}

/**
Create iterators with correct normalization normalization to an IMDIterator.
@param normalizationOption : Visual Normalization option desired
@param ws : workspace to fetch defaults from if needed
@return new IMDIterator
*/
DLLExport Mantid::API::IMDIterator *
createIteratorWithNormalization(const VisualNormalization normalizationOption,
                                Mantid::API::IMDWorkspace const *const ws) {

  using namespace Mantid::API;

  MDNormalization targetNormalization;
  // Fetch the default and continue
  if (normalizationOption == AutoSelect) {
    // enum to enum.
    targetNormalization =
        static_cast<MDNormalization>(ws->displayNormalization());
  } else {
    targetNormalization = static_cast<MDNormalization>(normalizationOption);
  }

  // Create the iterator
  IMDIterator *iterator = ws->createIterator();
  // Set normalization
  iterator->setNormalization(targetNormalization);
  // Return it
  return iterator;
}
}
}
