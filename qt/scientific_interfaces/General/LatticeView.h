// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt {
namespace CustomInterfaces {
/** Abstract lattice view.

  @author Owen Arnold, RAL ISIS
  @date 06/Oct/2011
 */
class LatticeView {
public:
  virtual double getA1() const = 0;
  virtual double getA2() const = 0;
  virtual double getA3() const = 0;
  virtual double getB1() const = 0;
  virtual double getB2() const = 0;
  virtual double getB3() const = 0;
  virtual void indicateModified() = 0;
  virtual void indicateDefault() = 0;
  virtual void indicateInvalid() = 0;
  virtual void initalize(double a1, double a2, double a3, double b1, double b2, double b3) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
