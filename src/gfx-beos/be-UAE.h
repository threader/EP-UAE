#ifndef BE_UAE_H
#define BE_UAE_H
<<<<<<< HEAD
/***********************************************************/ 
=======
/***********************************************************/
>>>>>>> p-uae/v2.1.0
//  UAE - The Un*x Amiga Emulator
//
//  BeOS port specific stuff
//
//  (c) 2000-2001 Axel Doerfler
//  (c) 1999 Be/R4 Sound - Raphael Moll
//  (c) 1998-1999 David Sowsy
//  (c) 1996-1998 Christian Bauer
//  (c) 1996 Patrick Hanevold
//
/***********************************************************/

#include <AppKit.h>
#include <InterfaceKit.h>
#include <StorageKit.h>
#include <support/SupportDefs.h>
#include <support/String.h>

<<<<<<< HEAD
#include <stdio.h> 
#include <stdlib.h> 
=======
#include <stdio.h>
#include <stdlib.h>
>>>>>>> p-uae/v2.1.0

// 0.8.17 BeOS Release 1
const int32 kBeUAERelease = 1;

class UAEWindow;

// Global variables
extern UAEWindow *gEmulationWindow;
//extern BString gSettingsName;

<<<<<<< HEAD
extern int inhibit_frame; 
=======
extern int inhibit_frame;
>>>>>>> p-uae/v2.1.0


class UAE : public BApplication {
public:
	UAE();

	void ReadyToRun();
	bool QuitRequested();
	void RefsReceived(BMessage *msg);

	status_t GetIconResource(char *name,icon_size size,BBitmap *dest) const;
	void RegisterMimeTypes();
	int GraphicsInit();
	void GraphicsLeave();

private:
	static long EmulationThreadFunc(void *obj);
	thread_id fEmulationThread;
<<<<<<< HEAD
	
=======

>>>>>>> p-uae/v2.1.0
	UAEWindow *fEmulationWindow;
};

extern void restoreWorkspaceResolution();

#endif

