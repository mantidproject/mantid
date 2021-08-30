// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace Mantid {
namespace Algorithms {

/* This is a simple class originally intended for use solely with FindCenterOfMassPosition2.cpp
 * 
 */
class WorkspaceBoundingBox {
public:
    WorkspaceBoundingBox(API::MatrixWorkspace_sptr workspace);
    ~WorkspaceBoundingBox();

    double getX() { return x;}
    double getY() { return y;}
    double getCenterX() { return *centerX;}
    double getCenterY() { return *centerY;}
    double getXMin() { return xMin;}
    double getXMax() { return xMax;}
    double getYMin() { return yMin;}
    double getYMax() { return yMax;}

    void setPosition(double x, double y){
        this.x = x;
        this.y = y;
    }

    void setCenter(double x, double y) {
        this.centerX = x;
        this.centerY = y;
    }

    void setBounds(double xMin, double xMax, double yMin, double yMax) {
        this.xMin = xMin;
        this.xMax = xMax;
        this.yMin = yMin;
        this.yMax = yMax;
    }

    double calculateDistance();
    double calculateRadiusX();
    double calculateRadiusY();
    bool isValidWs(int index);
    int findFirstValidWs(const int numSpec);
    double updatePositionAndReturnCount(double total_count, int index);
    void updateMinMax(int index);
    bool isOutOfBoundsOfNonDirectBeam(const double beam_radius, int index, const bool direct_beam);

private:
    API::MatrixWorkspace_sptr workspace;
    double x = 0;
    double y = 0;
    double centerX;
    double centerY;
    double xMin = 0;
    double xMax = 0;
    double yMin = 0;
    double yMax = 0;
    const int m_specID = 0;
}


}
}