#ifndef BOARD_H
#define BOARD_H
#include "ball.h"

extern class Board{
public:
	double x0, y0, x1, y1;
	Ball balls[16], cue;
#ifdef OCULUSBILLIARD
	Vector3d headPos;
#endif
	Board(double ax0, double ay0, double ax1, double ay1);
	void init();
	void anim(double dt
#ifdef OCULUSBILLIARD
		, const Vector3f &headPos
#endif
		);
	void draw();
	bool isStatic()const;
	void shoot();
	static const double floor_friction;
	static const double constant_friction;
	static const double holerad;
} board;

#endif
