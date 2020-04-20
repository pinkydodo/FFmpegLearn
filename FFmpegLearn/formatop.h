//
//  formatop.h
//  extraAudio
//
//  Created by pinky on 2020/4/11.
//  Copyright © 2020 pinky. All rights reserved.
//

#ifndef formatop_h
#define formatop_h

#include <stdio.h>

int getInfo( char* fileName );
int extra_audio( char* fileName );
int extra_video( char* fileName );

int remux( char* infile, char* outfile );
int crop( char* infile, int startTime, int duration , char* outfile);

//主要是把pts,dts拼接对了，对于第二个文件，要把pts,dts，全都计算上上个文件的截止时间
int concat( char* infile, char* infile2, char* outfile );

#endif /* formatop_h */
