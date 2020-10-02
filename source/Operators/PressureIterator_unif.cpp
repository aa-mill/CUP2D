//
//  CubismUP_2D
//  Copyright (c) 2018 CSE-Lab, ETH Zurich, Switzerland.
//  Distributed under the terms of the MIT license.
//
//  Created by Guido Novati (novatig@ethz.ch).
//


#include "PressureIterator_unif.h"
#include "../Poisson/PoissonSolver.h"
#include "../Shape.h"
#include "../Utils/BufferedLogger.h"

using namespace cubism;

#define EXPL_INTEGRATE_MOM
using CHI_MAT = Real[VectorBlock::sizeY][VectorBlock::sizeX];
using UDEFMAT = Real[VectorBlock::sizeY][VectorBlock::sizeX][2];
static constexpr Real EPS = std::numeric_limits<Real>::epsilon();

void PressureIterator_unif::updatePressureRHS(const double dt) const
{
  const size_t Nblocks = velInfo.size();

  const Real h = sim.getH(), facDiv = 0.5*h/dt;
  const std::vector<BlockInfo>& uDefInfo = sim.uDef->getBlocksInfo();
  const std::vector<BlockInfo>&  tmpInfo = sim.tmp->getBlocksInfo();
  const std::vector<BlockInfo>&  chiInfo = sim.chi->getBlocksInfo();

  #pragma omp parallel
  {
    static constexpr int stenBeg[3] = {-1,-1, 0}, stenEnd[3] = { 2, 2, 1};
    VectorLab velLab;  velLab.prepare( *(sim.vel),    stenBeg, stenEnd, 0);
    VectorLab uDefLab; uDefLab.prepare(*(sim.uDef),   stenBeg, stenEnd, 0);
    #pragma omp for schedule(static)
    for (size_t i=0; i < Nblocks; i++)
    {
      velLab.load( velInfo[i], 0);  const VectorLab& __restrict__ V   =  velLab;
      uDefLab.load(uDefInfo[i], 0); const VectorLab& __restrict__ UDEF= uDefLab;
      const ScalarBlock& __restrict__ CHI = *(ScalarBlock*)chiInfo[i].ptrBlock;
            ScalarBlock& __restrict__ TMP = *(ScalarBlock*)tmpInfo[i].ptrBlock;
      for(int iy=0; iy<VectorBlock::sizeY; ++iy)
      for(int ix=0; ix<VectorBlock::sizeX; ++ix) {
        const Real divVx  =    V(ix+1,iy).u[0] -    V(ix-1,iy).u[0];
        const Real divVy  =    V(ix,iy+1).u[1] -    V(ix,iy-1).u[1];
        const Real divUSx = UDEF(ix+1,iy).u[0] - UDEF(ix-1,iy).u[0];
        const Real divUSy = UDEF(ix,iy+1).u[1] - UDEF(ix,iy-1).u[1];
        TMP(ix, iy).s = facDiv*( divVx+divVy - CHI(ix,iy).s*(divUSx+divUSy) );
      }
    }
  }
}

void PressureIterator_unif::pressureCorrection(const double dt) const
{
  const size_t Nblocks = velInfo.size();

  const Real h = sim.getH(), pFac = -0.5*dt/h;
  const std::vector<BlockInfo>& presInfo = sim.pres->getBlocksInfo();
  const std::vector<BlockInfo>& tmpVInfo = sim.tmpV->getBlocksInfo();
  const std::vector<BlockInfo>& vPresInfo= sim.vFluid->getBlocksInfo();

  #pragma omp parallel
  {
    static constexpr int stenBeg[3] = {-1,-1, 0}, stenEnd[3] = { 2, 2, 1};
    ScalarLab presLab; presLab.prepare(*(sim.pres), stenBeg, stenEnd, 0);

    #pragma omp for schedule(static)
    for (size_t i=0; i < Nblocks; i++)
    {
      presLab.load(presInfo[i],0); const ScalarLab&__restrict__ P    = presLab;
            VectorBlock&__restrict__   V = *(VectorBlock*)vPresInfo[i].ptrBlock;
      const VectorBlock&__restrict__ TMPV= *(VectorBlock*) tmpVInfo[i].ptrBlock;
      for(int iy=0; iy<VectorBlock::sizeY; ++iy)
      for(int ix=0; ix<VectorBlock::sizeX; ++ix) {
        V(ix,iy).u[0] = TMPV(ix,iy).u[0] + pFac * (P(ix+1,iy).s-P(ix-1,iy).s);
        V(ix,iy).u[1] = TMPV(ix,iy).u[1] + pFac * (P(ix,iy+1).s-P(ix,iy-1).s);
      }
    }
  }
}

void PressureIterator_unif::integrateMomenta(Shape * const shape) const
{
  const size_t Nblocks = velInfo.size();

  const std::vector<ObstacleBlock*> & OBLOCK = shape->obstacleBlocks;
  const std::vector<BlockInfo>& vFluidInfo = sim.vFluid->getBlocksInfo();

  const Real Cx = shape->centerOfMass[0], Cy = shape->centerOfMass[1];
  const double hsq = std::pow(velInfo[0].h_gridpoint, 2);

  double PM=0, PJ=0, PX=0, PY=0, UM=0, VM=0, AM=0; //linear momenta
  #pragma omp parallel for schedule(dynamic,1) reduction(+:PM,PJ,PX,PY,UM,VM,AM)
  for(size_t i=0; i<Nblocks; i++)
  {
    const VectorBlock&__restrict__ V = *(VectorBlock*) vFluidInfo[i].ptrBlock;

    if(OBLOCK[vFluidInfo[i].blockID] == nullptr) continue;
    const CHI_MAT & __restrict__ chi = OBLOCK[ vFluidInfo[i].blockID ]->chi;
    #ifndef EXPL_INTEGRATE_MOM
    const Real lambdt = sim.lambda * sim.dt;
    const UDEFMAT & __restrict__ udef = OBLOCK[ vFluidInfo[i].blockID ]->udef;
    #endif

    for(int iy=0; iy<VectorBlock::sizeY; ++iy)
    for(int ix=0; ix<VectorBlock::sizeX; ++ix)
    {
      if (chi[iy][ix] <= 0) continue;
      #ifdef EXPL_INTEGRATE_MOM
        const Real F = hsq * chi[iy][ix];
        const Real udiff[2] = { V(ix,iy).u[0], V(ix,iy).u[1] };
      #else
        const Real Xlamdt = chi[iy][ix] * lambdt;
        const Real F = hsq * Xlamdt / (1 + Xlamdt);
        const Real udiff[2] = {
          V(ix,iy).u[0] - udef[iy][ix][0], V(ix,iy).u[1] - udef[iy][ix][1]
        };
      #endif
      double p[2]; vFluidInfo[i].pos(p, ix, iy); p[0] -= Cx; p[1] -= Cy;
      PM += F;
      PJ += F * (p[0]*p[0] + p[1]*p[1]);
      PX += F * p[0];
      PY += F * p[1];
      UM += F * udiff[0];
      VM += F * udiff[1];
      AM += F * (p[0]*udiff[1] - p[1]*udiff[0]);
    }
  }

  #ifdef EXPL_INTEGRATE_MOM
  shape->fluidAngMom = AM; shape->fluidMomX = UM; shape->fluidMomY = VM;
  shape->penalDX=0; shape->penalDY=0; shape->penalM=PM; shape->penalJ=PJ;
  #else
  shape->fluidAngMom = AM; shape->fluidMomX = UM; shape->fluidMomY = VM;
  shape->penalDX=PX; shape->penalDY=PY; shape->penalM=PM; shape->penalJ=PJ;
  #endif
}

Real PressureIterator_unif::penalize(const double dt) const
{
  const size_t Nblocks = velInfo.size();

  const std::vector<BlockInfo>& chiInfo = sim.chi->getBlocksInfo();
  const std::vector<BlockInfo>& tmpVInfo  = sim.tmpV->getBlocksInfo();
  const std::vector<BlockInfo>& vFluidInfo = sim.vFluid->getBlocksInfo();
  #ifndef EXPL_INTEGRATE_MOM
    const Real lamdt = sim.lambda * dt;
  #endif

  Real MX = 0, MY = 0, DMX = 0, DMY = 0;
  #pragma omp parallel for schedule(dynamic, 1) reduction(+ : MX, MY, DMX, DMY)
  for (size_t i=0; i < Nblocks; i++)
  for (Shape * const shape : sim.shapes)
  {
    const std::vector<ObstacleBlock*>& OBLOCK = shape->obstacleBlocks;
    const ObstacleBlock*const o = OBLOCK[velInfo[i].blockID];
    if (o == nullptr) continue;

    const Real u_s = shape->u, v_s = shape->v, omega_s = shape->omega;
    const Real Cx = shape->centerOfMass[0], Cy = shape->centerOfMass[1];

    const CHI_MAT & __restrict__ X = o->chi;
    const UDEFMAT & __restrict__ UDEF = o->udef;
    const VectorBlock&__restrict__ TMPV = *(VectorBlock*)  tmpVInfo[i].ptrBlock;
    const ScalarBlock& __restrict__ CHI = *(ScalarBlock*)   chiInfo[i].ptrBlock;
    const VectorBlock& __restrict__  UF = *(VectorBlock*)vFluidInfo[i].ptrBlock;
          VectorBlock& __restrict__   V = *(VectorBlock*)   velInfo[i].ptrBlock;

    for(int iy=0; iy<VectorBlock::sizeY; ++iy)
    for(int ix=0; ix<VectorBlock::sizeX; ++ix)
    {
      // What if multiple obstacles share a block? Do not write udef onto
      // grid if CHI stored on the grid is greater than obst's CHI.
      if(CHI(ix,iy).s > X[iy][ix]) continue;
      if(X[iy][ix] <= 0) continue; // no need to do anything

      Real p[2]; velInfo[i].pos(p, ix, iy); p[0] -= Cx; p[1] -= Cy;
      const Real US = u_s - omega_s * p[1] + UDEF[iy][ix][0];
      const Real VS = v_s + omega_s * p[0] + UDEF[iy][ix][1];
      #ifdef EXPL_INTEGRATE_MOM
        const Real penalFac = X[iy][ix];
      #else
        const Real penalFac = lamdt*X[iy][ix]/(1 +lamdt*X[iy][ix]);
      #endif
      const Real DFX = penalFac * (US - UF(ix,iy).u[0]);
      const Real DFY = penalFac * (VS - UF(ix,iy).u[1]);
      const Real DPX = TMPV(ix,iy).u[0]+DFX - V(ix,iy).u[0];
      const Real DPY = TMPV(ix,iy).u[1]+DFY - V(ix,iy).u[1];
      V(ix,iy).u[0] += DPX; // in V store u^t plus penalization force
      V(ix,iy).u[1] += DPY; // without pressure projection (for P rhs)
      MX += std::pow(V(ix,iy).u[0], 2); DMX += std::pow(DPX, 2);
      MY += std::pow(V(ix,iy).u[1], 2); DMY += std::pow(DPY, 2);
    }
  }
  // return L2 relative momentum
  return std::sqrt((DMX + DMY) / (EPS + MX + MY));
}

void PressureIterator_unif::finalizePressure(const double dt) const
{
  const size_t Nblocks = velInfo.size();

  const Real h = sim.getH(), pFac = -0.5*dt/h;
  const std::vector<BlockInfo>& presInfo = sim.pres->getBlocksInfo();

  #pragma omp parallel
  {
    static constexpr int stenBeg[3] = {-1,-1, 0}, stenEnd[3] = { 2, 2, 1};
    ScalarLab presLab; presLab.prepare(*(sim.pres), stenBeg, stenEnd, 0);

    #pragma omp for schedule(static)
    for (size_t i=0; i < Nblocks; i++)
    {
      presLab.load(presInfo[i],0); const ScalarLab&__restrict__ P    = presLab;
      VectorBlock&__restrict__ V = *(VectorBlock*)  velInfo[i].ptrBlock;
      for(int iy=0; iy<VectorBlock::sizeY; ++iy)
      for(int ix=0; ix<VectorBlock::sizeX; ++ix) {
        V(ix,iy).u[0] += pFac * (P(ix+1,iy).s - P(ix-1,iy).s);
        V(ix,iy).u[1] += pFac * (P(ix,iy+1).s - P(ix,iy-1).s);
      }
    }
  }
}

void PressureIterator_unif::operator()(const double dt)
{
  const size_t Nblocks = velInfo.size();

  // first copy velocity before either Pres or Penal onto tmpV
  const std::vector<BlockInfo>& tmpVInfo = sim.tmpV->getBlocksInfo();
  const std::vector<BlockInfo>& presInfo = sim.pres->getBlocksInfo();
  const std::vector<BlockInfo>&  tmpInfo = sim.tmp->getBlocksInfo();
  #pragma omp parallel for schedule(static)
  for (size_t i=0; i < Nblocks; i++) {
          VectorBlock & __restrict__ UF = *(VectorBlock*) tmpVInfo[i].ptrBlock;
    const VectorBlock & __restrict__  V = *(VectorBlock*)  velInfo[i].ptrBlock;
    UF.copy(V);
  }

  int iter = 0;
  Real relDF = 1e3;
  bool bConverged = false;
  for(iter = 0; iter < 1000; iter++)
  {
    sim.startProfiler("PCorrect");
    pressureCorrection(dt);
    sim.stopProfiler();

    sim.startProfiler("Obj_force");
    for(Shape * const shape : sim.shapes)
    {
      // integrate vel in velocity after PP
      integrateMomenta(shape);
      shape->updateVelocity(dt);
    }

     // finally update vel with penalization but without pressure
    relDF = penalize(dt);
    sim.stopProfiler();

    // pressure solver is going to use as RHS = div VEL - \chi div UDEF
    sim.startProfiler("Prhs");
    updatePressureRHS(dt);
    sim.stopProfiler();

    pressureSolver->solve(tmpInfo, presInfo);

    printf("iter:%02d - max relative error: %e\n", iter, relDF);
    bConverged = relDF<targetRelError || iter>100 || iter>2*oldNsteps;

    if(bConverged)
    {
      sim.startProfiler("PCorrect");
      finalizePressure(dt);
      sim.stopProfiler();
      break;
    }
  }

  oldNsteps = iter+1;
  if(oldNsteps > 30) targetRelError = std::max({relDF, targetRelError});
  if(oldNsteps > 10) targetRelError *= 1.01;
  if(oldNsteps <= 2) targetRelError *= 0.99;
  targetRelError = std::min((Real)1e-3, std::max((Real)1e-5, targetRelError));

  if(not sim.muteAll)
  {
  std::stringstream ssF; ssF<<sim.path2file<<"/pressureIterStats.dat";
  std::ofstream pfile(ssF.str().c_str(), std::ofstream::app);
  if(sim.step==0) pfile<<"step time dt iter relDF"<<std::endl;
  pfile<<sim.step<<" "<<sim.time<<" "<<sim.dt<<" "<<iter<<" "<<relDF<<std::endl;
  }
}

PressureIterator_unif::PressureIterator_unif(SimulationData& s) : Operator(s), pressureSolver( PoissonSolver::makeSolver(s) )  { }

PressureIterator_unif::~PressureIterator_unif() {
    delete pressureSolver;
}
