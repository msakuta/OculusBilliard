#include "ball.h"
#include "board.h"
#include <math.h>
#include <assert.h>

extern "C"{
#include <clib/c.h>
#include <clib/mathdef.h>
#include <clib/avec3.h>
#include <clib/aquat.h>
#include <clib/rseq.h>
}

#include <cpplib/mat3.h>

#ifdef OCULUSBILLIARD
#include "OVRAdapt.h"
#endif

#if USEODE
extern dWorldID world;
extern dSpaceID space;
static dJointGroupID contactgroup;
#endif


static bool world_init = false;


const double Board::floor_friction = .3, Board::constant_friction = .05;
const double Board::holerad = Ball::defaultRadius * 2.5;
const double Ball::mass = 0.150; // 150 grams
const double Ball::moi = 2. / 5. * mass * defaultRadius * defaultRadius; // moment of inertia
static const double G = 9.8;

const double Ball::defaultRadius = 0.030; // Meters

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

void Ball::init(const Vec3d pos){
#ifdef OCULUSBILLIARD
	model = NULL;
#endif
	rad = defaultRadius;
	trail0 = trail1 = 0;
#if USEODE
	dMass m;
	body = dBodyCreate (world);
	dMassSetSphere (&m,1,rad);
	dBodySetMass (body,&m);
	dBodySetPosition (body, pos[0], pos[1], pos[2]);
	geom = dCreateSphere(0, rad);
	dGeomSetBody (geom,body);
	dSpaceAdd (space, geom);
#endif
}

void Ball::collide(Ball &o){
	static double elasticity = .99;
	Vec3d delta = (pos - o.pos);
	Vec3d n = delta.norm();
	double cs = (velo - o.velo).sp(n); // Closing Speed
	velo -= n * cs * elasticity;
	o.velo += n * cs * elasticity;
}


void Ball::receiveImpulse(const Vec3d &impulse, const Vec3d &position){
#if USEODE
	velo += impulse / mass;
	omg += impulse.vp(position) / moi;
	dBodySetLinearVel(body, velo[0], velo[1], velo[2]);
	dBodySetAngularVel(body, omg[0], omg[1], omg[2]);
#else
	velo += impulse / mass;
	omg -= impulse.vp(position) / moi;
#endif
}

// hit cussion; momentum and angular momentum trade with infinitely inertial object
void Ball::concussion(const Vec3d &contact, const Vec3d &n){
//	const Vec3d contact = Vec3d(-rad, 0, 0), n(1, 0, 0);
	if(1 < pos[1] + contact[1])
		return;
	Vec3d cvelo = contact.vp(omg) - velo; // Velocity at Contact position
	double f = 1. - exp(-1e-3 * velo.sp(n));
	receiveImpulse(cvelo * f, contact);
/*	double f = 1e-1 * floor_friction * velo.sp(n) / moi;
	Vec3d domg = contact.vp(cvelo) * (f);
	if(omg.slen() * 1.8 * 1.8 < domg.slen())
		domg *= 1.8 * omg.len() / domg.len();
	omg += domg;
	aaccel += domg;*/
}

static Mat3d skewSymmetric(const Vec3d &v){
	Mat3d ret;
	ret[0] = ret[4] = ret[8] = 0.;
	ret[1] = v[2];
	ret[2] = -v[1];
	ret[3] = -v[2];
	ret[5] = v[0];
	ret[6] = v[1];
	ret[7] = -v[0];
	return ret;
}

void Ball::anim(Board &b, double dt){
	if(pot) // Don't simulate potted balls
		return;
	Ball (&balls)[numof(b.balls)] = b.balls;
	aaccel = vec3_000;
	pos += velo * dt;
/*	amat4_t mat;
	amat3_t omgt, nmat3;*/
	cpplib::Quatd qomg, q, qbackup;

	if(trail1 == trail0 || .2 * .2 < (pos - trails[trail1]).slen()){
		trail1 = (trail1+1) % numof(trails);
		if(trail1 == trail0)
			trail0 = (trail0+1) % numof(trails);
		trails[trail1] = pos;
	}

#if USEODE
	const dReal *SPos = dBodyGetPosition(body);
	pos[0] = SPos[0], pos[1] = SPos[1], pos[2] = SPos[2];
	const dReal *SRot = dBodyGetQuaternion(body);
	QUATCPY(rot, SRot);
	const dReal *SOmg = dBodyGetRotation(body);
	VECCPY(omg, SOmg);
#else
	// rotation
	if(1e-10 < omg.slen()){
		rot = cpplib::Quatd::rotation(omg.len() * dt, omg.norm()) * rot;
		rot.normin();
	}

/*	if(nmat){
		QUATCPY(qbackup, pt->rot);
		QUATCPY(pt->rot, *pq);
		tankrot(*nmat, pt);
		VECSADD(&(*nmat)[12], pt->velo, dt);
		QUATCPY(pt->rot, qbackup);
	}*/
	static double elasticity = .5;
	velo[1] -= G * dt;
	if(pos[1] < 0.){
		pos[1] = 0.;
		velo[1] = velo[1] < -10. - G * dt ? -.5 * velo[1] : 0.;
		concussion(Vec3d(0, -rad, 0), Vec3d(0, 1, 0));
	}
	if(pos[0] - rad < b.x0){
		pos[0] = b.x0 + rad;
		velo[0] = velo[0] < 0 ? -elasticity * velo[0] : 0;
		concussion(Vec3d(-rad, 0, 0), Vec3d(1, 0, 0));
//		Vec3d cvelo = contact.vp(omg) - (velo - n * velo.sp(n)); // Velocity at Contact position
//		omg += contact.vp(cvelo) * (1e-2 * floor_friction * velo[0] / moi);
	}
	if(b.x1 < pos[0] + rad){
		pos[0] = b.x1 - rad;
		velo[0] = 0 < velo[0] ? -elasticity * velo[0] : 0;
		concussion(Vec3d(rad, 0, 0), Vec3d(-1, 0, 0));
	}
	if(pos[2] - rad < b.y0){
		pos[2] = b.y0 + rad;
		velo[2] = velo[2] < 0 ? -elasticity * velo[2] : 0;
		concussion(Vec3d(0, 0, -rad), Vec3d(0, 0, 1));
	}
	if(b.y1 < pos[2] + rad){
		pos[2] = b.y1 - rad;
		velo[2] = 0 < velo[2] ? -elasticity * velo[2] : 0;
		concussion(Vec3d(0, 0, rad), Vec3d(0, 0, -1));
	}
	if(pos[1] <= 0./* && velo[1] <= 0.*/){
/*		if(velo[1] < G * dt)
			velo[1] = 0.;*/
		const Vec3d contact = Vec3d(0, -rad, 0);
		Vec3d cvelo = omg.vp(contact) + velo; // Velocity at Contact position
		if(1e-10 < cvelo.slen()){
			Mat3d impulseToTorque = skewSymmetric(contact);
			Mat3d invMOI(1. / moi, 0, 0, 0, 1. / moi, 0, 0, 0, 1. / moi);
			Mat3d deltaVelWorld = (impulseToTorque.transpose() * invMOI * impulseToTorque);
			Mat3d deltaVelocity = deltaVelWorld;
			deltaVelocity[0] += 1. / mass;
			deltaVelocity[4] += 1. / mass;
			deltaVelocity[8] += 1. / mass;
			Mat3d impulseMatrix = deltaVelocity.inverse();
			Mat3d testm = impulseMatrix * deltaVelocity;
			Vec3d velKill(-cvelo[0], -cvelo[1], -cvelo[2]);
			Vec3d impulseContact = impulseMatrix.vp(velKill);
			double planarImpulse = sqrt(
				impulseContact[0]*impulseContact[0] +
				impulseContact[2]*impulseContact[2]
				);
			const double friction = Board::floor_friction;
			if (planarImpulse > impulseContact[1] * friction){
				// We need to use dynamic friction
				impulseContact[0] /= planarImpulse;
				impulseContact[2] /= planarImpulse;

				double restingVelocity = deltaVelocity[4] +
					deltaVelocity[3] * friction * impulseContact[0] +
					deltaVelocity[5] * friction * impulseContact[2];
				impulseContact[1] = -cvelo[1] / restingVelocity;
				impulseContact[0] *= friction * impulseContact[1];
				impulseContact[2] *= friction * impulseContact[1];
			}
			receiveImpulse(impulseContact,  contact);
//			Vec3d domg = contact.vp(cvelo) * (1e-1 * floor_friction * dt / moi);
//			omg += domg;
//			aaccel += domg;
		//	velo += cvelo * (floor_friction * dt);
//			velo = (cvelo * exp(-dt * Board::floor_friction) + velo);
			double svelo = velo.slen();
			if(svelo < Board::constant_friction * dt * Board::constant_friction * dt)
				velo = Vec3d(0,0,0);
			else if(0. < svelo)
				velo *= (1. - Board::constant_friction * dt / sqrt(svelo));
/*			double f = exp(-Board::floor_friction * dt);
			omg *= f;*/
			double f = exp(-1. * dt);
			omg[1] *= f;
		}
#if 0
			receiveImpulse(cvelo * (1. - exp(-2. * dt)) / (1. + .1 * velo.len()), contact);
/*		Vec3d domg = contact.vp(cvelo) * (1e-1 * floor_friction * dt / moi);
		omg += domg;
		aaccel += domg;
	//	velo += cvelo * (floor_friction * dt);
		velo = (cvelo * exp(-1e0 / floor_friction * dt) + velo);*/
		double svelo = velo.slen();
		if(svelo < constant_friction * dt * constant_friction * dt)
			velo = vec3_000;
		else if(0. < svelo)
			velo *= (1. - constant_friction * dt / sqrt(svelo));
		double f = exp(-floor_friction * dt);
		omg *= f;
		f = exp(-1. * dt);
		omg[1] *= f;
//		omg[1] *= 1. / (1. + floor_friction * dt / moi);
#endif
		for(int ix = -1; ix <= 1; ix += 2){
			for(int iy = -1; iy <= 1; iy++){
				Vec3d potpos = Vec3d((board.x0 * (1 - ix) + board.x1 * (1 + ix)) / 2., 0, (board.y0 * (1 - iy) + board.y1 * (1 + iy)) / 2.);
				if((pos - potpos).slen() < Board::holerad * Board::holerad){
					pot = true;
#ifdef OCULUSBILLIARD
					if(model)
						model->SetVisible(false);
#endif
					return;
				}
			}
		}
	}

	// Air friction decays angular velocity unconditionally
	omg *= exp(-dt * Board::constant_friction);
//	omg *= 1. / (1. + Board::constant_friction * dt / moi);
	int i;
	for(i = 0; i < numof(balls); i++){
		Ball *ball = &balls[i];
		if(ball->pot) // Ignore potted balls
			continue;
		if(ball != this && intersects(balls[i]))
			collide(balls[i]);
	}
	if(!pot && intersects(b.cue))
		collide(b.cue);
#endif

#ifdef OCULUSBILLIARD
	if(model){
		Vector3f f(pos[0], pos[1], pos[2]);
		model->SetPosition(f);
		OVR::Quatf q(rot[0], rot[1], rot[2], rot[3]);
		model->SetOrientation(q);
		if(pot)
			model->SetVisible(false);
		else
			model->SetVisible(true);
	}
#endif
}

void Board::init(){
	world_initialize();
	random_sequence rs;
	init_rseq(&rs, 342925);
	int i, ix = 0, iy = 0;
	for(i = 0; i < numof(balls); i++){
		Ball &ball = balls[i];
		if(i == 0){
			ball.pos = Vec3d(0, 0, (x1 - x0) * .5);
			ball.velo = vec3_000;
			ball.omg = vec3_000;
			ball.rot = cpplib::Quatd(0,0,0,1);
		}
		else{
			double x = Ball::defaultRadius * (2. * ix - iy);
			double z = Ball::defaultRadius * (-2. * iy * sqrt(3.) / 2.);
			Vec3d pos = Vec3d(x, 0, z);
	//		x = drseq(&rs) * 50 - 25, z = drseq(&rs) * 50 - 25;
			Vec3d velo = Vec3d(0, 0, 0);
			x = (drseq(&rs) * 2 - 1) * M_PI, z = (drseq(&rs) * 2 - 1) * M_PI;
			Vec3d omg = vec3_000/*Vec3d(x, z, (drseq(&rs) * 2 - 1) * M_PI)*/;
			ball.pos = pos;
			ball.velo = velo;
			ball.omg = omg;
			ball.rot = cpplib::Quatd(0,0,0,1);
			if(iy <= ix){
				iy = iy + 1;
				ix = 0;
			}
			else
				ix++;
		}
	}
	cue = Ball(Vec3d(0, -10, 0), vec3_000);
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

void Board::anim(double dt
#ifdef OCULUSBILLIARD
		, const Vector3f &headPos
#endif
)
{
#ifdef OCULUSBILLIARD
	for(int i = 0; i < 3; i++)
		this->headPos[i] = headPos[i];
#endif

#if USEODE
	double simstep = 0.001; // 1ms simulation steps
	for(int i = 0; i < 10; i++){
	    dSpaceCollide (space,0,&nearCallback);
	    dWorldQuickStep (world, dt / 10.);
		dJointGroupEmpty (contactgroup);
	}
	for(int i = 0; i < numof(balls); i++){
		balls[i].anim(*this, dt);
	}
#else
	dt /= 10.;
	for(int t = 0; t < 10; t++){
		for(int i = 0; i < numof(balls); i++) for(int j = i+1; j < numof(balls); j++){
			Ball &b0 = balls[i], &b1 = balls[j];
			if(b0.pot || b1.pot)
				continue;
			Vec3d dr = b0.pos - b1.pos, v = b0.velo - b1.velo;
			double distAlongVelo = dr.sp(v);
			double minDist = (dr + distAlongVelo * v).len();
			if(minDist < b0.rad + b1.rad){
				("collide %lg\n", (dr + distAlongVelo * v).len());
			}
		}

		// Reset the cue ball if it's potted
		// but not totally erase the model
		if(balls[0].pot){
			balls[0].pos = Vec3d(0, 0, (x1 - x0) * .5);
			balls[0].velo = vec3_000;
			balls[0].omg = vec3_000;
			balls[0].rot = cpplib::Quatd(0,0,0,1);
			balls[0].pot = false;
		}

		for(int i = 0; i < numof(balls); i++){
			balls[i].anim(*this, dt);
		}
	}
#endif
}

bool Board::isStatic()const{
	int i;
	for(i = 0; i < numof(balls); i++) if(balls[i].velo.slen() < EPSILON && balls[i].omg.slen() < EPSILON);
	else return false;
	return true;
}

void Board::shoot(){
#ifdef OCULUSBILLIARD
	balls[0].velo += (balls[0].pos - ovrvc(headPos)).norm() * 5.;
#endif
}
