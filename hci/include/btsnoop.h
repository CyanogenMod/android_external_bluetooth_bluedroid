#pragma once

#include <stdbool.h>

void btsnoop_open(const char *p_path);
void btsnoop_close(void);

void btsnoop_capture(const HC_BT_HDR *p_buf, bool is_rcvd);
