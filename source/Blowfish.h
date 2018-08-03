//
//  Shape.h
//  CubismUP_2D
//
//  Virtual shape class which defines the interface
//  Default simple geometries are also provided and can be used as references
//
//  This class only contains static information (position, orientation,...), no dynamics are included (e.g. velocities,...)
//
//  Created by Christian Conti on 3/6/15.
//  Copyright (c) 2015 ETHZ. All rights reserved.
//

#pragma once
#include "Shape.h"

class BlowFish : public Shape
{
 public:
  Real flapAng_R = 0, flapAng_L = 0;
  Real flapVel_R = 0, flapVel_L = 0;
  Real flapAcc_R = 0, flapAcc_L = 0;

  void resetAll() {
    flapAng_R = 0, flapAng_L = 0;
    flapVel_R = 0, flapVel_L = 0;
    flapAcc_R = 0, flapAcc_L = 0;
    Shape::resetAll();
  }
  const Real radius;
  const Real rhoTop = 1.5; //top half
  const Real rhoBot = 0.5; //bot half
  const Real rhoFin = 1.0; //fins

  const Real finLength = 0.5*radius; //fins
  const Real finWidth  = 0.1*radius; //fins
  const Real finAngle0 = M_PI/6; //fins

  const Real attachDist = radius + std::max(finWidth, (Real)sim.getH()*2);
  //Real powerOutput = 0, old_powerOutput = 0;
  const Real deltaRho = 0.5; //ASSUME RHO FLUID == 1
  // analytical period of oscillation for small angles
  const Real timescale = sqrt(3*M_PI*radius/deltaRho/fabs(sim.gravity[1])/8);
  const Real minRho = std::min(rhoTop,rhoBot), maxRho = std::max(rhoTop,rhoBot);

  BlowFish(SimulationData&s, ArgumentParser&p, Real C[2]);

  void updatePosition(double dt) override;

  void create(const vector<BlockInfo>& vInfo) override;

  Real getCharLength() const  override
  {
    return 2 * radius;
  }

  void outputSettings(ostream &outStream) const override
  {
    outStream << "BlowFish\n";
    Shape::outputSettings(outStream);
  }
};
