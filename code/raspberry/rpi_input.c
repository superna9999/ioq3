/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include "../client/client.h"
#include "../sys/sys_local.h"

#define MOUSE_DEVICE "/dev/input/event0"
#define KEYBOARD_DEVICE "/dev/input/event1"

static int s_mouse_fd;
static int s_keyboard_fd;

static qboolean s_shift_mod;

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static qboolean IN_IsConsoleKey( keyNum_t key, int character )
{
	return qfalse;
}

/*
===============
IN_GobbleMotionEvents
===============
*/
static void IN_GobbleMotionEvents(void)
{
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( void )
{
}

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick( void )
{
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick( void )
{
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove(void)
{
}

#define ERROR_KEY 0x00

static const int s_scancode_table[] =
{
	ERROR_KEY, K_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', K_BACKSPACE, K_TAB,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', K_ENTER, K_CTRL, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', K_CONSOLE, K_SHIFT, '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/', K_SHIFT, K_KP_STAR, K_ALT, K_SPACE, K_CAPSLOCK, K_F1, K_F2, K_F3, K_F4, K_F5,
	K_F6, K_F7, K_F8, K_F9, K_F10, K_KP_NUMLOCK, K_SCROLLOCK, K_KP_HOME, K_KP_UPARROW, K_KP_PGUP, K_KP_MINUS, K_KP_LEFTARROW, K_KP_5, K_KP_RIGHTARROW, K_KP_PLUS, K_KP_END,
	K_KP_DOWNARROW, K_KP_PGDN, K_KP_INS, K_KP_DEL
};

/*
===============
IN_TranslateKey
===============
*/
static int IN_TranslateKey(int code)
{
	if(code <= 0x53)
		return s_scancode_table[code];

	printf("Keyboard unknown button '%u' pressed\n", code);
	return ERROR_KEY;
}

/*
===============
IN_TranslateCharUpper
===============
*/
static int IN_TranslateCharUpper(int code)
{
	if(code >= 'a' && code <= 'z')
		return code - 0x20;

	switch(code)
	{
		case '1':
			return '!';
		case '2':
			return '@';
		case '3':
			return '#';
		case '4':
			return '$';
		case '5':
			return '%';
		case '6':
			return '^';
		case '7':
			return '&';
		case '8':
			return '*';
		case '9':
			return '(';
		case '0':
			return ')';
		case '-':
			return '_';
		case '=':
			return '+';
		case '[':
			return '{';
		case ']':
			return '}';
		case '\\':
			return '|';
		case ';':
			return ':';
		case '\'':
			return '"';
		case ',':
			return '<';
		case '.':
			return '>';
		case '/':
			return '?';
	}

	return code;
}

/*
===============
IN_TranslateChar
===============
*/
static int IN_TranslateChar(int code)
{
	if(code == K_BACKSPACE)
		return CTRL('h');
	else if(code >= ' ' && code <= '}')
	{
		if(s_shift_mod)
			return IN_TranslateCharUpper(code);
		return code;
	}

	return ERROR_KEY;
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents(void)
{
	struct input_event event;

	while(read(s_mouse_fd, &event, sizeof(struct input_event)) > 0)
	{
		// Mouse Motion
		if(event.type == 2)
		{
			// X Movement
			if(event.code == 0)
			{
				Com_QueueEvent(0, SE_MOUSE, event.value, 0, 0, NULL);
			}
			// Y Movement
			else if(event.code == 1)
			{
				Com_QueueEvent(0, SE_MOUSE, 0, event.value, 0, NULL);
			}
			// Mouse Scroll
			else if(event.code == 8)
			{
				int button = (event.value == 1?K_MWHEELUP:K_MWHEELDOWN);
				Com_QueueEvent(0, SE_KEY, button, qtrue, 0, NULL);
				Com_QueueEvent(0, SE_KEY, button, qfalse, 0, NULL);
			}
		}
		// Mouse Button
		else if(event.type == 1)
		{
			int b;
			switch(event.code)
			{
				case 272:
					b = K_MOUSE1;
					break;
				case 273:
					b = K_MOUSE2;
					break;
				case 274:
					b = K_MOUSE3;
					break;
				default:
					printf("Mouse unknown button '%u' pressed\n", event.code);
					break;
			}

			Com_QueueEvent( 0, SE_KEY, b, (event.value == 1 ? qtrue : qfalse), 0, NULL);
		}
	}

	while(read(s_keyboard_fd, &event, sizeof(struct input_event)) > 0)
	{
		// Key Event
		if(event.type == 1)
		{
			int b = IN_TranslateKey(event.code);

			// Key Repeat Event
			if(b != ERROR_KEY)
			{
				if(event.value != 2)
				{
					qboolean pressed = (event.value == 1 ? qtrue : qfalse);

					if(b == K_SHIFT)
						s_shift_mod = pressed;

					Com_QueueEvent(0, SE_KEY, b, pressed, 0, NULL);
				}

				int asciiKey = IN_TranslateChar(b);
				if(asciiKey != ERROR_KEY && event.value != 0)
				{
					Com_QueueEvent(0, SE_CHAR, IN_TranslateChar(b), 0, 0, NULL);
				}
			}
		}
	}
}

/*
===============
IN_Frame
===============
*/
void IN_Frame(void)
{
	IN_ProcessEvents();
}

/*
===============
IN_Init
===============
*/
void IN_Init(void *windowData)
{
	s_mouse_fd = open(MOUSE_DEVICE, O_RDONLY);
	fcntl(s_mouse_fd, F_SETFL, O_NONBLOCK);

	s_keyboard_fd = open(KEYBOARD_DEVICE, O_RDONLY);
	fcntl(s_keyboard_fd, F_SETFL, O_NONBLOCK);

	s_shift_mod = qfalse;
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown(void)
{
}

/*
===============
IN_Restart
===============
*/
void IN_Restart(void)
{
}
