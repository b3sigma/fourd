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
Path.SetPotential(HarmonicOscillator)
Path.SetCouplingConstant(0.0)
print Path.beads
for i in range(0, len(Path.beads) - 1) :
    print "The value of the potential action between slice %i and %i (with a harmonic external potential is)" % (i, i+1), Path.PotentialAction(i, i+1)

print "The value of the potential action between slice 1 and 2 (with a harmonic external potential is)", Path.PotentialAction(1,2)
print "The expected value for CanonicalPath.txt was 2.464..."





