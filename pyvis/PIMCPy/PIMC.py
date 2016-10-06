# Following http://web.engr.illinois.edu/~bkclark/PIMCTutorial/tutorial.pdf
import numpy
##import pylab
from PIMCHelp import *
import random
import CalcStatistics


class PathClass:
  def __init__(self,beads,tau,lam):
     self.tau=tau
     self.lam=lam
     self.beads=beads.copy()
     self.NumTimeSlices=len(beads)
     self.NumParticles=len(beads[0])
     self.sigma=numpy.sqrt((self.lam*self.tau))
     print "I have setup the path with a temperature of ",1.0/(tau*self.NumTimeSlices), "and ",self.NumParticles," particles"
  def SetCouplingConstant(self,c):
     self.c=c
  def SetPotential(self,externalPotentialFunction):
     self.VextHelper=externalPotentialFunction
  def Vee(self,R):
     dist=numpy.sqrt(numpy.dot(R,R))
     return self.c/(dist+0.1)
  def Vext(self,R):
     return self.VextHelper(R)
  def KineticAction(self,slice1,slice2):
     tot=0.0
     disp=self.beads[slice1,0]-self.beads[slice2,0]
     dist2=numpy.dot(disp,disp)
     tot=tot+dist2*1.0/(4.0*self.lam*self.tau)
     disp=self.beads[slice1,1]-self.beads[slice2,1]
     dist2=numpy.dot(disp,disp)
     tot=tot+dist2*1.0/(4.0*self.lam*self.tau)
     return tot
  def PotentialAction(self,slice1,slice2):
     r1s1=self.beads[slice1,0]
     r2s1=self.beads[slice1,1]
     r1s2=self.beads[slice2,0]
     r2s2=self.beads[slice2,1]
     Vexternal=self.Vext(r1s1)+self.Vext(r2s1)+self.Vext(r1s2)+self.Vext(r2s2)
     delta1 = r1s1 - r2s1
     delta2 = r1s2 - r2s2
     r12s1=numpy.sqrt(numpy.dot(delta1, delta1))
     r12s2=numpy.sqrt(numpy.dot(delta2, delta2))
     Vinteraction=self.Vee(r12s2)+self.Vee(r12s1)
     return 0.5*self.tau*(Vinteraction+Vexternal)
  def RelabelBeads(self):
    #print "relabeling beads: ", self.beads
    slicesToShift=random.randint(0,self.NumTimeSlices-1)
    l=range(slicesToShift,len(self.beads))+range(0,slicesToShift)
    self.beads=self.beads[l].copy()
    #print "relabeled beads: ", self.beads
  def KineticEnergy(self):
    KE=0.0
    oneOverFourLambdaTau2=1.0/(4.0*self.lam*self.tau*self.tau)
    for ptcl in range(0,self.NumParticles):
        for slice in range(0,self.NumTimeSlices-1):
           disp=self.beads[slice,ptcl]-self.beads[slice+1,ptcl]
           dist2=numpy.dot(disp,disp)
           KE=KE+3.0/(2.0*self.tau)-dist2*oneOverFourLambdaTau2
        disp=self.beads[self.NumTimeSlices-1,ptcl]-self.beads[0,ptcl]
        dist2=numpy.dot(disp,disp)
        KE=KE+3.0/(2.0*self.tau)-dist2*oneOverFourLambdaTau2
    return KE/(self.NumTimeSlices+0.0)
  def PotentialEnergy(self):
     PE=0.0
     for slice in range(0,self.NumTimeSlices):
           r1=self.beads[slice,0]
           r2=self.beads[slice,1]
           r12=numpy.sqrt(numpy.dot(r1-r2,r1-r2))
           PE = PE+self.Vext(r1)+self.Vext(r2)+self.Vee(r12)
     return PE/(self.NumTimeSlices+0.0)
  def Energy(self):
     return self.PotentialEnergy()+self.KineticEnergy()
  def HarmonicEnergy(tau, numTimeSlices, numParticles) :
     return 0.0


def ZeroFunction(slice):
   return 0.0

def HarmonicOscillator(r):
    return 0.5*numpy.dot(r,r)

def CalcDensity(self,DensityHistogram):
    for slice in range(0,self.NumTimeSlices): 
        dist=numpy.sqrt(dot(self.beads[slice,0],self.beads[slice,0]))
        DensityHistogram.add(dist)
        #DensityHistogram.add(self.beads[slice,0,0])
        dist=numpy.sqrt(dot(self.beads[slice,1],self.beads[slice,1]))
        DensityHistogram.add(dist)
        #DensityHistogram.add(self.beads[slice,1,0])

def PairCorrelationFunction(self,PairHistogram):
    for slice in range(0,self.NumTimeSlices):
        disp=self.beads[slice,0]-self.beads[slice,1]
        dist=sqrt(dot(disp,disp) )
        PairHistogram.add(dist)

PairHistogram=Histogram(0.1,10.0,100)
DensityHistogram=Histogram(-5.0,5.0,100)
def PIMC(numSteps,Path,myMove):
   observableSkip=10
   EnergyTrace=[]
   accepted=0.0
   attempted=0.0
   for steps in range(0,numSteps):
         (accepted,attempted)=myMove(Path,accepted,attempted)
         DisplaceMove(Path)
         if steps % observableSkip==0 : # and steps>1000:
             EnergyTrace.append(Path.Energy())
             PairCorrelationFunction(Path,PairHistogram)
             CalcDensity(Path,DensityHistogram)
   
   #print EnergyTrace
   print CalcStatistics.Stats(numpy.array(EnergyTrace))
   #pylab.plot(EnergyTrace)
   #pylab.show()
   
   #print "Pair hisogram:"
   #PairHistogram.printMe()
   #print "Density Histogram"
   #DensityHistogram.printMe()
   
   #PairHistogram.plotMeNorm("pairme.png")
   #DensityHistogram.plotMe("density.png")
   #pylab.savefig("broken.png")
   
   print "Accepted Percentage: ",accepted/attempted
   #WriteArray("Canonical.txt",Path.beads)
   return EnergyTrace


def Bisect(Path,ptclToMove,maxStepSize):
   stepSize=maxStepSize
   logSampleProb=0.0
   while stepSize>=2:
        sigma=Path.sigma*numpy.sqrt(stepSize/2)
        for i in numpy.arange(0,maxStepSize,stepSize):
            midVec=(Path.beads[i,ptclToMove]+Path.beads[i+stepSize,ptclToMove])/2.0
            delta=numpy.random.normal(0.0,sigma,3)
            logSampleProb+=numpy.dot(delta,delta)/(2.0*sigma*sigma)
            Path.beads[i+stepSize/2,ptclToMove]=midVec+delta
        stepSize=stepSize/2
   return logSampleProb

def NewToOld(Path,ptclToMove,maxStepSize):
   stepSize=maxStepSize
   logSampleProb=0.0
   while stepSize>=2:
        sigma=Path.sigma*numpy.sqrt(stepSize/2)
        for i in numpy.arange(0,maxStepSize,stepSize):
            midVec=(Path.beads[i,ptclToMove]+Path.beads[i+stepSize,ptclToMove])/2.0
            delta=Path.beads[i+stepSize/2,ptclToMove]-midVec
            logSampleProb+=numpy.dot(delta,delta)/(2.0*sigma*sigma)
        stepSize=stepSize/2
   return logSampleProb


def SingleSliceMove(Path,accepted,attempted):
   attempted=attempted+1.0
   Path.RelabelBeads() # this approach seems silly?
   ptclToMove=random.randint(0,Path.NumParticles-1)
   sliceToMove=1

   oldAct=-Path.KineticAction(sliceToMove-1,sliceToMove)
   oldAct-=Path.KineticAction(sliceToMove,sliceToMove+1)
   oldAct-=Path.PotentialAction(sliceToMove-1,sliceToMove)
   oldAct-=Path.PotentialAction(sliceToMove,sliceToMove+1)

   moveScalar = 1.0
   delta = moveScalar * (numpy.random.random(3) - 0.5)
   Path.beads[1,ptclToMove]=Path.beads[1,ptclToMove]+delta

   newAct=-Path.KineticAction(sliceToMove-1,sliceToMove)
   newAct-=Path.KineticAction(sliceToMove,sliceToMove+1)
   newAct-=Path.PotentialAction(sliceToMove-1,sliceToMove)
   newAct-=Path.PotentialAction(sliceToMove,sliceToMove+1)

   # metropolis markoc chain monte carlo
   # prob of accepting is exp(-(S_new - S_old)) T(n->o)/T(o->n)
   # T(n->o)/T(o->n) is the ratio of configuration probabilities and is taken to be 1
   # due to the uniform box boundary
   if not(newAct-oldAct>numpy.log(random.random())):
      Path.beads[sliceToMove,ptclToMove]=Path.beads[sliceToMove,ptclToMove]-delta
   else:
      accepted=accepted+1.0
   return (accepted,attempted)

def DisplaceMove(Path):
   ptcl = random.randint(0,Path.NumParticles-1)
   delta = 0.3*numpy.array([random.random()-0.5, random.random()-0.5, random.random()-0.5])
   Vold = 0.0
   Vnew = 0.0
   for i in range(0,Path.NumTimeSlices):
     Vold += Path.Vext(Path.beads[i,ptcl])
     Vnew += Path.Vext(Path.beads[i,ptcl]+delta)
   if (Path.tau*(Vold-Vnew) > log(random.random())):
     for i in range(0,Path.NumTimeSlices):
       Path.beads[i,ptcl] += delta
     return True
   else:
     return False
   

def GaussianSingleSliceMove(Path,accepted,attempted):
   oneOverFourLambdaTau=1.0/(4.0*Path.lam*Path.tau)
   attempted=attempted+1
   maxStepSize=2
   Path.RelabelBeads()
   ptclToMove=random.randint(0,1)
   oldCoords=Path.beads[0:maxStepSize,ptclToMove].copy()
   T_newToOld=NewToOld(Path,ptclToMove,maxStepSize)


   oldKA=0.0
   for slice in range(0,maxStepSize):
     disp=Path.beads[slice,ptclToMove]-Path.beads[slice+1,ptclToMove]
     dist2=numpy.dot(disp,disp)
     oldKA=oldKA-dist2*oneOverFourLambdaTau
     oldKA=oldKA-Path.PotentialAction(slice,slice+1)
     oldKA=oldKA-Path.PotentialAction(slice-1,slice)

   T_oldToNew=Bisect(Path,ptclToMove,maxStepSize)


   newKA=0.0
   for slice in range(0,maxStepSize):
      disp=Path.beads[slice,ptclToMove]-Path.beads[slice+1,ptclToMove]
      dist2=numpy.dot(disp,disp)
      newKA=newKA-dist2*oneOverFourLambdaTau
      newKA=newKA-Path.PotentialAction(slice,slice+1) #Path.V_external(slice)+Path.V_internal(slice)
      newKA=newKA-Path.PotentialAction(slice-1,slice) #Path.V_external(slice)+Path.V_internal(slice)
   if not((newKA-oldKA-(T_newToOld-T_oldToNew))>numpy.log(random.random())):
     Path.beads[0:maxStepSize,ptclToMove]=oldCoords.copy()
   else:
     accepted=accepted+1
   return (accepted,attempted)





def BisectionMove(Path,accepted,attempted):
   attempted=attempted+1
   maxStepSize=8
   Path.ShiftMove()
   ptclToMove=random.randint(0,1)
#   print "PTCL TO MOVE",ptclToMove                                                  
   oldCoords=Path.beads[0:maxStepSize,ptclToMove].copy()
   T_newToOld=NewToOld(Path,ptclToMove,maxStepSize)


   oldKA=0.0
   for slice in range(0,maxStepSize):
     disp=Path.beads[slice,ptclToMove]-Path.beads[slice+1,ptclToMove]
     dist2=numpy.dot(disp,disp)
     oldKA=oldKA-dist2*Path.oneOverFourLambdaTau
     oldKA=oldKA-Path.PotentialAction(slice)

   T_oldToNew=Bisect(Path,ptclToMove,maxStepSize)


   newKA=0.0
   for slice in range(0,maxStepSize):
      disp=Path.beads[slice,ptclToMove]-Path.beads[slice+1,ptclToMove]
      dist2=numpy.dot(disp,disp)
      newKA=newKA-dist2*Path.oneOverFourLambdaTau
      newKA=newKA-Path.PotentialAction(slice) #Path.V_external(slice)+Path.V_internal(slice)                                                   
   if not((newKA-oldKA-(T_newToOld-T_oldToNew))>numpy.log(random.random())):
     Path.beads[0:maxStepSize,ptclToMove]=oldCoords.copy()
   else:
     accepted=accepted+1
#     print "ERRROR"                                                                 
   return (accepted,attempted)

SingleSliceBisection=BisectionMove


def DoubleWell(self,slice):
    a=2.0
    x1=self.beads[slice,0,0]
    y1=self.beads[slice,0,1]
    z1=self.beads[slice,0,2]
    x2=self.beads[slice,1,0]
    y2=self.beads[slice,1,1]
    z2=self.beads[slice,1,2]
    return 1.0*(((x1-a)*(x1-a)+y1*y1+z1*z1)*((x1+a)*(x1+a)+y1*y1+z1*z1)+((x2-a)*(x2-a)+y2*y2+z2*z2)*((x2+a)*(x2+a)+y2*y2+z2*z2))

