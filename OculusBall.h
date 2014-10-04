#ifndef OCULUSBALL_H
#define OCULUSBALL_H
#include "Kernel/OVR_Math.h"
#include "Kernel/OVR_Array.h"
#include "Kernel/OVR_String.h"
#include "Kernel/OVR_Color.h"

#include "RenderTiny_D3D11_Device.h"

#define USEODE 0

#if USEODE
#include <ode/ode.h>
#endif

using namespace OVR;

class Board;

class Ball{
public:
	Vector3d pos;
	Vector3d velo;
	Vector3d omg;
	Vector3d aaccel;
	Quatd rot;
	double rad;
	Model *model;
#if USEODE
	dBodyID body;
	dGeomID geom;
#endif
	void init(const Vector3d &pos);
	Ball() : pos(0, 0, 0), velo(0, 0, 0), rot(0,0,0,1), omg(0,0,0), model(NULL){ init(pos); }
	Ball(const Vector3d &pos, const Vector3d &velo) : pos(pos), velo(velo), rot(0,0,0,1), omg(0,0,0), model(NULL){ init(pos); }
	Ball(const Vector3d &pos, const Vector3d &velo, const Vector3d &omg) : pos(pos), velo(velo), rot(0,0,0,1), omg(omg), model(NULL){ init(pos); }
	void anim(Board &b, double dt);
	void draw(const Board &b)const, drawtra(const Board &b)const, drawShadow(const Board &b)const;
	void receiveImpulse(const Vector3d &impulse, const Vector3d &position);
	double getEnergy()const{
		return velo.LengthSq() * mass + omg.LengthSq() * moi;
	}
private:
	void collide(Ball &o);
	Vector3d trails[128];
	int trail0, trail1;
	static const double mass;
	static const double moi; // moment of inertia
	void concussion(const Vector3d &contact, const Vector3d &n);
	bool intersects(const Ball &o)const{
		return (pos - o.pos).LengthSq() < (rad + o.rad) * (rad + o.rad) && (pos - o.pos).Dot(velo - o.velo) < 0.;
	}
};

#endif
