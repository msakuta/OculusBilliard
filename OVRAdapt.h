#ifndef OVRADAPT_H
#define OVRADAPT_H

#include <cpplib/vec3.h>
#include "Kernel/OVR_Math.h"


class ovrvc : OVR::Vector3d{
public:
	ovrvc(const Vec3d &o) : OVR::Vector3d(o[0], o[1], o[2]){}
	ovrvc(const OVR::Vector3d &o) : OVR::Vector3d(o){}
	operator Vec3d(){
		return Vec3d(x, y, z);
	}
};

#endif
