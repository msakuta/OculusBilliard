#include "Joystick.h"
#include "antiglut.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>


bool Joystick::InitJoystick()
{
	if(init)
		return true;
	// Make sure joystick driver is present
	UINT uiNumJoysticks;
	if ((uiNumJoysticks = joyGetNumDevs()) == 0)
		return false;

	// Make sure the joystick is attached
	JOYINFO jiInfo;
	unsigned i;
	for(i = m_uiJoystickID; i < uiNumJoysticks; i++){
		MMRESULT mr = joyGetPos(i, &jiInfo);
		if (mr == JOYERR_NOERROR){
			m_uiJoystickID = i;
			break;
		}
	}
	if(i == uiNumJoysticks){
		return false;
	}

	// Calculate the trip values
	JOYCAPS jcCaps;
	joyGetDevCaps(m_uiJoystickID, &jcCaps, sizeof(JOYCAPS));
	DWORD dwXCenter = ((DWORD)jcCaps.wXmin + jcCaps.wXmax) / 2;
	DWORD dwYCenter = ((DWORD)jcCaps.wYmin + jcCaps.wYmax) / 2;
	DWORD dwZCenter = ((DWORD)jcCaps.wZmin + jcCaps.wZmax) / 2;
	DWORD dwRCenter = ((DWORD)jcCaps.wRmin + jcCaps.wRmax) / 2;
	m_rcJoystickTrip.left = (jcCaps.wXmin + (WORD)dwXCenter) / 2;
	m_rcJoystickTrip.right = (jcCaps.wXmax + (WORD)dwXCenter) / 2;
	m_rcJoystickTrip.top = (jcCaps.wYmin + (WORD)dwYCenter) / 2;
	m_rcJoystickTrip.bottom = (jcCaps.wYmax + (WORD)dwYCenter) / 2;
	m_rcJoystickTrip2.left = (jcCaps.wZmin + (WORD)dwZCenter) / 2;
	m_rcJoystickTrip2.right = (jcCaps.wZmax + (WORD)dwZCenter) / 2;
	m_rcJoystickTrip2.top = (jcCaps.wRmin + (WORD)dwRCenter) / 2;
	m_rcJoystickTrip2.bottom = (jcCaps.wRmax + (WORD)dwRCenter) / 2;

	init = true;
	return true;
}

void Joystick::CaptureJoystick()
{
	// Capture the joystick
//	if (m_uiJoystickID == JOYSTICKID1)
//		joySetCapture(hWndApp, m_uiJoystickID, NULL, TRUE);
}

void Joystick::ReleaseJoystick()
{
	// Release the joystick
//	if (m_uiJoystickID == JOYSTICKID1)
//		joyReleaseCapture(m_uiJoystickID);
}

void Joystick::CheckJoystick(input_t &input)
{
	// Initialize the returned buffer for the cases of errors or absence of a joystick.
	for(int i = 0; i < 4; i++)
		input.analog[i] = 0;
	input.joyState = 0;
	input.joyChange = 0;

	if (m_uiJoystickID == JOYSTICKID1 || m_uiJoystickID == JOYSTICKID2){
		// Reserve old press state for taking differences
		int oldpress = input.press;
		JOYINFOEX jiInfo;
		JOYSTATE jsJoystickState = 0;

		// JOYINFOEX has members that need to be initialized before using.
		jiInfo.dwSize = sizeof jiInfo;
		jiInfo.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNBUTTONS;

		if(joyGetPosEx(m_uiJoystickID, &jiInfo) == JOYERR_NOERROR){
			// Check horizontal movement
			if (jiInfo.dwXpos < (WORD)m_rcJoystickTrip.left)
				jsJoystickState |= JOY_LEFT;
			else if (jiInfo.dwXpos > (WORD)m_rcJoystickTrip.right)
				jsJoystickState |= JOY_RIGHT;
			input.analog[0] = jiInfo.dwXpos / ((m_rcJoystickTrip.left + m_rcJoystickTrip.right) / 2.) - 1.;

			// Check vertical movement
			if (jiInfo.dwYpos < (WORD)m_rcJoystickTrip.top)
				jsJoystickState |= JOY_UP;
			else if (jiInfo.dwYpos > (WORD)m_rcJoystickTrip.bottom)
				jsJoystickState |= JOY_DOWN;
			input.analog[1] = jiInfo.dwYpos / ((m_rcJoystickTrip.top + m_rcJoystickTrip.bottom) / 2.) - 1.;

			// Check horizontal movement of right joystick
			if (jiInfo.dwZpos < (WORD)m_rcJoystickTrip2.left)
				jsJoystickState |= JOY_LEFT;
			else if (jiInfo.dwZpos > (WORD)m_rcJoystickTrip2.right)
				jsJoystickState |= JOY_RIGHT;
			input.analog[2] = jiInfo.dwZpos / ((m_rcJoystickTrip2.left + m_rcJoystickTrip2.right) / 2.) - 1.;

			// Check vertical movement of right joystick
			if (jiInfo.dwRpos < (WORD)m_rcJoystickTrip2.top)
				jsJoystickState |= JOY_UP;
			else if (jiInfo.dwRpos > (WORD)m_rcJoystickTrip2.bottom)
				jsJoystickState |= JOY_DOWN;
			input.analog[3] = jiInfo.dwRpos / ((m_rcJoystickTrip2.top + m_rcJoystickTrip2.bottom) / 2.) - 1.;

			// Check buttons
			if(jiInfo.dwButtons & JOY_BUTTON1)
				/*input.press |= PL_ENTER,*/ jsJoystickState |= JOY_FIRE1;
			if(jiInfo.dwButtons & JOY_BUTTON2)
				/*input.press |= PL_RCLICK,*/ jsJoystickState |= JOY_FIRE2;
			if(jiInfo.dwButtons & JOY_BUTTON3)
				jsJoystickState |= JOY_FIRE3;
			if(jiInfo.dwButtons & JOY_BUTTON4)
				jsJoystickState |= JOY_FIRE4;
			if(jiInfo.dwButtons & JOY_BUTTON5)
				jsJoystickState |= JOY_FIRE5;
			if(jiInfo.dwButtons & JOY_BUTTON6)
				jsJoystickState |= JOY_FIRE6;
			if(jiInfo.dwButtons & JOY_BUTTON7)
				jsJoystickState |= JOY_FIRE7;

			// Debug output
//			char buf[256];
//			sprintf(buf, "joy %d %d\r\n", jsJoystickState, m_joyState);
//			OutputDebugStringA(buf);

			input.joyState = jsJoystickState;
			input.joyChange = jsJoystickState ^ m_joyState;
			m_joyState = jsJoystickState;
		}
	}
}