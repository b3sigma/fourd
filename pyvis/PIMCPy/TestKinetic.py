#!/bin/env python
import numpy
#import pylab
import CalcStatistics
from PIMCHelp import *
import random
import numpy
from PIMC import *


tau=0.5
lam=0.5
Path=PathClass(ReadArray("CanonicalPath.txt"),tau,lam)
Path.SetPotential(ZeroFunction)
Path.SetCouplingConstant(0.0)
for i in range(0, len(Path.beads) - 1) :
    print "The value of the kinetic action is on slice [%i, %i] is" %(i, i+1), Path.KineticAction(i, i+1)

print "The value of the kinetic action is ", Path.KineticAction(1,2)
print "The expected value for the test data path in CanonicalPath.txt was 0.567..."

