#include "OculusBoard.h"
#include <math.h>
#include <assert.h>

extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/avec3.h>
#include <clib/aquat.h>
#include <clib/rseq.h>
}


#if USEODE
extern dWorldID world;
extern dSpaceID space;
static dJointGroupID contactgroup;
#endif


static bool world_init = false;


const double floor_friction = .3, constant_friction = 1*.2;
const double Ball::mass = 1.;
const double Ball::moi = 2. / 5. * mass * 1e0; // moment of inertia
double G = 98. * 4;

static void world_initialize(){
	world_init = true;
#if USEODE
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate (0);

	contactgroup = dJointGroupCreate (0);
	dWorldSetGravity (world,0,-9.8,0);
	dWorldSetQuickStepNumIterations (world, 64);

	dCreatePlane (space,0,1,0,-1);
	dCreatePlane (space,-1,0,0,-fabs(board.x0));
	dCreatePlane (space,1,0,0,-fabs(board.x1));
	dCreatePlane (space,0,0,-1,-fabs(board.y0));
	dCreatePlane (space,0,0,1,-fabs(board.y1));
#endif
}


void Board::init(){
	world_initialize();
	random_sequence rs;
	init_rseq(&rs, 342925);
	int i, ix = 0, iy = 0;
	for(i = 0; i < numof(balls); i++){
		if(i == 0)
			balls[i] = Ball(Vector3d(0, 0, 20), Vector3d(0,0,0), Vector3d(0,0,0));
		else{
			double x = 2. * ix - iy, z = -2. * iy * sqrt(3.) / 2.;
			Vector3d pos = Vector3d(x, 0, z);
	//		x = drseq(&rs) * 50 - 25, z = drseq(&rs) * 50 - 25;
			Vector3d velo = Vector3d(0, 0, 0);
			x = (drseq(&rs) * 2 - 1) * M_PI, z = (drseq(&rs) * 2 - 1) * M_PI;
			Vector3d omg = Vector3d(0,0,0)/*Vec3d(x, z, (drseq(&rs) * 2 - 1) * M_PI)*/;
			balls[i] = Ball(pos, velo, omg);
			if(iy <= ix){
				iy = iy + 1;
				ix = 0;
			}
			else
				ix++;
		}
	}
	cue = Ball(Vector3d(0, -10, 0), Vector3d(0,0,0));
	cue.rad = .1;
}


Board::Board(double ax0, double ay0, double ax1, double ay1) : x0(ax0), y0(ay0), x1(ax1), y1(ay1){
}


#if USEODE
// this is called by dSpaceCollide when two objects in space are
// potentially colliding.

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
  assert(o1);
  assert(o2);

  if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
  {
    fprintf(stderr,"testing space %p %p\n", o1,o2);
    // colliding a space with something
    dSpaceCollide2(o1,o2,data,&nearCallback);
    // Note we do not want to test intersections within a space,
    // only between spaces.
    return;
  }

//  fprintf(stderr,"testing geoms %p %p\n", o1, o2);

  const int N = 32;
  dContact contact[N];
  int n = dCollide (o1,o2,N,&(contact[0].geom),sizeof(dContact));
  if (n > 0) 
  {
    for (int i=0; i<n; i++) 
    {
	  // Paranoia  <-- not working for some people, temporarily removed for 0.6
      //dIASSERT(dVALIDVEC3(contact[i].geom.pos));
      //dIASSERT(dVALIDVEC3(contact[i].geom.normal));
      //dIASSERT(!dIsNan(contact[i].geom.depth));
      contact[i].surface.slip1 = 0.7;
      contact[i].surface.slip2 = 0.7;
      contact[i].surface.mode = dContactSoftERP | dContactSoftCFM | dContactApprox1 | dContactSlip1 | dContactSlip2;
      contact[i].surface.mu = 50.0; // was: dInfinity
      contact[i].surface.soft_erp = 0.96;
      contact[i].surface.soft_cfm = 0.04;
      dJointID c = dJointCreateContact (world,contactgroup,&contact[i]);
      dJointAttach (c,
		    dGeomGetBody(contact[i].geom.g1),
		    dGeomGetBody(contact[i].geom.g2));
    }
  }
}
#endif


#define EPSILON 1e-10

void Board::anim(double dt){
	int i, j, t;
#if USEODE
	double simstep = 0.001; // 1ms simulation steps
	for(i = 0; i < 10; i++){
	    dSpaceCollide (space,0,&nearCallback);
	    dWorldQuickStep (world, dt / 10.);
		dJointGroupEmpty (contactgroup);
	}
	for(i = 0; i < numof(balls); i++){
		balls[i].anim(*this, dt);
	}
#else
	dt /= 10.;
	for(t = 0; t < 10; t++){
/*		for(i = 0; i < numof(balls); i++) for(j = i+1; j < numof(balls); j++){
			Ball &b0 = balls[i], &b1 = balls[j];
			Vector3d dr = b0.pos - b1.pos, v = b0.velo - b1.velo;
			double distAlongVelo = dr.Dot(v);
			double minDist = (dr + v * distAlongVelo).Length();
			if(minDist < b0.rad + b1.rad){
				("collide %lg\n", (dr + v * distAlongVelo).Length());
			}
		}*/
		cue.anim(*this, dt);
		for(i = 0; i < numof(balls); i++){
			balls[i].anim(*this, dt);
		}
	}
#endif
}

bool Board::isStatic()const{
	int i;
	for(i = 0; i < numof(balls); i++) if(balls[i].velo.LengthSq() < EPSILON && balls[i].omg.LengthSq() < EPSILON);
	else return false;
	return true;
}
