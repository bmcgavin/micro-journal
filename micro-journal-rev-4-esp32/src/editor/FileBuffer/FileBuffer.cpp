#include "FileBuffer.h"
#include "app/app.h"
#include "display/display.h"

//
#include <FS.h>
#include <SD.h>

//
int FileBuffer::getSeekPos()
{
    return seekPos;
}

//
String FileBuffer::getFileName()
{
    return fileName;
}

// Fill the buffer from File
void FileBuffer::load(String fileName)
{
    // app status
    JsonDocument &app = app_status();

    //
    if (fileName.isEmpty())
    {
        //
        app["error"] = "Load file failed. File name is empty.";
        app["screen"] = ERRORSCREEN;

        return;
    }

    // Save filen name
    this->fileName = fileName;

    // Check if the file exists, create if not
    if (!SD.exists(fileName))
    {
        File file = SD.open(fileName, FILE_WRITE);
        if (!file)
        {
            //
            file.close();
            delay(100);

            //
            app["error"] = "Failed to create file";
            app["screen"] = ERRORSCREEN;

            return;
        }

        //
        file.close();
        delay(100);
    }

    // open the text file
    app_log("Loading file %s\n", fileName);
    File file = SD.open(fileName);
    if (!file)
    {
        //
        app["error"] = format("file open failed %s\n", fileName);
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // Determine file size and set buffer accordingly
    fileSize = file.size();
    app_log("File: %s of size: %d\n", fileName, fileSize);

    // calcualte the file offset
    seekPos = 0;
    int stepSize = BUFFER_SIZE / 2; // use half of the buffer
    if (fileSize > 0)
    {
        // this offset will offer last portion of the buffer
        seekPos = (fileSize / stepSize) * stepSize;
    }

    // when it is exactly the buffer end
    // go one buffer behind so that screen will show something
    if (fileSize == seekPos && seekPos > 0)
    {
        if (seekPos > stepSize)
            seekPos -= stepSize;
        else
            // defensive code in order for the offset not to go negative (MAX in unsigned in)
            seekPos = 0;
    }

    // move the file position to offset
    if (!file.seek(seekPos))
    {
        //
        file.close();
        delay(100);

        //
        app["error"] = format("Failed to seek file pointer. fileSize: %d seekPos: %d\n", fileSize, seekPos);
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // reset the buffer
    reset();

    // Read file content into text buffer
    bufferSize = 0;
    buffer[bufferSize] = '\0';
    while (file.available())
    {
        buffer[bufferSize++] = file.read();
    }
    cursorPos = bufferSize;

    //
    file.close();
    delay(100);

    // log
    app_log("Read size: %d, seek: %d, buffer: %d, cursor: %d\n", fileSize, seekPos, bufferSize, cursorPos);
}

void FileBuffer::save()
{
    // app status
    JsonDocument &app = app_status();

    //
    app_log("Saving file %s\n", fileName);
    File file = SD.open(fileName, FILE_WRITE);
    if (!file)
    {
        //
        app["error"] = "Failed to open file for writing\n";
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());

        return;
    }

    // Seek to the last loaded offset
    if (!file.seek(seekPos))
    {
        app_log("Failed to seek file pointer\n");
        file.close();
        delay(100);

        return;
    }
    app_log("Writing file at: %d\n", seekPos);

    // writing the file content
    size_t length = file.print(buffer);
    if (length >= 0)
    {
        app_log("File written: %d bytes\n", length);
    }
    else
    {
        app["error"] = "Save failed\n";
        app["screen"] = ERRORSCREEN;
        app_log(app["error"].as<const char *>());
    }

    //
    file.close();
    delay(100);

    // recalculate the file size
    // calculate the file size
    file = SD.open(fileName, FILE_READ);
    if (!file)
    {
        app["error"] = "Failed to open file for reading\n";
        app_log(app["error"].as<const char *>());
        app["screen"] = ERRORSCREEN;

        //
        return;
    }

    //
    fileSize = file.size();
    file.close();
    delay(100);
}

//
void FileBuffer::reset()
{
    //
    memset(buffer, '\0', sizeof(buffer));

    //
    bufferSize = 0;
}

//
char *FileBuffer::getBuffer()
{
    return buffer;
}

//
int FileBuffer::getBufferSize()
{
    return bufferSize;
}

void FileBuffer::addChar(char c)
{
    if (bufferSize < BUFFER_SIZE)
    {
        // shift the trailing texts
        if (bufferSize > cursorPos)
            memmove(buffer + cursorPos + 1, buffer + cursorPos, bufferSize - cursorPos);

        //
        buffer[cursorPos++] = c;
        buffer[++bufferSize] = '\0';
    }
}

void FileBuffer::removeLastChar()
{
    if (bufferSize > 0 && cursorPos > 0)
    {
        // Shift the trailing texts left by one position
        if (bufferSize > cursorPos)
        {
            memmove(buffer + cursorPos - 1, buffer + cursorPos, bufferSize - cursorPos);
        }

        // Decrease buffer size and cursor position
        --bufferSize;
        --cursorPos;

        // Null terminate the buffer
        buffer[bufferSize] = '\0';
    }
}

void FileBuffer::removeLastWord()
{
    if (bufferSize == 0 || cursorPos == 0)
        return;

    int end = cursorPos - 1;

    // Move the end pointer to the last non-space character before cursorPos
    while (end >= 0 && buffer[end] == ' ')
        end--;

    // If the buffer is all spaces or empty
    if (end < 0)
        return;

    // Find the beginning of the last word before cursorPos
    int start = end;
    while (start >= 0 && buffer[start] != ' ' && buffer[start] != '\n')
        start--;

    // If start is less than 0, it means the word is at the beginning of the buffer
    if (start < 0)
    {
        start = 0;
    }

    // Shift the remaining characters after the word
    int shiftStart = start + 1;
    int shiftEnd = cursorPos;
    int shiftLength = bufferSize - cursorPos;

    memmove(&buffer[shiftStart], &buffer[shiftEnd], shiftLength);

    // Update buffer size and cursor position
    bufferSize -= (end - start + 1);
    cursorPos = shiftStart;

    // Null-terminate the buffer
    buffer[bufferSize] = '\0';
}

//
bool FileBuffer::available()
{
    // is there still buffer?
    return bufferSize < BUFFER_SIZE;
}