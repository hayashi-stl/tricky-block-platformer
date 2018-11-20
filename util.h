#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

void *file_contents(const char *filename, GLint *length);
void *read_tga(const char *filename, int *width, int *height);

#endif // UTIL_H_INCLUDED
