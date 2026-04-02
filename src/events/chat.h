#pragma once
#ifndef SOURCE_EVENTS_CHAT_VXL_HDR
#define SOURCE_EVENTS_CHAT_VXL_HDR

#include "directives/dextern.h"

#include "text/text_obj.h"

/* Chat log text object. */
extern vxtxt_obj vxchat_text;

VX_C_START

/* Append text to the chat and make it appear. */
void vxchat_add_text(const char *append);
/* Same as vxchat_add_text, but frees the given string afterwards. */
void vxchat_add_text_free(char *append);

/* Formats the given string for the chat log. */
char *vxchat_fmt_chat(char *to_fmt);

VX_C_END

#endif
