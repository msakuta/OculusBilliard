#include "OculusBall.h"
#include "OculusBoard.h"


const double floor_friction = .3, constant_friction = 1*.2;


void Ball::init(const Vector3d &pos){
//	if(!world_init)
//		return;
	rad = 1.;
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
	Vector3d delta = (pos - o.pos);
	Vector3d n = delta.Normalized();
	double cs = (velo - o.velo).Dot(n); // Closing Speed
	velo -= n * cs * elasticity;
	o.velo += n * cs * elasticity;
}


void Ball::receiveImpulse(const Vector3d &impulse, const Vector3d &position){
#if USEODE
	velo += impulse / mass;
	omg += impulse.vp(position) / moi;
	dBodySetLinearVel(body, velo[0], velo[1], velo[2]);
	dBodySetAngularVel(body, omg[0], omg[1], omg[2]);
#else
	velo += impulse / mass;
	omg -= impulse.Cross(position) / moi;
#endif
}

// hit cussion; momentum and angular momentum trade with infinitely inertial object
void Ball::concussion(const Vector3d &contact, const Vector3d &n){
//	const Vec3d contact = Vec3d(-rad, 0, 0), n(1, 0, 0);
	if(1 < pos[1] + contact[1])
		return;
	Vector3d cvelo = contact.Cross(omg) - velo; // Velocity at Contact position
	double f = 1. - exp(-1e-3 * velo.Dot(n));
	receiveImpulse(cvelo * f, contact);
/*	double f = 1e-1 * floor_friction * velo.sp(n) / moi;
	Vec3d domg = contact.vp(cvelo) * (f);
	if(omg.slen() * 1.8 * 1.8 < domg.slen())
		domg *= 1.8 * omg.len() / domg.len();
	omg += domg;
	aaccel += domg;*/
}

void Ball::anim(Board &b, double dt){
	Ball (&balls)[sizeof(b.balls) / sizeof(Ball)] = b.balls;
	aaccel = Vector3d(0,0,0);
	pos += velo * dt;
/*	amat4_t mat;
	amat3_t omgt, nmat3;*/
	Quatd qomg, q, qbackup;

	if(trail1 == trail0 || .2 * .2 < (pos - trails[trail1]).LengthSq()){
		trail1 = (trail1+1) % (sizeof(trails) / sizeof*trails);
		if(trail1 == trail0)
			trail0 = (trail0+1) % (sizeof(trails) / sizeof*trails);
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
	rot.Rotate(omg * (dt / 2.));

/*	if(nmat){
		QUATCPY(qbackup, pt->rot);
		QUATCPY(pt->rot, *pq);
		tankrot(*nmat, pt);
		VECSADD(&(*nmat)[12], pt->velo, dt);
		QUATCPY(pt->rot, qbackup);
	}*/
	static const double G = 1.;
	static double elasticity = .5;
	velo[1] -= G * dt;
	if(pos[1] < 0.){
		pos[1] = 0.;
		velo[1] = velo[1] < -10. - G * dt ? -.5 * velo[1] : 0.;
		concussion(Vector3d(0, -rad, 0), Vector3d(0, 1, 0));
	}
	if(pos[0] - rad < b.x0){
		pos[0] = b.x0 + rad;
		velo[0] = velo[0] < 0 ? -elasticity * velo[0] : 0;
		concussion(Vector3d(-rad, 0, 0), Vector3d(1, 0, 0));
//		Vec3d cvelo = contact.vp(omg) - (velo - n * velo.sp(n)); // Velocity at Contact position
//		omg += contact.vp(cvelo) * (1e-2 * floor_friction * velo[0] / moi);
	}
	if(b.x1 < pos[0] + rad){
		pos[0] = b.x1 - rad;
		velo[0] = 0 < velo[0] ? -elasticity * velo[0] : 0;
		concussion(Vector3d(rad, 0, 0), Vector3d(-1, 0, 0));
	}
	if(pos[2] - rad < b.y0){
		pos[2] = b.y0 + rad;
		velo[2] = velo[2] < 0 ? -elasticity * velo[2] : 0;
		concussion(Vector3d(0, 0, -rad), Vector3d(0, 0, 1));
	}
	if(b.y1 < pos[2] + rad){
		pos[2] = b.y1 - rad;
		velo[2] = 0 < velo[2] ? -elasticity * velo[2] : 0;
		concussion(Vector3d(0, 0, rad), Vector3d(0, 0, -1));
	}
	if(pos[1] <= 0./* && velo[1] <= 0.*/){
		if(velo[1] < G * dt)
			velo[1] = 0.;
		const Vector3d contact = Vector3d(0, -rad, 0);
		Vector3d cvelo = contact.Cross(omg) - Vector3d(velo[0], 0, velo[2]); // Velocity at Contact position
		receiveImpulse(cvelo * (1. - exp(-2. * dt)) / (1. + .1 * velo.LengthSq()), contact);
/*		Vec3d domg = contact.vp(cvelo) * (1e-1 * floor_friction * dt / moi);
		omg += domg;
		aaccel += domg;
	//	velo += cvelo * (floor_friction * dt);
		velo = (cvelo * exp(-1e0 / floor_friction * dt) + velo);*/
		double svelo = velo.LengthSq();
		if(svelo < constant_friction * dt * constant_friction * dt)
			velo = Vector3d(0,0,0);
		else if(0. < svelo)
			velo *= (1. - constant_friction * dt / sqrt(svelo));
		double f = exp(-floor_friction * dt);
		omg *= f;
		f = exp(-1. * dt);
		omg[1] *= f;
//		omg[1] *= 1. / (1. + floor_friction * dt / moi);
	}
	omg *= 1. / (1. + constant_friction * dt / moi);
	int i;
	for(i = 0; i < sizeof(balls) / sizeof*balls; i++) if(&balls[i] != this && intersects(balls[i])){
		collide(balls[i]);
	}
	if(intersects(b.cue))
		collide(b.cue);
#endif

	if(model){
		Vector3f f(pos[0], pos[1], pos[2]);
		model->SetPosition(f);
		Quatf q(rot.x, rot.y, rot.z, rot.w);
		model->SetOrientation(q);
	}
}
