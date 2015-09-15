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
