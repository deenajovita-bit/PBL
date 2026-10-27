#include <stdlib.h>
#include "pagerep.h"

/* returns 1 if ok, 0 if bad (empty, not a number, negative, too long) */
int parse_refs(const char *text, int *refs, int *n) {
    const char *p = text;
    *n = 0;
    while (*p) {
        while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;
        char *end;
        long v = strtol(p, &end, 10);
        if (end == p || v < 0 || v > 9999) return 0;
        if (*n >= MAX_REFS) return 0;
        refs[(*n)++] = (int)v;
        p = end;
    }
    return *n > 0;
}
