Feature: Mantid developer
	I wish to demonstrate 
	How HB3A data reduction GUI is used to reduce HB3A data 

	Background: 
		Given I am using HFIR 4-circle data reduction GUI

	Scenario: Calculate UB matrix from 2 reflections
		Given I input experiment number and select the mode to access data from server directly
		Then I download data to a specified directory
		Then I load one data set, find 1 peak from it and specify its HKL value
		Then I load another data set, find 1 peak from it and specify its HKL value
		Then I calculate UB matrix from the 2 reflections
		Then I get the UB matrix and calculate HKL values for the 2 peaks given earlier
	    Then I used the calculated UB matrix to index some peaks
	    Then I import more peaks to refine UB matrix and lattice parameters
