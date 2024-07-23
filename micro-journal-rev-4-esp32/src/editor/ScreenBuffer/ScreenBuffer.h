#include "editor/editor.h"

#ifndef ScreenBuffer_h
#define ScreenBuffer_h

#define SCREEN_BUFFER_SIZE 1000

//
class ScreenBuffer
{
public:
    //
    int rows = 10;
    int cols = 26;

    //
    char *line_position[SCREEN_BUFFER_SIZE + 2];
    int line_length[SCREEN_BUFFER_SIZE + 2];
    int total_line = 0;
    int start_line = 0;

    bool clear_later = false;
    unsigned int clear_last = 0;

    //
    void loop();
    void Update(FileBuffer &fileBuffer);
    void delayedClear(int millseconds);

    // clear screen flag
    bool clear = true;
};

#endif