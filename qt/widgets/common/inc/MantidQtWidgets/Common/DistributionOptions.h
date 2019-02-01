// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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