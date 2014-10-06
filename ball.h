#ifndef BALL_H
#define BALL_H
#include <cpplib/vec3.h>
#include <cpplib/quat.h>

#define USEODE 0

#if USEODE
#include <ode/ode.h>
#endif

class Board;

class Ball{
public:
	Vec3d pos;
	Vec3d velo;
	Vec3d omg;
	Vec3d aaccel;
	Quatd rot;
	double rad;
#if USEODE
	dBodyID body;
	dGeomID geom;
#endif
	void init(const Vec3d pos);
	Ball() : pos(vec3_000), velo(vec3_000), rot(quat_u), omg(vec3_000){ init(pos); }
	Ball(const Vec3d &pos, const Vec3d &velo) : pos(pos), velo(velo), rot(quat_u), omg(vec3_000){ init(pos); }
	Ball(const Vec3d &pos, const Vec3d &velo, const Vec3d &omg) : pos(pos), velo(velo), rot(quat_u), omg(omg){ init(pos); }
	void anim(Board &b, double dt);
	void draw(const Board &b)const, drawtra(const Board &b)const, drawShadow(const Board &b)const;
	void receiveImpulse(const Vec3d &impulse, const Vec3d &position);
	double getEnergy()const{
		return velo.slen() * mass + omg.slen() * moi;
	}
	double getMass()const{return mass;}

	static const double defaultRadius;
private:
	void collide(Ball &o);
	Vec3d trails[128];
	int trail0, trail1;
	static const double mass;
	static const double moi; // moment of inertia
	void concussion(const Vec3d &contact, const Vec3d &n);
	bool intersects(const Ball &o)const{
		return (pos - o.pos).slen() < (rad + o.rad) * (rad + o.rad) && (pos - o.pos).sp(velo - o.velo) < 0.;
	}
};

#endif
