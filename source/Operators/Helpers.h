//
//  CubismUP_2D
//  Copyright (c) 2018 CSE-Lab, ETH Zurich, Switzerland.
//  Distributed under the terms of the MIT license.
//
//  Created by Guido Novati (novatig@ethz.ch).
//


#pragma once

#include "../Operator.h"

class findMaxU
{
  SimulationData& sim;
  const std::vector<cubism::BlockInfo> & velInfo = sim.vel->getBlocksInfo();
  const size_t Nblocks = velInfo.size();
 public:
  findMaxU(SimulationData& s) : sim(s) { }

  Real run() const;

  std::string getName() const {
    return "findMaxU";
  }
};

class FadeOut : public Operator
{
  std::array<double, 4> solveMatrix(const double W, const double E,
                                    const double S, const double N) const;
 public:
  FadeOut(SimulationData& s) : Operator(s) { }

  void operator()(const double dt);

  std::string getName() {
    return "FadeOut";
  }
};

class Checker
{
  SimulationData& sim;
  const std::vector<cubism::BlockInfo> & velInfo = sim.vel->getBlocksInfo();
  const size_t Nblocks = velInfo.size();
 public:
  Checker(SimulationData& s) : sim(s) { }

  void run(std::string when) const;

  std::string getName() const {
    return "Checker";
  }
};

class IC : public Operator
{
  public:
  IC(SimulationData& s) : Operator(s) { }

  void operator()(const double dt);

  std::string getName() {
    return "IC";
  }
};

class ApplyObjVel : public Operator
{
  public:
  ApplyObjVel(SimulationData& s) : Operator(s) { }

  void operator()(const double dt);

  std::string getName() {
    return "ApplyObjVel";
  }
};

class computeVorticity
{
  SimulationData& sim;
  const std::vector<cubism::BlockInfo> & velInfo = sim.vel->getBlocksInfo();
  const size_t Nblocks = velInfo.size();
 public:
  computeVorticity(SimulationData& s) : sim(s) { }

  void run() const;

  std::string getName() const {
    return "computeVorticity";
  }
};


class computeDivergence
{
  SimulationData& sim;
  const std::vector<cubism::BlockInfo> & velInfo = sim.vel->getBlocksInfo();
  const size_t Nblocks = velInfo.size();
 public:
  computeDivergence(SimulationData& s) : sim(s) { }

  void run() const;

  std::string getName() const {
    return "computeDivergence";
  }
};
