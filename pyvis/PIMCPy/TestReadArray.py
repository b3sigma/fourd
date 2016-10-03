#!/bin/env python
from PIMCHelp import *
import numpy


a = numpy.array([[[1.0, 2.0],[3.0,4.0],[5.0,6.0]],
                 [[7.0, 8.0],[9.0,1.1],[1.2,1.3]],
                 [[1.4, 1.5],[1.6,1.7],[1.8,1.0]],
                 [[2.4, 3.5],[4.6,5.7],[6.8,7.7]]])

print 'a = '
print a
WriteArray('Test.dat', a)

b= ReadArray('Test.dat')
print 'b = '
print b

print a-b
