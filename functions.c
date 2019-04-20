#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *trimLeft(char *s) {
    while (isspace(*s)) {
        s++;
    }
    return s;
}

char *trimRight(char *s) {
    int len = strlen(s);
    if (len == 0) {
        return s;
    }
    char *pos = s + len - 1;
    while (pos >= s && isspace(*pos)) {
        *pos = '\0';
        pos--;
    }
    return s;
}
char *trim(char *s) { return trimRight(trimLeft(s)); }

char **split(char *str, const char *sch, int *size) {
    char **split = (char **)malloc(sizeof(char *) * 10);
    char *pch;
    *size = 0;
    pch = strtok(str, sch);
    while (pch != NULL) {
        pch = trim(pch);
        split[*size] = (char *)malloc(sizeof(char) * strlen(pch));
        split[*size] = pch;
        (*size)++;
        pch = strtok(NULL, sch);
    }
    split[*size] = (char *)malloc(sizeof(char));
    split[*size] = NULL;
    return split;
}