#pragma once

#ifndef NULL
#define NULL 0
#endif // NULL

#define DEL_NULL(pointy) { delete pointy; pointy = NULL; }

#define DEL_NULL_ARR(pointyArr) { delete [] pointyArr; pointyArr = NULL; }