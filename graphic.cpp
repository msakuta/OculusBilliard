#include "graphic.h"
#include "board.h"
#include "ball.h"
#include <math.h>
#include "bitmap.h"

extern "C"{
#include <clib/c.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufdraw.h>
#include <clib/avec3.h>
}

#include "antiglut.h"
#include <windows.h>
#include <GL/glext.h>
#include <GL/glu.h>

static void drawShadeSphere(){
	int n = 32, slices, stacks;
	double (*cuts)[2];
	cuts = CircleCuts(n);
	glBegin(GL_QUADS);
	for(slices = 0; slices < n; slices++) for(stacks = 0; stacks < n/2; stacks++){
		int stacks1 = (stacks+1) % n, slices1 = (slices+1) % n;
		int m;
		void (WINAPI *glproc[3])(GLdouble, GLdouble, GLdouble) = {glNormal3d, glTexCoord3d, glVertex3d};
		for(m = 0; m < 3; m++) glproc[m](cuts[stacks][0] * cuts[slices][0], cuts[stacks][1], cuts[stacks][0] * cuts[slices][1]);
		for(m = 0; m < 3; m++) glproc[m](cuts[stacks1][0] * cuts[slices][0], cuts[stacks1][1], cuts[stacks1][0] * cuts[slices][1]);
		for(m = 0; m < 3; m++) glproc[m](cuts[stacks1][0] * cuts[slices1][0], cuts[stacks1][1], cuts[stacks1][0] * cuts[slices1][1]);
		for(m = 0; m < 3; m++) glproc[m](cuts[stacks][0] * cuts[slices1][0], cuts[stacks][1], cuts[stacks][0] * cuts[slices1][1]);
	}
	glEnd();
}

static void (WINAPI *const glproc[3])(const GLdouble *) = {glNormal3dv, glTexCoord3dv, glVertex3dv};

static void drawOctSphereInt(int level, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2){
	Vec3d p01 = (p0 + p1).norm(), p12 = (p1 + p2).norm(), p20 = (p2 + p0).norm();
	const Vec3d *faces[][3] = {
		{&p0, &p01, &p20},
		{&p01, &p1, &p12},
		{&p12, &p2, &p20},
		{&p01, &p12, &p20},
	};
	int i;
	for(i = 0; i < numof(faces); i++) if(level){
		drawOctSphereInt(level - 1, *faces[i][0], *faces[i][1], *faces[i][2]);
	}
	else{
		int m;
		for(m = 0; m < 3; m++) glproc[m](*faces[i][0]);
		for(m = 0; m < 3; m++) glproc[m](*faces[i][1]);
		for(m = 0; m < 3; m++) glproc[m](*faces[i][2]);
	}
}

static void drawOctSphere(){
	static const Vec3d verts[] = {
		Vec3d(-1,0,0), Vec3d(0,0,-1), Vec3d(0,-1,0),
		Vec3d(1,0,0), Vec3d(0,0,1), Vec3d(0,1,0),
	};
	static const Vec3d *const faces[][3] = {
		{&verts[0], &verts[1], &verts[2]},
		{&verts[1], &verts[0], &verts[5]},
		{&verts[1], &verts[3], &verts[2]},
		{&verts[3], &verts[1], &verts[5]},
		{&verts[3], &verts[4], &verts[2]},
		{&verts[4], &verts[3], &verts[5]},
		{&verts[4], &verts[0], &verts[2]},
		{&verts[0], &verts[4], &verts[5]},
	};
	glBegin(GL_TRIANGLES);
	int i;
	for(i = 0; i < numof(faces); i++){
		drawOctSphereInt(2, *faces[i][0], *faces[i][1], *faces[i][2]);
	}
	glEnd();
}

void Ball::draw(const Board &b)const{
	static GLuint texlist[16] = {0};
	static bool init = false;
	int ind = (this - b.balls) % numof(texlist);
	const double genfunc[2][4] = {
		{1,0,0,0},
		{0,0,1,0},
	};
	if(!init){
		init = true;
		GLuint last_active = 0;
		int i;
		for(i = 0; i < numof(texlist); i++){
			char buf[64];
			sprintf(buf, i ? "images/ball%d.png" : "images/cueball.png", i);
			void (*pngFree)(BITMAPINFO*);
			BITMAPINFO *bi = ReadPNG(buf, &pngFree);
			if(!bi || !(texlist[i] = CacheSUFTex(buf, bi, 0)))
				texlist[i] = last_active;
			else
				last_active = texlist[i];
			pngFree(bi);
		}
	}
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT);
	glCallList(texlist[ind]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGendv(GL_S, GL_OBJECT_PLANE, genfunc[0]);
	glTexGendv(GL_T, GL_OBJECT_PLANE, genfunc[1]);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glPushMatrix();
	gldTranslate3dv(pos);
	gldMultQuat(rot);
	if(rad != 1.)
		gldScaled(rad);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glTranslated(.5, .5, 0);
	glScaled(.5, .5, 1);
//	drawShadeSphere();
//	drawOctSphere();
	gldOctSphere(3);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	if(gr.show_velo | gr.show_trail){
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
	}
	if(gr.show_velo){
		Vec3d cvelo = Vec3d(0, -1, 0).vp(omg) + velo;
		glBegin(GL_LINES);
		glColor4ub(255,0,0,255);
		glVertex3d(pos[0], -1, pos[2]);
		glVertex3d(pos[0] + cvelo[0] * 10, -1 + cvelo[1] * 10, pos[2] + cvelo[2] * 10);
		glColor4ub(255,255,0,255);
		glVertex3dv(pos);
		glVertex3dv(pos + omg);
		glColor4ub(0,255,255,255);
		glVertex3dv(pos);
		glVertex3dv(pos + aaccel);
		glEnd();
	}
	if(gr.show_trail){
		glBegin(GL_LINE_STRIP);
		int i, j;
		for(j = 0, i = trail0; i != trail1; j++, i = (i + 1) % numof(trails)){
			glColor4ub(j * 256 / numof(trails),0,255,255);
			glVertex3dv(trails[i]);
		}
		glEnd();
	}
	glPopAttrib();

}

void Ball::drawShadow(const Board &b)const{
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glColor4ub(0,0,0,127);
	glBegin(GL_POLYGON);
	double (*cuts)[2] = CircleCuts(16);
	int i;
	for(i = 0; i < 16; i++)
		glVertex3d(pos[0] + cuts[i][0] * rad, -Ball::defaultRadius, pos[2] + cuts[i][1] * rad);
	glEnd();
	glPopAttrib();
}

void lightOn(){
	GLfloat light_pos[2][4] = {{1, 2, 1, 0}, {-2.1f, 2, -1.3f, 0}, };
	GLfloat color[] = {.5f, .5f, .5f, 1.f}, amb[] = {.2f, .2f, .2f, 1.f};
	GLfloat spec[] = {1,1,1,1};
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos[0]);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light_pos[1]);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, color);
	glLightfv(GL_LIGHT1, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
	glEnable(GL_NORMALIZE);

	// Material Settings
	GLfloat mat_specular[] = {0.5, 0.5, 0.5, 1.}/*{ 1., 1., .1, 1.0 }*/;
	GLfloat mat_diffuse[] = { .8f, .8f, .8f, 1.0 };
	GLfloat mat_ambient_color[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_color);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

static const double railHeight = 2 * Ball::defaultRadius;
static double beginVertex[3];
static bool beginVertexAvailable = false;

static GLvoid WINAPI begin(GLenum type, void *data){
	glBegin(data ? GL_QUAD_STRIP : type);
	beginVertexAvailable = false;
}

static void  WINAPI end(void *data){
	double *vd = beginVertex;
	if(beginVertexAvailable){
		glVertex3d(vd[0], vd[1], vd[2]);
		glVertex3d(vd[0], vd[1] - railHeight, vd[2]);
	}
	glEnd();
}

static void  WINAPI vertex(void *vd){
//	printf("%lg %lg %lg\n", ((double*)vd)[0], ((double*)vd)[1], ((double*)vd)[2]);
	glTexCoord2d(((double*)vd)[0] / 5., ((double*)vd)[2] / 5.);
	glVertex3dv((GLdouble*)vd);
}

static void  WINAPI vertex2(GLdouble *vd){
//	printf("%lg %lg %lg\n", ((double*)vd)[0], ((double*)vd)[1], ((double*)vd)[2]);
	glTexCoord2d(((double*)vd)[0] / 5., ((double*)vd)[2] / 5.);
	glNormal3d(vd[2], 0, -vd[0]);
	glVertex3d(vd[0], vd[1], vd[2]);
	glVertex3d(vd[0], vd[1] - railHeight, vd[2]);
	if(!beginVertexAvailable){
		beginVertexAvailable = true;
		VECCPY(beginVertex, vd);
	}
}

static GLuint ReadPNGTexture(const char *path){
	void (*pngFree)(BITMAPINFO*);
	BITMAPINFO *bi = ReadPNG(path, &pngFree);
	GLuint ret = 0;
	if(bi){
		ret = CacheSUFTex(path, bi, 0);
		pngFree(bi);
	}
	return ret;
}

void Board::draw(){
	static GLuint texlist = 0, texfloor = 0, texwood = 0, texfield = 0, texlogo = 0;
	if(!texlist)
		texlist = ReadPNGTexture("images\\wall.png");
	if(!texfloor)
		texfloor = ReadPNGTexture("images\\floor.png");
	if(!texwood)
		texwood = ReadPNGTexture("images\\wood.png");
	if(!texfield)
		texfield = ReadPNGTexture("images\\field.png");
	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	lightOn();

	static const double bx0 = -5, bx1 = 5, by0 = -10, by1 = 10, bz0 = -0.7, bz1 = 3;
	glPushAttrib(GL_TEXTURE_BIT);
	glCallList(texlist);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	GLfloat white[4] = {1,1,1,1};
	glColor4fv(white);
	glBegin(GL_QUADS);

	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0); glVertex3d(bx0, bz0, by0);
	glTexCoord2d(1, 0); glVertex3d(bx1, bz0, by0);
	glTexCoord2d(1, 1); glVertex3d(bx1, bz1, by0);
	glTexCoord2d(0, 1); glVertex3d(bx0, bz1, by0);

	glNormal3d(-1, 0, 0);
	glTexCoord2d(0, 0); glVertex3d(bx1, bz0, by0);
	glTexCoord2d(1, 0); glVertex3d(bx1, bz0, by1);
	glTexCoord2d(1, 1); glVertex3d(bx1, bz1, by1);
	glTexCoord2d(0, 1); glVertex3d(bx1, bz1, by0);

	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0); glVertex3d(bx1, bz0, by1);
	glTexCoord2d(1, 0); glVertex3d(bx0, bz0, by1);
	glTexCoord2d(1, 1); glVertex3d(bx0, bz1, by1);
	glTexCoord2d(0, 1); glVertex3d(bx1, bz1, by1);

	glNormal3d(1, 0, 0);
	glTexCoord2d(0, 0); glVertex3d(bx0, bz0, by1);
	glTexCoord2d(1, 0); glVertex3d(bx0, bz0, by0);
	glTexCoord2d(1, 1); glVertex3d(bx0, bz1, by0);
	glTexCoord2d(0, 1); glVertex3d(bx0, bz1, by1);

	glNormal3d(0, -1, 0);
	glTexCoord2d(0, 0); glVertex3d(bx0, bz1, by0);
	glTexCoord2d(1, 0); glVertex3d(bx1, bz1, by0);
	glTexCoord2d(1, 1); glVertex3d(bx1, bz1, by1);
	glTexCoord2d(0, 1); glVertex3d(bx0, bz1, by1);

	glEnd();

	glCallList(texfloor);

	glBegin(GL_QUADS);

	glNormal3d(0, 1, 0);
	glTexCoord2d(0, 0); glVertex3d(bx0, bz0, by0);
	glTexCoord2d(0, 10); glVertex3d(bx0, bz0, by1);
	glTexCoord2d(10, 10); glVertex3d(bx1, bz0, by1);
	glTexCoord2d(10, 0); glVertex3d(bx1, bz0, by0);

	glEnd();

#define glTessVertex3d(x,y,z) (tmpv[tmpi][0] = (x), tmpv[tmpi][1] = (y), tmpv[tmpi][2] = (z), gluTessVertex(tobj, tmpv[tmpi], tmpv[tmpi]), tmpi++)

	int i;
	int x, y;
	static GLuint boardmodel = 0;
	static GLUtesselator *tobj = NULL;
	const double holerad = Ball::defaultRadius * 2.;
	const double border = 0.2;
	const GLfloat aad[4] = {1, 1, 1, 1};
	double (*cuts)[2] = CircleCuts(8);
	if(!boardmodel){
		glNewList(boardmodel = glGenLists(1), GL_COMPILE);
		tobj = gluNewTess();
		static GLdouble tmpv[256][3];
		int tmpi = 0;
		int n;
		gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (WINAPI*)())vertex);
		gluTessCallback(tobj, GLU_TESS_BEGIN_DATA, (GLvoid (WINAPI*)())begin);
		gluTessCallback(tobj, GLU_TESS_END_DATA, (GLvoid (WINAPI*)())end);
		gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);

	//	glPolygonMode(GL_FRONT, GL_LINE);

		glNormal3d(0, -1, 0);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, aad);
		glCallList(texwood);
		for(n = 0; n < 2; n++){
			tmpi = 0;
			if(n == 1){
				gluTessProperty(tobj, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
				gluTessCallback(tobj, GLU_TESS_BEGIN_DATA, (GLvoid (WINAPI*)())begin);
				gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (WINAPI*)())vertex2);
			}
			gluTessBeginPolygon(tobj, (void*)n);
			gluTessBeginContour(tobj);
			glTessVertex3d(x0 - border, 0, y0 - border);
			glTessVertex3d(x0 - border, 0, y1 + border);
			glTessVertex3d(x1 + border, 0, y1 + border);
			glTessVertex3d(x1 + border, 0, y0 - border);
			gluTessEndContour(tobj);
			gluTessBeginContour(tobj);
			double cn = 0;
			glTessVertex3d(x0 - cn, 0, y0 - cn);
			glTessVertex3d(x1 + cn, 0, y0 - cn);
			glTessVertex3d(x1 + cn, 0, y1 + cn);
			glTessVertex3d(x0 - cn, 0, y1 + cn);
			gluTessEndContour(tobj);
			for(y = 0; y < 3; y++) for(x = 0; x < 2; x++){
				double dx = x ? x1 : x0, dy = y == 0 ? y0 : y == 1 ? 0. : y1;
				gluTessBeginContour(tobj);
				for(i = 7; 0 <= i; i--)
					glTessVertex3d(dx + cuts[i][0] * holerad, 0, dy + cuts[i][1] * holerad);
				gluTessEndContour(tobj);
			}
			gluTessEndPolygon(tobj);
		}
		glEndList();
	}

//	gluDeleteTess(tobj);

	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);

	// Table cloth
	glCallList(texfield);
	const GLfloat field_aad[4] = {.2f, .5f, .5f, 1.f};
	const GLfloat spec[] = {0,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, field_aad);
	glNormal3d(0, 1, 0);
	glBegin(GL_QUADS);
	glTexCoord2d(x0, y0); glVertex3d(x0, -Ball::defaultRadius, y0);
	glTexCoord2d(x0, y1); glVertex3d(x0, -Ball::defaultRadius, y1);
	glTexCoord2d(x1, y1); glVertex3d(x1, -Ball::defaultRadius, y1);
	glTexCoord2d(x1, y0); glVertex3d(x1, -Ball::defaultRadius, y0);
	glEnd();

	glPopAttrib();

	// Pocket holes
	glDisable(GL_LIGHTING);
	glColor4ub(0,0,0,255);
	for(y = 0; y < 3; y++) for(x = 0; x < 2; x++){
		double dx = x ? x1 : x0, dy = y == 0 ? y0 : y == 1 ? 0. : y1;
		glBegin(GL_POLYGON);
		for(i = 0; i < 8; i++)
			glVertex3d(dx + cuts[i][0] * holerad, -Ball::defaultRadius, dy + cuts[i][1] * holerad);
		glEnd();
	}

	// Ball shadows
	for(i = 0; i < numof(balls); i++){
		balls[i].drawShadow(*this);
	}

	glEnable(GL_DEPTH_TEST);

	// Paint z-buffer with table's geometry
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glBegin(GL_QUADS);
	glTexCoord2d(x0 / 5, y0 / 5); glVertex3d(x0 - border, -Ball::defaultRadius, y0 - border);
	glTexCoord2d(x0 / 5, y1 / 5); glVertex3d(x0 - border, -Ball::defaultRadius, y1 + border);
	glTexCoord2d(x1 / 5, y1 / 5); glVertex3d(x1 + border, -Ball::defaultRadius, y1 + border);
	glTexCoord2d(x1 / 5, y0 / 5); glVertex3d(x1 + border, -Ball::defaultRadius, y0 - border);
	glEnd();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	lightOn();

	// Sights on rails on long edge
	static const double pointrad = 0.01;
	static const double pointheight = 0.001; // Shold be drawn as decals on rails
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, aad);
	for(y = 1; y < 4; y++) for(x = 0; x < 2; x++){
		double dx = x ? x1 + border / 2 : x0 - border / 2, dy = y0 * y / 4;
		glBegin(GL_POLYGON);
		for(i = 0; i < 8; i++)
			glVertex3d(dx + cuts[i][0] * pointrad, pointheight, dy + cuts[i][1] * pointrad);
		glEnd();
		glBegin(GL_POLYGON);
		dy *= -1;
		for(i = 0; i < 8; i++)
			glVertex3d(dx + cuts[i][0] * pointrad, pointheight, dy + cuts[i][1] * pointrad);
		glEnd();
	}

	// Sights on rails on short edge
	for(y = 0; y < 2; y++) for(x = 1; x <= 3; x++){
		double dx = (x1 * (4 - x) + x0 * x) / 4, dy = y == 0 ? y0 - border / 2 : y1 + border / 2;
		glBegin(GL_POLYGON);
		for(i = 0; i < 8; i++)
			glVertex3d(dx + cuts[i][0] * pointrad, pointheight, dy + cuts[i][1] * pointrad);
		glEnd();
	}

	glCallList(texwood);
	glCallList(boardmodel);

	static const double railBottom = -railHeight * 2.;
	glBegin(GL_QUADS);
	for(int i = 0; i < 2; i++){
		static const double heights[2] = {railBottom, -railHeight};
		int sig = i * 2 - 1;
		int f = i, s = 1 - i;
		glNormal3d(-i, 0, 0);
		glTexCoord2d(y0, heights[f]); glVertex3d(sig * (x0 - border), heights[f], y0 - border);
		glTexCoord2d(y0, heights[s]); glVertex3d(sig * (x0 - border), heights[s], y0 - border);
		glTexCoord2d(y1, heights[s]); glVertex3d(sig * (x0 - border), heights[s], y1 + border);
		glTexCoord2d(y1, heights[f]); glVertex3d(sig * (x0 - border), heights[f], y1 + border);
		glNormal3d(0, 0, -i);
		glTexCoord2d(x0, heights[f]); glVertex3d(x0 - border, heights[f], sig * (y1 + border));
		glTexCoord2d(x0, heights[s]); glVertex3d(x0 - border, heights[s], sig * (y1 + border));
		glTexCoord2d(x1, heights[s]); glVertex3d(x1 + border, heights[s], sig * (y1 + border));
		glTexCoord2d(x1, heights[f]); glVertex3d(x1 + border, heights[f], sig * (y1 + border));

		static const double flHeights[2] = {bz0, railBottom};
		glNormal3d(-1, 0, 0);
		glTexCoord2d(y0, flHeights[f]); glVertex3d(sig * (x0 + border / 2), flHeights[f], y0 + border / 2);
		glTexCoord2d(y0, flHeights[s]); glVertex3d(sig * (x0 + border / 2), flHeights[s], y0 + border / 2);
		glTexCoord2d(y1, flHeights[s]); glVertex3d(sig * (x0 + border / 2), flHeights[s], y1 - border / 2);
		glTexCoord2d(y1, flHeights[f]); glVertex3d(sig * (x0 + border / 2), flHeights[f], y1 - border / 2);
		glNormal3d(0, 0, -1);
		glTexCoord2d(x0, flHeights[f]); glVertex3d(x0 + border / 2, flHeights[f], sig * (y1 - border / 2));
		glTexCoord2d(x0, flHeights[s]); glVertex3d(x0 + border / 2, flHeights[s], sig * (y1 - border / 2));
		glTexCoord2d(x1, flHeights[s]); glVertex3d(x1 - border / 2, flHeights[s], sig * (y1 - border / 2));
		glTexCoord2d(x1, flHeights[f]); glVertex3d(x1 - border / 2, flHeights[f], sig * (y1 - border / 2));
	}
	glEnd();
	
	glPopAttrib();


	glDisable(GL_LIGHTING);

	glPopAttrib();

/*	for(i = (int)x0; i <= (int)x1; i++){
		glBegin(GL_LINES);
		glVertex3d(i, -1., y0);
		glVertex3d(i, -1., y1);
		glVertex3d(x0, -1., i);
		glVertex3d(x1, -1., i);
		glEnd();
	}*/

	glPushAttrib(GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	lightOn();
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	for(i = 0; i < numof(balls); i++){
		balls[i].draw(*this);
	}
//	cue.draw(*this);
	glPopAttrib();
}
