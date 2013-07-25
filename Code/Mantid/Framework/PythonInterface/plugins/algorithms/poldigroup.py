#!/usr/bin/python
# A little program to generate a grouping file for POLDI

print('<?xml version="1.0" encoding="UTF-8" ?>')
print('<detector-grouping>')

for i in range(0,400):
    name = 'g' + str(i)
    idx = i*2
    print('<group name=\"' +name +'\" >')
    print('<ids val=\"' + str(idx) + '-' + str(idx+1) + '\"/>')
    print('</group>')

print('</detector-grouping>')
