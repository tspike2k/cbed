/*
Authors:   tspike (github.com/tspike2k)
Copyright: Copyright (c) 2026
License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
*/

#include "../src/common.c"

static u8 g_buffer[2*1024*1024];

int main(){
    fmt_msg("This is a test of the text formatting functions provided by the ceabed library.\n");
    Buffer buffer = {g_buffer, Array_Len(g_buffer)};
    String a = fmt_buffer("The secret is {0}.", &buffer, fmt_i(42));
    String b = fmt_buffer("Hello, {1}!", &buffer, fmt_i(42), fmt_cstr("World"));
    fmt_msg_put(a.text, a.size);
    fmt_msg_put("\n", 1);
    fmt_msg_put(b.text, b.size);
    fmt_msg_put("\n", 1);
}
