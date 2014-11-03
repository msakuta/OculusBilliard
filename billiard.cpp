
#define USEWIN 1 /* whether use windows api (wgl*) */

#include "viewer.h"
#include "player.h"
#include "board.h"
#include "graphic.h"
//#include <unzip32.h>

extern "C"{
#include <clib/rseq.h>
#include <clib/timemeas.h>
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/aquat.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufdraw.h>
#include <clib/gl/gltext.h>
#include "bitmap.h"
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>


#if !USEWIN
#include <GL/glut.h>
#else
#include "antiglut.h"
#include <windows.h>
#include <commctrl.h>
#endif
#include <GL/gl.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#define exit something_meanless
/*#include <windows.h>*/
#undef exit
#include <GL/glext.h>

//#include <ode/ode.h>

/*
dWorldID world;
dSpaceID space;
*/

static double g_fix_dt = 0.;
static double gametimescale = 1.;
static double g_space_near_clip = 0.01, g_space_far_clip = 1e4;
bool mouse_captured = false;
static bool pause = false, charging = false;
static double power = 0.;
static double deviation[2];
static const double tableLength = 3.7 / 2.;
static const double tableWidth = tableLength / 2.;

enum directionset{LEFT = 1, RIGHT = 2, FORWARD = 4, BACKWARD = 8, DOWNWARD = 16, UPWARD = 32};
int keystate = 0;

Player pl;
Graphic gr;
Board board(-tableWidth, -tableLength, tableWidth, tableLength);

int selected = 0;
double view_dist = 2;
static bool freelook = false;
static bool g_wireframe = false;

static HDC hcdc;
static HBITMAP hbm;
static GLubyte *chbuf;

static HWND hPower, hPowerUpDown;

//extern "C" DWORD WINAPI GetGlyphIndicesW(HDC, LPCTSTR, int, LPWORD, DWORD);

void draw_text(const wchar_t *s){
/*	BITMAP bm;
	GetObject(hbm, sizeof bm, &bm);*/
	static bool init = false;
	if(!init){
		init = true;
		{
			HDC hwdc = GetDC(GetDesktopWindow());
			hcdc = CreateCompatibleDC(hwdc);
			struct BITMAPINFO2{
				BITMAPINFOHEADER biHeader;
				RGBQUAD biColors[2];
			} bi = {{
				sizeof (BITMAPINFO2), //DWORD  biSize; 
				32, //LONG   biWidth; 
				32, //LONG   biHeight; 
				1, //WORD   biPlanes; 
				1, //WORD   biBitCount; 
				BI_RGB, //DWORD  biCompression; 
				0, //DWORD  biSizeImage; 
				1000, //LONG   biXPelsPerMeter; 
				1000, //LONG   biYPelsPerMeter; 
				2, //DWORD  biClrUsed; 
				2, //DWORD  biClrImportant; 
			}, {0,0,0,0,255,255,255,0}};
			hbm = CreateDIBSection(hcdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&chbuf, NULL, 0);

			ReleaseDC(GetDesktopWindow(), hwdc);
		}
	}
	SelectObject(hcdc, hbm);
	SelectObject(hcdc, GetStockObject(DEFAULT_GUI_FONT));
	SetBkColor(hcdc, RGB(255,255,255));
	SetTextColor(hcdc, RGB(0,0,0));
/*	GLfloat rp[4];
	glGetFloatv(GL_CURRENT_RASTER_POSITION, rp);*/

	while(*s != L'\0'){
//		ZeroMemory(chbuf, 16 * 16 / 4);
		int w;
//		WORD ww;
//		char aa;
//		DWORD cbuf[1] = {0};
//		ABC abc;
		GCP_RESULTSW gr = {sizeof(GCP_RESULTSW)};
//		wctomb((char*)cbuf, *s);
//		GetGlyphIndicesW(hcdc, s, 1, &ww, GGI_MARK_NONEXISTING_GLYPHS);
//		GetCharWidthI(hcdc, ww, 1, NULL, &w);
//		GetCharABCWidths(hcdc, *cbuf, *cbuf, &abc);
//		gr.lpCaretPos = &w;
//		gr.lpClass = &aa;
		gr.nGlyphs = 1;
		gr.nMaxFit = 1;
		w = GetCharacterPlacementW(hcdc, s, 1, 1000, &gr, 0);
		w &= 0xffff;
		if(0 && 0 <= *s - 0x3041 && *s - 0x3041 < 100)
			glCallList(*s);
		else{
			RECT r = {0, 0, 32, 32};
//			int di[2];
			ExtTextOutW(hcdc, 0, 0, ETO_OPAQUE, &r, s, 1, NULL);
/*			glRecti(rp[0], rp[1], rp[0] + w, rp[1] + 20);
			rp[0] += w;*/
			glBitmap(32, 32, 0, 0, (GLfloat)w, 0, chbuf);
		}
		s++;
	}
}


void draw_func(Viewer &vw, double dt){
	glClearDepth(1.);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	if(freelook){
		vw.pos = pl.pos;
		glMultMatrixd(vw.rot);
		gldTranslate3dv(vw.pos);
	}
	else{
		// Smoothly interpolate view distance
		static double smoothDist = 1.;
		double f = exp(-dt * 10.);
		smoothDist = smoothDist * f + view_dist * (1. - f);
		Vec3d focusvec = Vec3d(0, -smoothDist * .035, -smoothDist);
		Vec3d mov = vw.irot.dvp3(focusvec);
		pl.pos = (-board.balls[selected].pos + mov);
		pl.velo = vec3_000;
		glMultMatrixd(vw.rot);
		gldTranslate3dv(pl.pos);
	}
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FLAT);
	board.draw();
	glPopAttrib();
	gldTranslate3dv(board.balls[selected].pos);
	if(0){
		Vec3d lookdir = pl.rot.itrans(vec3_001);
		glColor4ub(0,0,255,255);
		glBegin(GL_LINES);
		glVertex3d(0, -1, 0);
		glVertex3d(10 * lookdir[0], -1, 10 * lookdir[2]);
		glEnd();
	}
	if(charging){
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glMultMatrixd(vw.irot);
		glTranslated(deviation[0], deviation[1], Ball::defaultRadius);
		glColor4ub(255,0,0,127);
		int i;
		double (*cuts)[2] = CircleCuts(8);
		glBegin(GL_POLYGON);
		for(i = 0; i < 8; i++)
			glVertex2d(Ball::defaultRadius * .1 * cuts[i][0], Ball::defaultRadius * .1 * cuts[i][1]);
		glEnd();
		glPopAttrib();
	}
	glPopMatrix();
	glDisable(GL_LIGHTING);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
	glPushMatrix();
	glLoadIdentity();               /* define the projection */
	glMatrixMode (GL_MODELVIEW);    /* prepare for and then */ 
/*	glTranslated(-(double)vw.vp.w / vw.vp.m * .5, -(double)vw.vp.h / vw.vp.m * .5, 0);*/
	glColor4ub(255,255,255,255);
	glRasterPos2d(-1, -1);
	gldprintf("TS: %lg fps %lg", gametimescale, 1. / dt);
	glRasterPos2d(-1, -1 + 16. / vw.vp.h);
	int i;
	double total = 0.;
	for(i = 0; i < numof(board.balls); i++)
		total += board.balls[i].getEnergy();
	gldprintf("e: %8.8lg/%8.8lg", board.balls[selected].getEnergy(), total);
	glRasterPos2d(-1, -1 + 2 * 16. / vw.vp.h);
	gldprintf("c: %lg", power);
	glRasterPos2d(0, -1 + 2 * 16. / vw.vp.h);
	gldprintf("static: %d", (int)board.isStatic());

	glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);    /* prepare for and then */ 
	glPopMatrix();
}

/// The 'lathe' type method to update rotation of the viewing orientation.
/// We could have only euler angles, pitch, yaw and roll, in order to achieve this
/// type of response to mouse movements, but we had quaternion as the rotation
/// representation, so we have to settle for it.
void Player::rotateLook(double dx, double dy){
	const double speed = .001 / 2.;
	const double one_minus_epsilon = .999;

	Quatd rot = this->rot;

	// Fully calculate pitch, yaw and roll from quaternion.
	// This calculation is costly, but worth doing everytime mouse moves, for majority of screen is affected.
	Vec3d view = rot.itrans(vec3_001);
	double phi = -atan2(view[0], view[2]);
	double theta = atan2(view[1], sqrt(view[2] * view[2] + view[0] * view[0]));
	Quatd rot1 = Quatd::rotation(theta, 1, 0, 0).rotate(phi, 0, 1, 0);
	Quatd rot2 = rot * rot1.cnj();
	Vec3d right = rot2.itrans(vec3_100);

	// Add in instructed movement here.
	phi += -dx * speed;
	theta = rangein(theta + -dy * speed, -M_PI / 2. * one_minus_epsilon, M_PI / 2. * one_minus_epsilon);
	double roll = -atan2(right[1], right[0]);
	this->rot = Quatd::rotation(roll, 0, 0, 1).rotate(theta, 1, 0, 0).rotate(phi, 0, 1, 0);
}

static POINT mouse_pos = {0, 0};

void display_func(void){
	static int init = 0;
	static timemeas_t tm;
	static double gametime = 0.;
	double dt = 0.;
	if(!init){
		extern double dwo;
		init = 1;
/*
#ifdef _WIN32
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
		glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
		glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)wglGetProcAddress("glMultiTexCoord1fARB");
#endif
*/
//		anim_sun(0.);
		TimeMeasStart(&tm);
//		warf.soundtime = TimeMeasLap(&tmwo) - dwo;
	}
	else{
		int trainride = 0;
		double t1, rdt;

		t1 = TimeMeasLap(&tm);
		if(g_fix_dt)
			rdt = g_fix_dt;
		else
			rdt = (t1 - gametime) * gametimescale;

		dt = !init ? 0. : rdt < 1. ? rdt : 1.;

		if(mouse_captured){
			POINT p;
			if(GetCursorPos(&p) && (p.x != mouse_pos.x || p.y != mouse_pos.y)){
				if(charging){
					deviation[0] += (p.x - mouse_pos.x) * .001;
					deviation[1] -= (p.y - mouse_pos.y) * .001;
					if(1. < deviation[0] * deviation[0] + deviation[1] * deviation[1]){
						double s = 1. / ::sqrt(deviation[0] * deviation[0] + deviation[1] * deviation[1]);
						deviation[0] *= s;
						deviation[1] *= s;
					}
				}
				else{
					pl.rotateLook(p.x - mouse_pos.x, p.y - mouse_pos.y);
				}
				SetCursorPos(mouse_pos.x, mouse_pos.y);
//				mouse_pos = p;
			}
		}

		if(!pause){
			if(keystate & LEFT)
				pl.velo -= pl.rot.itrans(vec3_100) * dt * 1;
			if(keystate & RIGHT)
				pl.velo += pl.rot.itrans(vec3_100) * dt * 1;
			if(keystate & FORWARD)
				pl.velo -= pl.rot.itrans(vec3_001) * dt * 1;
			if(keystate & BACKWARD)
				pl.velo += pl.rot.itrans(vec3_001) * dt * 1;
			if(keystate & DOWNWARD)
				pl.velo -= pl.rot.itrans(vec3_010) * dt * 1;
			if(keystate & UPWARD)
				pl.velo += pl.rot.itrans(vec3_010) * dt * 1;
			pl.pos += pl.velo * dt;
			board.anim(dt);
			if(charging){
				power += dt;
				char buf[256];
				sprintf_s(buf, "%d", int(power * 100));
				SendMessage(hPowerUpDown, UDM_SETPOS32, 0, int(power * 100));
//				SetWindowText(hPower, buf);
			}
		}

		gametime = t1;
	}
	Viewer viewer;
	{
		double hack[16] = {
			1,0,0,0,
			0,1,0,0,
			0,0,10,0,
			0,0,0,1,
		};
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		viewer.vp.set(vp);
		double dnear = g_space_near_clip, dfar = g_space_far_clip;
/*		if(pl.cs->w && pl.cs->w->vft->nearplane)
			dnear = pl.cs->w->vft->nearplane(pl.cs->w);
		if(pl.cs->w && pl.cs->w->vft->farplane)
			dfar = pl.cs->w->vft->farplane(pl.cs->w);*/
		glMatrixMode (GL_PROJECTION);    /* prepare for and then */ 
		glLoadIdentity();               /* define the projection */
		viewer.frustum(dnear, dfar);  /* transformation */
/*		glMultMatrixd(hack);*/
		glGetDoublev(GL_PROJECTION_MATRIX, hack);
		glMatrixMode (GL_MODELVIEW);  /* back to modelview matrix */
/*		glDepthRange(.5,100);*/
	}
	viewer.rot = pl.rot.tomat4();
	viewer.irot = pl.rot.cnj().tomat4();
	draw_func(viewer, dt);
}

void mouse_func(int button, int state, int x, int y){

	if(state == GLUT_UP && button == GLUT_LEFT_BUTTON){
		int i;
		random_sequence rs;
		init_rseqf(&rs, clock());
		i = selected;
		Vec3d lookdir = pl.rot.itrans(-vec3_001);
		power = SendMessage(hPowerUpDown, UDM_GETPOS32, 0, 0) / 100.;

		// Cancel impulse downward
		if(board.balls[i].pos[1] <= 0. && lookdir[1] < 0.)
			lookdir[1] = 0.;

		board.balls[i].receiveImpulse(board.balls[i].getMass() * power * lookdir,
			pl.rot.itrans(board.balls[i].rad * Vec3d(-deviation[0], -deviation[1], 0)));
		board.cue.pos = board.balls[i].pos + pl.rot.itrans(Vec3d(-deviation[0], -deviation[1], -1)).norm().scale(board.balls[i].rad * 1.1);
		charging = false;
	}
	else if(state == GLUT_DOWN && button == GLUT_LEFT_BUTTON){
		charging = true;
		deviation[0] = deviation[1] = 0.;
		power = 1.;
	}

#if USEWIN && defined _WIN32
	if((!pl.control || !mouse_captured) && state == GLUT_UP && button == GLUT_RIGHT_BUTTON){
		mouse_captured = !mouse_captured;
		if(mouse_captured){
			int c;
			HWND hwd;
			RECT r;
			hwd = GetActiveWindow();
			GetClientRect(hwd, &r);
			mouse_pos.x = (r.left + r.right) / 2;
			mouse_pos.y = (r.top + r.bottom) / 2;
			ClientToScreen(hwd, &mouse_pos);
			SetCursorPos(mouse_pos.x, mouse_pos.y);
			c = ShowCursor(FALSE);
			while(0 <= c)
				c = ShowCursor(FALSE);
		}
		else{
			while(ShowCursor(TRUE) < 0);
		}
	}
#endif
}

void reshape_func(int w, int h)
{
	int m = w < h ? h : w;
	glViewport(0, 0, w, h);
//	g_width = w;
//	g_height = h;
//	g_max = m;
}


#if USEWIN && defined _WIN32
static HGLRC wingl(HWND hWnd, HDC *phdc){
	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		PFD_DRAW_TO_WINDOW |   // support window 
		PFD_SUPPORT_OPENGL |   // support OpenGL 
		PFD_DOUBLEBUFFER,      // double buffered 
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // 24-bit color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // no alpha buffer 
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		32,                    // 32-bit z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	}; 
	int  iPixelFormat; 
	HGLRC hgl;
	HDC hdc;

	hdc = GetDC(hWnd);

	// get the best available match of pixel format for the device context  
	iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
		
	// make that the pixel format of the device context 
	SetPixelFormat(hdc, iPixelFormat, &pfd);

	hgl = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hgl);

	*phdc = hdc;

	return hgl;
}

static HGLRC winglstart(HDC hdc){
	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                     // version number 
		PFD_DRAW_TO_WINDOW |   // support window 
		PFD_SUPPORT_OPENGL |   // support OpenGL 
		PFD_DOUBLEBUFFER,      // double buffered 
		PFD_TYPE_RGBA,         // RGBA type 
		24,                    // 24-bit color depth 
		0, 0, 0, 0, 0, 0,      // color bits ignored 
		0,                     // no alpha buffer 
		0,                     // shift bit ignored 
		0,                     // no accumulation buffer 
		0, 0, 0, 0,            // accum bits ignored 
		32,                    // 32-bit z-buffer 
		0,                     // no stencil buffer 
		0,                     // no auxiliary buffer 
		PFD_MAIN_PLANE,        // main layer 
		0,                     // reserved 
		0, 0, 0                // layer masks ignored 
	}; 
	int  iPixelFormat; 
	HGLRC hgl;

	// get the best available match of pixel format for the device context  
	iPixelFormat = ChoosePixelFormat(hdc, &pfd); 
		
	// make that the pixel format of the device context 
	SetPixelFormat(hdc, iPixelFormat, &pfd);

	hgl = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hgl);

	return hgl;
}

static void winglend(HGLRC hgl){
	wglMakeCurrent (NULL, NULL) ;
	if(!wglDeleteContext(hgl))
		fprintf(stderr, "eeee!\n");
}
#endif

static LRESULT WINAPI CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	static HDC hdc;
	static HGLRC hgl = NULL;
	static int moc = 0;
	switch(message){
		case WM_CREATE:
			hgl = wingl(hWnd, &hdc);
			if(!SetTimer(hWnd, 2, 10, NULL))
				assert(0);
			{
				INITCOMMONCONTROLSEX cc = {sizeof(INITCOMMONCONTROLSEX), ICC_UPDOWN_CLASS };
				InitCommonControlsEx(&cc);
			}
			hPower = CreateWindow("EDIT", "a", WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 10, 100, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
			hPowerUpDown = CreateWindow(UPDOWN_CLASS, "", WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT, 110, 10, 20, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
			SendMessage(hPowerUpDown, UDM_SETBUDDY, WPARAM(hPower), 0);
			SendMessage(hPowerUpDown, UDM_SETRANGE32, 1, 10000);
			SendMessage(hPowerUpDown, UDM_SETPOS32, 0, 1);
			{
				UDACCEL a[3] = {{0, 1}, {1, 5}, {2, 10}};
				SendMessage(hPowerUpDown, UDM_GETACCEL, numof(a), LPARAM(&a));
				SendMessage(hPowerUpDown, UDM_SETACCEL, numof(a), LPARAM(&a));
			}
			break;

		case UDN_DELTAPOS:
			{
				LPNMUPDOWN ud;
				char buf[256];
				ud = (LPNMUPDOWN)lParam;
				sprintf_s(buf, "%d", ud->iPos);
				SetWindowText(hPower, buf);
			}
			break;

		case WM_TIMER:
			if(hgl){
				HDC hdc;
				hdc = GetDC(hWnd);
				display_func();
				wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
				ReleaseDC(hWnd, hdc);
			}
			break;

		case WM_SIZE:
			if(hgl){
				reshape_func(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_RBUTTONDOWN:
			SetFocus(hWnd);
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONDOWN:
			SetFocus(hWnd);
			mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_RBUTTONUP:
			mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_LBUTTONUP:
			mouse_func(GLUT_LEFT_BUTTON, GLUT_UP, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_MOUSEWHEEL:
			if((short)HIWORD(wParam) < 0)
				view_dist *= 1.2;
			else
				view_dist /= 1.2;
			return 0;

		case WM_CHAR:
			switch(wParam){
				case 'z':
					selected = (selected + 1) % numof(board.balls); break;
				case '[':
					gametimescale *= 2.; break;
				case ']':
					gametimescale /= 2.; break;
				case 't':
					gr.show_trail = !gr.show_trail; break;
				case 'y':
					gr.show_velo = !gr.show_velo; break;
				case 'p':
					pause = !pause; break;
				case 'b':
					board.init(); break;
				case 'u':
					freelook = !freelook; break;
				case 'e':
					pl.velo = vec3_000; break;
				case 'v':
					g_wireframe = !g_wireframe; break;
			}
			return 0;

		/* technique to enable momentary key commands */
		case WM_KEYUP:
//			BindKeyUp(toupper(wParam));
			switch(wParam){
//			case VK_DELETE: BindKeyUp(DELETEKEY); break;
//			case VK_ESCAPE: BindKeyUp(ESC); break;
			case VK_ESCAPE: if(mouse_captured){
				mouse_captured = false;
				while(ShowCursor(TRUE) < 0);
			}
//			case VK_SHIFT: BindKeyUp(1); break;
//			case VK_CONTROL: BindKeyUp(2); break;
				case 'W': keystate &= ~FORWARD; break;
				case 'S': keystate &= ~BACKWARD; break;
				case 'A': keystate &= ~LEFT; break;
				case 'D': keystate &= ~RIGHT; break;
				case 'Q': keystate &= ~UPWARD; break;
				case 'Z': keystate &= ~DOWNWARD; break;
			}
			break;

		case WM_KEYDOWN:
			switch(wParam){
				case 'W': keystate |= FORWARD; break;
				case 'S': keystate |= BACKWARD; break;
				case 'A': keystate |= LEFT; break;
				case 'D': keystate |= RIGHT; break;
				case 'Q': keystate |= UPWARD; break;
				case 'Z': keystate |= DOWNWARD; break;
			}
			break;

		/* TODO: don't call every time the window defocus */
/*		case WM_KILLFOCUS:
			BindKillFocus();
			break;*/

		case WM_DESTROY:
			KillTimer(hWnd, 2);

			if(moc){
				ReleaseCapture();
				while(ShowCursor(TRUE) < 0);
				ClipCursor(NULL);
			}

			winglend(hgl);

			PostQuitMessage(0);
			break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

#if defined _WIN32
HWND hWndApp;
#endif

int main(int argc, char *argv[])
{
	board.init();

#if USEWIN
	{
		MSG msg;
		HWND hWnd;
		ATOM atom;
		HINSTANCE hInst;
		RECT rc = {100,100,100+640,100+480};
/*		RECT rc = {0,0,0+512,0+384};*/
		hInst = GetModuleHandle(NULL);
//		wglUseFontBitmaps();
//		wglUseFontOutlines();

		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX); 

			wcex.style			= CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc	= (WNDPROC)WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= hInst;
			wcex.hIcon			= LoadIcon(hInst, IDI_APPLICATION);
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= NULL;
			wcex.lpszMenuName	= NULL;
			wcex.lpszClassName	= "gltest";
			wcex.hIconSm		= LoadIcon(hInst, IDI_APPLICATION);

			atom = RegisterClassEx(&wcex);
		}


		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION, FALSE);

		hWndApp = hWnd = CreateWindow(LPCTSTR(atom), "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
/*		hWnd = CreateWindow(atom, "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			100, 100, 400, 400, NULL, NULL, hInst, NULL);*/

		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#else
	glutMainLoop();
#endif

	while(ShowCursor(TRUE) < 0);
	while(0 <= ShowCursor(FALSE));

	return 0;
}

#if 0 && defined _WIN32

HINSTANCE g_hInstance;

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, int nCmdShow) 
{
	g_hInstance = hinstance;
	return main(1, lpCmdLine);
}
#endif
