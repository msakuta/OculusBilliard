#ifndef PLAYER_H
#define PLAYER_H
#include <cstring>
extern "C"{
#include <clib/aquat.h>
#include <clib/mathdef.h>
}
#include <cpplib/quat.h>

#define FEETD 0.001

typedef struct input input_t;
struct tank;
struct astrobj;
struct coordsys;
class Entity;

class Player{
public:
	Player(){
		std::memset(this, 0, sizeof *this);
		rot = Quatd::rotation(M_PI / 6., Vec3d(1, 0, 0));
	}
	Vec3d pos;
	Vec3d velo;
	double accel[3];
	double pyr[3], avelo[3];
	Quatd rot;
	double rad;
	struct coordsys *cs;
	void (*mover)(struct player *, input_t *inputs, double dt); /* virtual mover function */
	Entity *chase, *control, *selected, *lastchase;
	struct astrobj *sight;
	int chasecamera; /* multiple cameras can be mounted on a vehicle for having fun! */
	int trigger, detail, minimap;
	int mousex, mousey;
	int floortouch;
	int gear; /* acceleration gear in ghost mode */
	double gametime;
	double velolen; /* trivial; performance keeper */
	double height; /* trivial; indicates air pressure surrounding the player */

	void rotateLook(double dx, double dy);
};



#endif
