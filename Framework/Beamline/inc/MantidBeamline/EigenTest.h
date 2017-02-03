#ifndef MANTID_EIGENTEST_H
#define MANTID_EIGENTEST_H

#include <Eigen/Core>
#include <Eigen/Geometry>

class EigenTest {

  virtual Eigen::Vector3d getPos() const = 0;
  virtual Eigen::Quaterniond getRotation() const = 0;

};

#endif //MANTID_EIGENTEST_H
