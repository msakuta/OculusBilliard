#ifndef OCULUSBOARD_H
#define OCULUSBOARD_H
#include "OculusBall.h"

extern class Board{
public:
	double x0, y0, x1, y1;
	Ball balls[16], cue;
	Vector3d headPos;
	double globalTime;
	Board(double ax0, double ay0, double ax1, double ay1);
	void init();
	void anim(double dt, const Vector3f &headPos);
	void draw();
	bool isStatic()const;
	void shoot();
} board;

#endif
