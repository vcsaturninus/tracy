#ifndef __TRACY_H
#define __TRACY_H

/*
 * BSD 2-Clause License
 * 
 * Copyright (c) 2022, vcsaturninus <vcsaturninus@protonmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

void Tracy_init(void);
void Tracy_destroy(void);
void Tracy_push(const char *source_file, const char *func_name, int line_num);
void Tracy_pop(void);
void Tracy_dump(void);

#define TRACY_SYSLOG_ENV_VAR "TRACY_USE_SYSLOG"
#define TRACY_USE_LOGF_ENV_VAR "TRACY_USE_LOG_FILE"
#define TRACY_LOGF_NAME_ENV_VAR "TRACY_LOG_FILE"
#define TRACY_DEFAULT_LOG_PATH "/tmp/tracy.log"

#ifdef TRACY_DEBUG_MODE // enable tracy functionality
#   define TRETURN Tracy_pop(); return
#   define __tracy_push Tracy_push(__FILE__, __func__, __LINE__)
#   define __tracy_dump Tracy_dump()
#   define __tracy_init Tracy_init()
#   define __tracy_destroy Tracy_destroy()
#else // disable tracy functionality, removing all overhead
#   define TRETURN return
#   define __tracy_push
#   define __tract_dump 
#   define __tracy_init
#   define __tracy_destroy
#endif
    


#endif
