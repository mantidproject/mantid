# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Script to perform set up necessary for the SQL database
of system tests.
THIS WILL DELETE YOUR DATABASE!

This is not strictly necessary because it will be created automatically
when running the scripts.
"""
import sys
import time


import sqlresults
sqlresults.setup_database()
