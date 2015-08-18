#include "FileSystem.h"
#include "nesGlobal.h"


u8  ROM_Cache[MAX_ROM_SIZE];
u8 *SRAM_Name;

u64	ROM_Size;

FS_archive sdmcArchive;

extern bool inGame;	
extern bool VSYNC;
extern bool CPU_Running;

extern u8	frameSkip;

void unicodeToChar(char* dst, u16* src) {
    if(!src || !dst)return;
    while(*src)*(dst++)=(*(src++))&0xFF;
    *dst=0x00;
}

 
void NES_LOADROMLIST() {
	Handle romHandle;
	
	FS_dirent dirStruct;
	FS_path dirPath = FS_makePath(PATH_CHAR, "/3DNES/ROMS");

	// init SDMC archive
	sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FSUSER_OpenDirectory(NULL, &romHandle, sdmcArchive, dirPath);

	// Get number of files in directory
	fileSystem.totalFiles = 0;
	while(1) {
		u32 dataRead = 0;
		FSDIR_Read(romHandle, &dataRead, 1, &dirStruct);
		if(dataRead == 0) break;
		fileSystem.totalFiles++;
	}

	fileSystem.fileList = linearAlloc(MAX_FILENAME_SIZE * fileSystem.totalFiles);

	FSUSER_OpenDirectory(NULL, &romHandle, sdmcArchive, dirPath);

	fileSystem.totalFiles = 0;
	while(1) {
		u32 dataRead = 0;
		FSDIR_Read(romHandle, &dataRead, 1, &dirStruct);
		if(dataRead == 0) break;
		unicodeToChar(&fileSystem.fileList[MAX_FILENAME_SIZE * fileSystem.totalFiles], dirStruct.name);
		fileSystem.totalFiles++;
	}

	FSDIR_Close(romHandle);
}

/* Draw All ROM's */
void NES_drawROMLIST() {
	int i = 0;

	for(i = 0; i < fileSystem.totalFiles; i++) {
		draw_string_c(55 + (i * 15), fileSystem.fileList[i * MAX_FILENAME_SIZE]);
	}

	draw_string(10, (fileSystem.cFile * 15) + 53, "->");

}

/* Draw Configuration Menu */
void NES_drawConfigurationMenu() {
	char gameFPS[18];


	draw_string(10, (fileSystem.cConfig* 15) + 70, "->");

	sprintf(gameFPS, "FrameSkip: %d", frameSkip);

	draw_string_c(50, "Configuration Menu");
	draw_string_c(73, gameFPS);
	
	if (VSYNC)
		draw_string_c(88, "VSYNC: ENABLE");
	else
		draw_string_c(88, "VSYNC: DISABLE");

	draw_string_c(118, "Exit and Start Game");

}

void FS_StringConc(char* dst, char* src1, char* src2) {
	int i = 0;
    int Size2 = strlen(src1);
    int Size3 = strlen(src2);


    for (i = 0; i < Size2; i++) {
        dst[i] = src1[i];
    }

    for (i = 0; i < Size3; i++) {
        dst[i + Size2] = src2[i];
    }

}

// TODO: 
// Here will need a complete re-work, because the files now are loaded
// in different way, so we need copy to the "filename stream" the correct 
// size of filename, another important thing is reset the cpu
// that include clear all registers

void NES_LoadSelectedGame() {
	u32    bytesRead = 0;
	u32    SRAM_Size = 0;
	u32    ROMDIR_Size = (strlen("/3DNES/ROMS/") + strlen(fileSystem.fileList[fileSystem.currFile]) + 1);
	Handle fileHandle;


	CPU_Running = false;

	/* Alloc ROM Directory */
	char ROM_DIR[ROMDIR_Size];

	/* Clear ROM_Dir */
	memset(ROM_DIR, 0x0, ROMDIR_Size);


	//FS_StringConc(ROM_DIR,  "/3DNES/ROMS/", fileSystem.fileList[fileSystem.currFile]);

	/* TODO: FIX IT
	/*if (SRAM_Name != NULL) {
		linearFree(SRAM_Name);
		SRAM_Size = (strlen(fileSystem.fileList[fileSystem.currFile]) - 4);
		SRAM_Name = linearAlloc(SRAM_Size);
		strncpy((char*)SRAM_Name, fileSystem.fileList[fileSystem.currFile], SRAM_Size);
	}
	*/
	
	FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, FS_makePath(PATH_CHAR, ROM_DIR), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	FSFILE_GetSize(fileHandle, &ROM_Size);
	FSFILE_Read(fileHandle, &bytesRead, 0x0, (u32*)ROM_Cache, (u32)ROM_Size);
	FSFILE_Close(fileHandle);

	/* Start Emulation */
	inGame = true;
}

void NES_ConfigurationMenu() { 
	u32 keys = ((u32*)0x10000000)[7];
	
	/* Configuration Menu */
		if(keys & BUTTON_LEFT) {
			if(!fileSystem.UKEY_LEFT) {
				switch(fileSystem.cConfig) {
					case 0:
						frameSkip--;
					break;

					case 1:
						VSYNC = false;
					break;

					case 2:
						
					break;

					default:

					break;
				}

				fileSystem.UKEY_LEFT = 1;
			}
		} else {
			fileSystem.UKEY_LEFT = 0;
		}
	
		if(keys & BUTTON_RIGHT) {
			if(!fileSystem.UKEY_RIGHT) {
				switch(fileSystem.cConfig) {
					case 0:
						frameSkip++;
					break;

					case 1:
						VSYNC = true;
					break;

					case 2:
						
					break;

					default:

					break;
				}


				fileSystem.UKEY_RIGHT = 1;
			}
		} else {
			fileSystem.UKEY_RIGHT = 0;
		}

		if(keys & BUTTON_UP){
			if(!fileSystem.UKEY_UP){
				if(fileSystem.cConfig > 0)
					fileSystem.cConfig--;
				
				fileSystem.UKEY_UP = 1;
			}
		} else {
			fileSystem.UKEY_UP = 0;
		}

		if(keys & BUTTON_DOWN){
			if(!fileSystem.UKEY_DOWN) {

				if(fileSystem.cConfig < 2)
					fileSystem.cConfig++;
				
				fileSystem.UKEY_DOWN = 1;
			}
		} else {
			fileSystem.UKEY_DOWN = 0;
		}


		if(keys & BUTTON_B) {
			if(!fileSystem.UKEY_B) {
				if(fileSystem.cConfig == 2)
					fileSystem.inMenu = 0;

				fileSystem.UKEY_B = 1;
			} else {
				fileSystem.UKEY_B = 0;
			}
		}

}

void NES_CurrentFileUpdate() {
	u32 keys = ((u32*)0x10000000)[7];

	if(fileSystem.inMenu == 0) {
		if(keys & BUTTON_UP){
			if(!fileSystem.UKEY_UP){
				if(fileSystem.sFile > 0) 
					fileSystem.sFile--;
				 else {
					if(fileSystem.cFile > 0)
					   fileSystem.cFile--;
				}

				if(fileSystem.currFile > 0)
					fileSystem.currFile--;
				
				fileSystem.UKEY_UP = 1;
			}
		} else {
			fileSystem.UKEY_UP = 0;
		}

		if(keys & BUTTON_DOWN){
			if(!fileSystem.UKEY_DOWN) {
				if(fileSystem.cFile < fileSystem.totalFiles)
					fileSystem.cFile++;
				 else 
					fileSystem.sFile++;
				
				fileSystem.currFile++;
				
				fileSystem.UKEY_DOWN = 1;
			}
		} else {
			fileSystem.UKEY_DOWN = 0;
		}


		if(keys & BUTTON_LEFT) {
			if(!fileSystem.UKEY_LEFT) {
				fileSystem.inMenu = 1;

				fileSystem.UKEY_LEFT = 1;
			}
		} else {
			fileSystem.UKEY_LEFT = 0;
		}
	

	}


	if(keys & BUTTON_B) {
			if(!fileSystem.UKEY_B) {
				NES_LoadSelectedGame();

				fileSystem.UKEY_B = 1;
			} else {
				fileSystem.UKEY_B = 0;
			}
		}

}


void NES_MainMenu() {
	if (fileSystem.inMenu == 0) {
		NES_drawROMLIST();
		NES_CurrentFileUpdate();
	}
	else {
		NES_drawConfigurationMenu();
		NES_ConfigurationMenu();
	}
}
