#ifndef MACROS_H
#define MACROS_H

#define ARRAY_SIZE(a) (sizeof(a)/sizeof*(a))

#define MAX(a, b) \
({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a > _b ? _a : _b; \
})

#define MIN(a, b) \
({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a < _b ? _a : _b; \
})

#define CONTROL(ch) ((ch)-'A'+1)

#define INSIDE_RECT(r, p) \
({ \
    __auto_type _r = (r); \
    __auto_type _p = (p); \
    _p.x >= _r.x && _p.y >= _r.y && \
        _p.x < _r.x + _r.w && _p.y < _r.y + _r.h; \
})

#endif
