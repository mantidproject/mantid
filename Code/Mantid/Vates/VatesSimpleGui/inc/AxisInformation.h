/*
 * axisinformation.h
 *
 *  Created on: Apr 14, 2011
 *      Author: 2zr
 */

#ifndef AXISINFORMATION_H_
#define AXISINFORMATION_H_

#include <string>

class AxisInformation {
public:
	AxisInformation();
	virtual ~AxisInformation() {}

	double getMaximum() { return this->maximum; }
	double getMinimum() { return this->minimum; }
	std::string getTitle() { return this->title; }

	void setMaximum(double max) { this->maximum = max; }
	void setMinimum(double min) { this->minimum = min; }
	void setTitle(std::string title) { this->title = title; }

private:
	std::string title;
	double minimum;
	double maximum;
};

#endif /* AXISINFORMATION_H_ */
