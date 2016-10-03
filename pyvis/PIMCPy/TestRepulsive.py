#!/bin/env python
import numpy
#import pylab
#import stats
from VMCHelp import *
import random
import numpy
from PIMC5 import *



numParticles=2
numTimeSlices=10
tau=0.1
lam=0.5
Path=PathClass(numParticles,numTimeSlices,tau,lam)
Path.SetCouplingConstant(1.0)
Path.SetPotential(HarmonicOscillator)
print PIMC(10000,Path,SingleSliceBisection)
