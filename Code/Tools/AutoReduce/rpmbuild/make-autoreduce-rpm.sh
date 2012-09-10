#!/bin/bash

tar -czf ~/rpmbuild/SOURCES/autoreduce.tgz ./autoreduce
rpmbuild -ba ./SPECS/autoreduce.spec
tar -czf ~/rpmbuild/SOURCES/autoreduce-adara.tgz ./autoreduce-adara
rpmbuild -ba ./SPECS/autoreduce-adara.spec
