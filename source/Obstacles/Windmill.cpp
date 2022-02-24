#include "Windmill.h"
#include "ShapeLibrary.h"
#include "../Utils/BufferedLogger.h"
#include <cmath>

using namespace cubism;

void Windmill::create(const std::vector<BlockInfo>& vInfo)
{
  // windmill stuff
  const Real h =  vInfo[0].h_gridpoint;
  for(auto & entry : obstacleBlocks) delete entry;
  obstacleBlocks.clear();
  obstacleBlocks = std::vector<ObstacleBlock*> (vInfo.size(), nullptr);

  #pragma omp parallel
  {
    //// original ellipse
    /*
    // center of ellipse 1 wrt to center of windmill,at T=0
    double center_orig1[2] = {smajax/2, -(smajax/2)*std::tan(M_PI/6)};
    // center of ellipse 1 wrt to origin
    double center1[2] = {center[0] + std::cos(orientation) * center_orig1[0] - std::sin(orientation)* center_orig1[1], 
                         center[1] + std::sin(orientation) * center_orig1[0] + std::cos(orientation) * center_orig1[1]};

    FillBlocks_Ellipse kernel1(smajax, sminax, h, center1, (orientation + 2*M_PI / 3), rhoS);

    // center of ellipse 2 wrt to center of windmill,at T=0
    double center_orig2[2] = {0, smajax/(2*std::cos(M_PI/6))};
    // center of ellipse 1 wrt to origin
    double center2[2] = {center[0] + std::cos(orientation) * center_orig2[0] - std::sin(orientation)* center_orig2[1], 
                         center[1] + std::sin(orientation) * center_orig2[0] + std::cos(orientation) * center_orig2[1]};

    FillBlocks_Ellipse kernel2(smajax, sminax, h, center2, (orientation + M_PI / 3), rhoS);

    // center of ellipse 3 wrt to center of windmill,at T=0
    double center_orig3[2] = {-smajax/2, -(smajax/2)*std::tan(M_PI/6)};
    // center of ellipse 1 wrt to origin
    double center3[2] = {center[0] + std::cos(orientation) * center_orig3[0] - std::sin(orientation)* center_orig3[1], 
                         center[1] + std::sin(orientation) * center_orig3[0] + std::cos(orientation) * center_orig3[1]};

    FillBlocks_Ellipse kernel3(smajax, sminax, h, center3, orientation, rhoS);
    */

    //// symmetrical ellipse

    double frac = 0.55;
    double d = smajax * (1.0 - 2.0*frac/3.0);

    // center of ellipse 1 wrt to center of windmill,at T=0, bottom one
    double center_orig1[2] = {d * std::sin(M_PI/6), -d * std::cos(M_PI/6)};
    // center of ellipse 1 wrt to origin
    double center1[2] = {center[0] + std::cos(orientation) * center_orig1[0] - std::sin(orientation)* center_orig1[1], 
                         center[1] + std::sin(orientation) * center_orig1[0] + std::cos(orientation) * center_orig1[1]};

    FillBlocks_Ellipse kernel1(smajax, sminax, h, center1, (orientation + 2*M_PI / 3), rhoS);

    // center of ellipse 2 wrt to center of windmill,at T=0, top one
    double center_orig2[2] = {d * std::sin(M_PI/6), +d * std::cos(M_PI/6)};
    // center of ellipse 1 wrt to origin
    double center2[2] = {center[0] + std::cos(orientation) * center_orig2[0] - std::sin(orientation)* center_orig2[1], 
                         center[1] + std::sin(orientation) * center_orig2[0] + std::cos(orientation) * center_orig2[1]};

    FillBlocks_Ellipse kernel2(smajax, sminax, h, center2, (orientation + M_PI / 3), rhoS);

    // center of ellipse 3 wrt to center of windmill,at T=0, horizontal one
    double center_orig3[2] = {-d, 0};
    // center of ellipse 1 wrt to origin
    double center3[2] = {center[0] + std::cos(orientation) * center_orig3[0] - std::sin(orientation)* center_orig3[1], 
                         center[1] + std::sin(orientation) * center_orig3[0] + std::cos(orientation) * center_orig3[1]};

    FillBlocks_Ellipse kernel3(smajax, sminax, h, center3, orientation, rhoS);


    // fill blocks for the three ellipses
    #pragma omp for schedule(dynamic, 1)
    for(size_t i=0; i<vInfo.size(); i++)
    {
      if(kernel1.is_touching(vInfo[i]))
      {
        assert(obstacleBlocks[vInfo[i].blockID] == nullptr);
        obstacleBlocks[vInfo[i].blockID] = new ObstacleBlock;
        obstacleBlocks[vInfo[i].blockID]->clear(); //memset 0
      }
      else if(kernel2.is_touching(vInfo[i]))
      {
        assert(obstacleBlocks[vInfo[i].blockID] == nullptr);
        obstacleBlocks[vInfo[i].blockID] = new ObstacleBlock;
        obstacleBlocks[vInfo[i].blockID]->clear(); //memset 0
      }
      else if(kernel3.is_touching(vInfo[i]))
      {
        assert(obstacleBlocks[vInfo[i].blockID] == nullptr);
        obstacleBlocks[vInfo[i].blockID] = new ObstacleBlock;
        obstacleBlocks[vInfo[i].blockID]->clear(); //memset 0
      }

      ScalarBlock& B = *(ScalarBlock*)vInfo[i].ptrBlock;
      if(obstacleBlocks[vInfo[i].blockID] == nullptr) continue;
      kernel1(vInfo[i], B, * obstacleBlocks[vInfo[i].blockID]);
      kernel2(vInfo[i], B, * obstacleBlocks[vInfo[i].blockID]);
      kernel3(vInfo[i], B, * obstacleBlocks[vInfo[i].blockID]);
    }
  }
}

void Windmill::updateVelocity(double dt)
{
  if (std::floor((1/time_step) * (sim.time + sim.dt)) - std::floor((1/time_step) * (sim.time)) != 0)
  {
    // appliedTorque = torque_over_time(sim.time);
    temp_torque = torque_over_time(sim.time); // change torque value every 0.01s
    
    printValues();
  }

  omega = omega + dt * temp_torque / penalJ;
  
  //Shape::updateVelocity(dt);

  // omega = omega + torque_over_time(sim.time) * dt / penalJ;

  // update omega according to a velocity function of time
  // appliedTorque = torque_over_time(sim.time);

}

double Windmill::torque_over_time(double time)
{
  double frequency = 0.5; // means period is 1/f = 2s
  double sinus = sin(2*M_PI*frequency * time);
  return torque_max * sinus;
}

void Windmill::updatePosition(double dt)
{
  if (std::floor((1/time_step) * (sim.time + sim.dt)) - std::floor((1/time_step) * (sim.time)) != 0) // every .01 seconds print velocity profile
  {
    print_vel_profile(avg_profile);
    // reset avg_profile to zero for next time step
    avg_profile = std::vector<double> (32, 0.0);
    // temp_torque = torque_over_time(sim.time);
    // omega = omega + temp_torque * time_step / penalJ;
  }

  Shape::updatePosition(dt);

  update_avg_vel_profile(dt);
}

void Windmill::printRewards(Real r_flow)
{
  std::stringstream ssF;
  ssF<<sim.path2file<<"/rewards_"<<obstacleID<<".dat";
  std::stringstream & fout = logger.get_stream(ssF.str());

  fout<<sim.time<<" "<<r_flow<<std::endl;
  fout.flush();
}

void Windmill::printNanRewards(bool is_energy, Real r)
{
  std::ofstream fout("nan.txt");

  if (is_energy)
  {
    fout<<"Energy produces "<<r<<std::endl;
  } else
  {
    fout<<"Flow produces "<<r<<std::endl;
  }
  fout.close();
}

void Windmill::printValues()
{
  std::stringstream ssF;
  ssF<<sim.path2file<<"/values_"<<obstacleID<<".dat";
  std::stringstream & fout = logger.get_stream(ssF.str());

  fout<<sim.time<<" "<<appliedTorque<<std::endl;
  fout.flush();
}

void Windmill::act( double action )
{
  // dimensionful applied torque from dimensionless action, divide by second squared
  // windscale is around 0.15, lengthscale is around 0.0375, so action is multiplied by around 16
  //appliedTorque = action / ( (lengthscale/windscale) * (lengthscale/windscale) );

  appliedTorque = action;
  //omega = action;
  printValues();
}

double Windmill::reward(Real factor, std::vector<double> true_profile)
{
  // first element of true profile is actually the time

  std::vector<double> curr_profile = vel_profile();

  Real r_flow = 0.0;

  for(int i=0; i < 32; ++i)
  {
    // used to be r_flow += std::sqrt( (true_profile[i]-curr_profile[i])*(true_profile[i]-curr_profile[i]) );
    r_flow += (true_profile[i+1]-curr_profile[i])*(true_profile[i+1]-curr_profile[i]);
  }

  r_flow *= -factor;
  printRewards(r_flow);
  return r_flow;
}

void Windmill::update_avg_vel_profile(double dt)
{
  std::vector<double> vel = vel_profile();
  for (int k(0); k < vel.size(); ++k)
  {
    avg_profile[k] += vel[k] * dt / time_step;
  }
}

void Windmill::print_vel_profile(std::vector<double> vel_profile)
{

  if(not sim.muteAll)
  {
    std::stringstream ssF;
    ssF<<sim.path2file<<"/velocity_profile_"<<obstacleID<<".dat";
    std::stringstream & fout = logger.get_stream(ssF.str());
    fout<<sim.time;

    for (int k = 0; k < vel_profile.size(); ++k)
    {
      // need to normalize profile by the time step
      fout<<" "<<vel_profile[k];
    }
    fout<<std::endl;
    
    fout.flush();
  }
}

std::vector<double> Windmill::vel_profile()
{
  // We take a region of size 0.7 * 0.0875, which cuts 4 vertical blocks in half along a vertical line
  // we choose to split these 4 blocks in the vertical dimension into 32 intervals
  // each one of the 32 intervals has a height of 0.7/32 = 0.021875
  // we average the velocity in each of the 32 intervals

  std::vector<double> vel_profile(32, 0.0);
  std::vector<double> vel_avg(32, 0.0);
  std::vector<double> region_area(32, 0.0);

  double height = 0.021875;

  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();
  
  // loop over all the blocks
  for(size_t t=0; t < velInfo.size(); ++t)
  {
    // get pointer on block
    const VectorBlock& b = * (const VectorBlock*) velInfo[t].ptrBlock;
    // loop over all the points
    double da = velInfo[t].h_gridpoint * velInfo[t].h_gridpoint;

    for(size_t i=0; i < b.sizeX; ++i)
      {
        for(size_t j=0; j < b.sizeY; ++j)
        {
          const std::array<Real,2> oSens = velInfo[t].pos<Real>(i, j);
          int num = numRegion(oSens, height);
          if (num)
          {
            region_area[num-1] += da;
            vel_avg[num-1] += std::sqrt(b(i, j).u[0]*b(i, j).u[0] + b(i, j).u[1]*b(i, j).u[1]) * da; // norm velocity profile
            //vel_avg[num-1] += b(i, j).u[0] * da; // velocity profile in x direction
          }
        }
      }
  }

  // divide each vel_avg by the corresponding area
  for (int k = 0; k < 32; ++k)
  {
    vel_profile[k] = vel_avg[k]/region_area[k];
  }

  return vel_profile;
}

int Windmill::numRegion(const std::array<Real, 2> point, double height) const
{
  // returns 0 if outside of the box
  std::array<Real, 2> lower_left = {x_start, y_start};
  std::array<Real, 2> upper_right = {x_end, y_end};
  double rel_pos_height = point[1] - lower_left[1];
  //std::array<Real, 2> rel_pos = {point[0] - lower_left[0], point[1] - lower_left[1]};
  int num = 0;

  if(point[0] >= lower_left[0] && point[0] <= upper_right[0])
  {
    if(point[1] >= lower_left[1] && point[1] <= upper_right[1])
    {
      // point is inside the rectangle to compute velocity profile
      // now find out in what region of the rectangle we are in
      num = static_cast<int>(std::ceil(rel_pos_height/height));
      
      return num;
    }
  }

  return 0;
}



///////////////////////////////////////////////////////unneeded

std::vector<double>  Windmill::state()
{
  // intitialize state vector
  std::vector<double> state(2);

  // angle
  // there exist a rotational symmetry, at every 120 degrees, so basically 
  double mill_orientation = std::fmod(orientation, M_PI/3);
  
  state[0] = mill_orientation;

  state[1] = omega;
  // angular velocity, dimensionless so multiply by seconds
  //state[1] = omega * (lengthscale/windscale);
  

  return state;
}


/* helpers to compute reward */

// average function
std::vector<double> Windmill::average(std::array<Real, 2> pSens) const
{
  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();

  // get blockId
  const size_t blockId = holdingBlockID(pSens, velInfo);

  // get block
  const auto& sensBinfo = velInfo[blockId];

  // get origin of block
  const std::array<Real,2> oSens = sensBinfo.pos<Real>(0, 0);

  // get inverse gridspacing in block
  const Real invh = 1/(sensBinfo.h_gridpoint);

  // get index for sensor
  const std::array<int,2> iSens = safeIdInBlock(pSens, oSens, invh);

  // // get velocity field at point
  // const VectorBlock& b = * (const VectorBlock*) sensBinfo.ptrBlock;

  // stencil for averaging
  static constexpr int stenBeg[3] = {-5,-5, 0}, stenEnd[3] = { 6, 6, 1};

  VectorLab vellab; vellab.prepare(*(sim.vel), stenBeg, stenEnd, 1);
  vellab.load(sensBinfo, 0); VectorLab & __restrict__ V = vellab;

  double avgX=0.0;
  double avgY=0.0;

  // average velocity in a cube of 11 points per direction around the point of interest (5 on each side)
  for (ssize_t i = -5; i < 6; ++i)
  {
    for (ssize_t j = -5; j < 6; ++j)
    {
      avgX += V(iSens[0] + i, iSens[1] + j).u[0];
      avgY += V(iSens[0] + i, iSens[1] + j).u[1];
    }
  }

  avgX/=121.0;
  avgY/=121.0;
  
  return std::vector<double> {avgX, avgY};
}

std::vector<double> Windmill::easyAverage() const
{
  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();

  // dummy average
  std::vector<double> sum = {0.0, 0.0};
  double total_area = 0.0;
  
  // loop over all the blocks
  for(size_t t=0; t < velInfo.size(); ++t)
  {
    // get pointer on block
    const VectorBlock& b = * (const VectorBlock*) velInfo[t].ptrBlock;
    // loop over all the points
    double da = velInfo[t].h_gridpoint * velInfo[t].h_gridpoint;

    for(size_t i=0; i < b.sizeX; ++i)
      {
        for(size_t j=0; j < b.sizeY; ++j)
        {
          const std::array<Real,2> oSens = velInfo[t].pos<Real>(i, j);
          if (isInArea(oSens))
          {
            total_area += da;
            sum[0] += b(i, j).u[0] * da;
            sum[1] += b(i, j).u[1] * da;
          }
        }
      }
  }

  // dummy average
  sum[0]/= total_area;
  sum[1]/= total_area;

  // intelligent average
  // smart_sum[0]/= smart_total_area;
  // smart_sum[1]/= smart_total_area;

  std::cout<<" Dummy Average: ["<< sum[0] <<", "<< sum[1] <<"] => NUM_PTS = "<<total_area<<std::endl;
  // std::cout<<" Smart Average: ["<< smart_sum[0] <<", "<< smart_sum[1] <<"]=> NUM_PTS = "<<smart_total_area<<std::endl;
  return sum;
}

std::vector<double> Windmill::sophAverage() const
{
  // Dummy way, looping over all the points in the grid and checking whether they are in the region of interest.

  // get info on the blocks
  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();

  // // dummy average
  // std::vector<double> sum = {0.0, 0.0};
  // double total_area = 0.0;

  // intelligent average
  std::vector<double> smart_sum = {0.0, 0.0};
  double smart_total_area = 0.0;
  
  // loop over all the blocks
  for(size_t t=0; t < velInfo.size(); ++t)
  {
    
    // get pointer on block
    const VectorBlock& b = * (const VectorBlock*) velInfo[t].ptrBlock;
    // loop over all the points
    double da = velInfo[t].h_gridpoint * velInfo[t].h_gridpoint;

    // // dummy average
    // for(size_t i=0; i < b.sizeX; ++i)
    // {
    //   for(size_t j=0; j < b.sizeY; ++j)
    //   {
    //     const std::array<Real,2> oSens = velInfo[t].pos<Real>(i, j);
    //     if (isInArea(oSens))
    //     {
    //       total_area += da;
    //       sum[0] += b(i, j).u[0] * da;
    //       sum[1] += b(i, j).u[1] * da;
    //     }
    //   }
    // }

    // intelligent average
    // get the lower left and upper right terms of the block
    std::array<Real,2> ll = velInfo[t].pos<Real>(0, 0);
    std::array<Real,2> ul = velInfo[t].pos<Real>(0, b.sizeY-1);
    std::array<Real,2> ur = velInfo[t].pos<Real>(b.sizeX-1, b.sizeY-1);
    std::array<Real,2> lr = velInfo[t].pos<Real>(b.sizeX-1, 0);

    // block is entirely inside the region
    if (isInArea(ll) && isInArea(ul) && isInArea(ur) && isInArea(lr))
    {
      for(size_t i=0; i < b.sizeX; ++i)
      {
        for(size_t j=0; j < b.sizeY; ++j)
        {
          smart_total_area += da;
          smart_sum[0] += b(i, j).u[0] * da;
          smart_sum[1] += b(i, j).u[1] * da;
        }
      }

    } // block is partly inside the region
    else if (isInArea(ll) || isInArea(ul) || isInArea(ur) || isInArea(lr))
    {
      for(size_t i=0; i < b.sizeX; ++i)
      {
        for(size_t j=0; j < b.sizeY; ++j)
        {
          const std::array<Real,2> oSens = velInfo[t].pos<Real>(i, j);
          if (isInArea(oSens))
          {
            smart_total_area += da;
            smart_sum[0] += b(i, j).u[0] * da;
            smart_sum[1] += b(i, j).u[1] * da;
          }
        }
      } // region is contained within one block
    } else if (target[0] >= ul[0] && target[0] <= ur[0] && target[1] <= ur[1] && target[1] >= lr[1])
    {
      for(size_t i=0; i < b.sizeX; ++i)
      {
        for(size_t j=0; j < b.sizeY; ++j)
        {
          const std::array<Real,2> oSens = velInfo[t].pos<Real>(i, j);
          if (isInArea(oSens))
          {
            smart_total_area += da;
            smart_sum[0] += b(i, j).u[0] * da;
            smart_sum[1] += b(i, j).u[1] * da;
          }
        }
      }
    }

  }

  // dummy average
  // sum[0]/= total_area;
  // sum[1]/= total_area;

  // intelligent average
  smart_sum[0]/= smart_total_area;
  smart_sum[1]/= smart_total_area;

  // std::cout<<" Dummy Average: ["<< sum[0] <<", "<< sum[1] <<"] => NUM_PTS = "<<total_area<<std::endl;
  std::cout<<" Smart Average: ["<< smart_sum[0] <<", "<< smart_sum[1] <<"]=> NUM_PTS = "<<smart_total_area<<std::endl;

  return smart_sum;

}

bool Windmill::isInArea(const std::array<Real, 2> point) const
{
  std::array<Real, 2> lower_left = {target[0]-dimensions[0]/2.0, target[1]-dimensions[1]/2.0};
  std::array<Real, 2> upper_right = {target[0]+dimensions[0]/2.0, target[1]+dimensions[1]/2.0};

  if(point[0] >= lower_left[0] && point[0] <= upper_right[0])
  {
    if(point[1] >= lower_left[1] && point[1] <= upper_right[1])
    {
      return true;
    }
  }

  return false;
}

// function that finds block id of block containing pos (x,y)
size_t Windmill::holdingBlockID(const std::array<Real,2> pos, const std::vector<cubism::BlockInfo>& velInfo) const
{
  for(size_t i=0; i<velInfo.size(); ++i)
  {
    // get gridspacing in block
    const Real h = velInfo[i].h_gridpoint;

    // compute lower left corner of block
    std::array<Real,2> MIN = velInfo[i].pos<Real>(0, 0);
    for(int j=0; j<2; ++j)
      MIN[j] -= 0.5 * h; // pos returns cell centers

    // compute top right corner of block
    std::array<Real,2> MAX = velInfo[i].pos<Real>(VectorBlock::sizeX-1, VectorBlock::sizeY-1);
    for(int j=0; j<2; ++j)
      MAX[j] += 0.5 * h; // pos returns cell centers

    // check whether point is inside block
    if( pos[0] >= MIN[0] && pos[1] >= MIN[1] && pos[0] <= MAX[0] && pos[1] <= MAX[1] )
    {
      // select block
      return i;
    }
  }
  printf("ABORT: coordinate (%g,%g) could not be associated to block\n", pos[0], pos[1]);
  fflush(0); abort();
  return 0;
};

// function that gives indice of point in block
std::array<int, 2> Windmill::safeIdInBlock(const std::array<Real,2> pos, const std::array<Real,2> org, const Real invh ) const
{
  const int indx = (int) std::round((pos[0] - org[0])*invh);
  const int indy = (int) std::round((pos[1] - org[1])*invh);
  const int ix = std::min( std::max(0, indx), VectorBlock::sizeX-1);
  const int iy = std::min( std::max(0, indy), VectorBlock::sizeY-1);
  return std::array<int, 2>{{ix, iy}};
};


std::vector<std::vector<double>> Windmill::getUniformGrid()
{
  /*
  // // get pointer on block
  // // const VectorBlock& b = * (const VectorBlock*) velInfo[t].ptrBlock;
  const unsigned int nX = VectorBlock::sizeX;
  const unsigned int nY = VectorBlock::sizeY;
  

  std::vector<VectorBlock *> velblocks = sim.vel->GetBlocks();
  const std::vector<cubism::BlockInfo>& velInfo = sim.vel->getBlocksInfo();
  std::array<int, 3> bpd = sim.vel->getMaxBlocks();
  const unsigned int unx = bpd[0]*(1<<(levelMax-1))*nX;
  const unsigned int uny = bpd[1]*(1<<(levelMax-1))*nY;
  const unsigned int NCHANNELS = 2;


  std::vector <float> uniform_mesh(uny*unx*NCHANNELS);

  for (size_t i = 0 ; i < velInfo.size() ; i++)
  {
    const int level = velInfo[i].level;
    const BlockInfo & info = velInfo[i];
    const VectorBlock& block = * (const VectorBlock*) info.ptrBlock;

    for (unsigned int y = 0; y < nY; y++)
    for (unsigned int x = 0; x < nX; x++)
    {
      //////// need to clarify this stuff
      float output[NCHANNELS]={0.0};
      float dudx  [NCHANNELS]={0.0};
      float dudy  [NCHANNELS]={0.0};
      TStreamer::operate(block, x, y, 0, (float *)output); //StreamerVector in Definitions.h

      if (x!= 0 && x!= nX-1)
      {
        float output_p [NCHANNELS]={0.0};
        float output_m [NCHANNELS]={0.0};
        TStreamer::operate(block, x+1, y, 0, (float *)output_p);
        TStreamer::operate(block, x-1, y, 0, (float *)output_m);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudx[j] = 0.5*(output_p[j]-output_m[j]);
      }
      else if (x==0)
      {
        float output_p [NCHANNELS]={0.0};
        TStreamer::operate(block, x+1, y, 0, (float *)output_p);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudx[j] = output_p[j]-output[j];        
      }
      else
      {
        float output_m [NCHANNELS]={0.0};
        TStreamer::operate(block, x-1, y, 0, (float *)output_m);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudx[j] = output[j]-output_m[j];        
      }

      if (y!= 0 && y!= nY-1)
      {
        float output_p [NCHANNELS]={0.0};
        float output_m [NCHANNELS]={0.0};
        TStreamer::operate(block, x, y+1, 0, (float *)output_p);
        TStreamer::operate(block, x, y-1, 0, (float *)output_m);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudy[j] = 0.5*(output_p[j]-output_m[j]);
      }
      else if (y==0)
      {
        float output_p [NCHANNELS]={0.0};
        TStreamer::operate(block, x, y+1, 0, (float *)output_p);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudy[j] = output_p[j]-output[j];        
      }
      else
      {
        float output_m [NCHANNELS]={0.0};
        TStreamer::operate(block, x, y-1, 0, (float *)output_m);
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          dudy[j] = output[j]-output_m[j];        
      }

      int iy_start = (info.index[1]*nY + y)*(1<< ( (levelMax-1)-level ) );
      int ix_start = (info.index[0]*nX + x)*(1<< ( (levelMax-1)-level ) );

      const int points = 1<< ( (levelMax-1)-level ); 
      const double dh = 1.0/points;

      for (int iy = iy_start; iy< iy_start + (1<< ( (levelMax-1)-level ) ); iy++)
      for (int ix = ix_start; ix< ix_start + (1<< ( (levelMax-1)-level ) ); ix++)
      {
        double cx = (ix - ix_start - points/2 + 1 - 0.5)*dh;
        double cy = (iy - iy_start - points/2 + 1 - 0.5)*dh;
        for (unsigned int j = 0; j < NCHANNELS; ++j)
          uniform_mesh[iy*NCHANNELS*unx+ix*NCHANNELS+j] = output[j]+ cx*dudx[j]+ cy*dudy[j];
      }
    }

    }

  }
  
  */

}

// set initial conditions of the agent
void Windmill::setInitialConditions(double init_angle)
{
  // Intial fixed condition of angle and angular velocity
  
  printf("[Korali] Initial Conditions:\n");
  printf("[Korali] orientation: %f\n", init_angle);

  setOrientation(init_angle);
}

