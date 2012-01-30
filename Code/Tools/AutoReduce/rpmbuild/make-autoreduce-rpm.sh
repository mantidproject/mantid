#!/bin/bash

tar -czf ~/rpmbuild/SOURCES/autoreduce.tgz ./autoreduce
rpmbuild -ba ./SPECS/autoreduce.spec
