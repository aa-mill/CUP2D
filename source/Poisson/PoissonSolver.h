//
//  CubismUP_2D
//  Copyright (c) 2018 CSE-Lab, ETH Zurich, Switzerland.
//  Distributed under the terms of the MIT license.
//
//  Created by Guido Novati (novatig@ethz.ch).
//


#pragma once

#include "../Operator.h"

class PoissonSolver
{
 protected:
  SimulationData& sim;

  // memory buffer for mem transfers to/from solver:
  Real * buffer = nullptr; // rhs in cub2rhs, sol in sol2cub
  void cub2rhs(const std::vector<cubism::BlockInfo>& BSRC);
  void sol2cub(const std::vector<cubism::BlockInfo>& BDST);

 public:
  int iter = 0;
  const size_t stride;
  // total number of DOFs in y and x:
  const size_t totNy = sim.vel->getBlocksPerDimension(1) * VectorBlock::sizeY;
  const size_t totNx = sim.vel->getBlocksPerDimension(0) * VectorBlock::sizeX;
  Real * const presMom = new Real[totNy * totNx];

  PoissonSolver(SimulationData& s, long stride);

  virtual void solve(const std::vector<cubism::BlockInfo>& BSRC,
                     const std::vector<cubism::BlockInfo>& BDST) = 0;

  virtual ~PoissonSolver() { }

  static PoissonSolver * makeSolver(SimulationData& sim);
};
