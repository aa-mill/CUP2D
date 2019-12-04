//
//  CubismUP_2D
//  Copyright (c) 2018 CSE-Lab, ETH Zurich, Switzerland.
//  Distributed under the terms of the MIT license.
//
//  Created by Guido Novati (novatig@ethz.ch).
//


#pragma once
#include "PoissonSolver.h"
#ifdef HYPREFFT

#include "HYPRE_struct_ls.h"

class HYPREdirichlet : public PoissonSolver
{
  const bool bPeriodic = false;
  double * dbuffer;
  const std::string solver;
  HYPRE_StructGrid     hypre_grid;
  HYPRE_StructStencil  hypre_stencil;
  HYPRE_StructMatrix   hypre_mat;
  HYPRE_StructVector   hypre_rhs;
  HYPRE_StructVector   hypre_sol;
  HYPRE_StructSolver   hypre_solver;
  HYPRE_StructSolver   hypre_precond;
  Real pLast = 0;

 public:
  void solve(const std::vector<cubism::BlockInfo>& BSRC,
             const std::vector<cubism::BlockInfo>& BDST) override;

  HYPREdirichlet(SimulationData& s);

  std::string getName() {
    return "hypre";
  }

  ~HYPREdirichlet() override;
};

#endif
