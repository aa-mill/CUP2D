//
//  CubismUP_2D
//  Copyright (c) 2021 CSE-Lab, ETH Zurich, Switzerland.
//  Distributed under the terms of the MIT license.
//

#pragma once

#include "SimulationData.h"

class Operator
{
protected:
  SimulationData& sim;
  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();

public:
  Operator(SimulationData& s) : sim(s) { }
  virtual ~Operator() {}
  virtual void operator()(const double dt) = 0;

  virtual std::string getName() = 0;
};
