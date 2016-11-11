#!/bin/env python
import numpy
#import pylab
import CalcStatistics
import random
import numpy
from PIMC import *



numParticles=2
numTimeSlices=5
tau=0.1
lam=0.5
Path=PathClass(numpy.zeros((numTimeSlices,numParticles,3),float),tau,lam)
Path.SetPotential(ZeroFunction)
Path.SetCouplingConstant(0.0)
print PIMC(10000,Path,GaussianSingleSliceMove)
