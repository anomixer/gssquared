# Clipboard

## Copy

"Copy" for now will do the following:

* Pre-allocate enough memory for conceivable Apple II bitmaps (640x216 should do it)

Call the Display Engine Copy routine

This routine (depending on display engine) will:

* Copy the screen buffer to the temporary memory after it.
* return the bitmap dimensions

"Copy" will then populate the bmp header for use when / if the clipboard callback routine is called.

## Paste

Paste should be handled by the keyboard module. When a paste is done, copy the pasted string into a buffer.

each time C000 is read, if:
    C000 would return false, then read the next character from the buffer.
    if it would return true, then they have not processed the keystroke.

Implemented!
