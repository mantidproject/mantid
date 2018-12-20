#ifndef DISTRIBUTIONOPTIONS_H_
#define DISTRIBUTIONOPTIONS_H_

/**
 * This file contains declarations of options which control
 * normalization of mantid curves.
 */
namespace MantidQt {

// Enumerate how to handle distributions
enum DistributionFlag {
  DistributionDefault = 0, // Use preferences value
  DistributionTrue,        // Force distribution plotting
  DistributionFalse        // Disable distribution plotting
};
} // namespace MantidQt

#endif // DISTRIBUTIONOPTIONS_H_