/*
 * tiny_printf.h
 *
 *  Created on: Feb 28, 2018
 *      Author: Michael Hartmann
 */

#ifndef TINY_PRINTF_H_
#define TINY_PRINTF_H_

#include <stdarg.h>

#define FLOAT_SUPPORT

#ifdef __cplusplus
 extern "C" {
#endif

   int siprintf(char *buf, const char *fmt, ...);
   int fiprintf(FILE * stream, const char *fmt, ...);
   int iprintf(const char *fmt, ...);
   int ts_formatstring(char *buf, const char *fmt, va_list va);
   int ts_formatlength(const char *fmt, va_list va);

#ifdef __cplusplus
}

#endif

#endif /* TINY_PRINTF_H_ */
