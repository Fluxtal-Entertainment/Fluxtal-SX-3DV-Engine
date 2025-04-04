#include "platform.h"

//Linux platform layer
#if FPLATFORM_LINUX

#include "core/logger.h"
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h> //sudo apt-get install libx11-dev OR sudo dnf install libX11-devel (if not installed earlier)
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h> //sudo apt-get install libxkbcommon-x11-dev OR sudo dnf install libxkbcommon-x11-devel (if not installed earlier)
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   //nanosleep
#else
#include <unistd.h>//usleep
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct internal_state
{
    Display *display;
    xcb_connection_t *connection;
    xcb_window_t window;
    xcb_screen_t *screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state; 

b8 platform_startup(platform_state* plat_state, const char* application_name, i32 x, i32 y, i32 width, i32 height)
{
    //Create internal state
    plat_state -> internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state -> internal_state;
    //Connect to X server
    state -> display = XOpenDisplay(NULL);
    //Turn off key repeats
    XAutoRepeatOff(state -> display);
    //Retrive the connection from the display
    state -> connection = XGetXCBConnection(state -> display);
    if(xcb_connection_has_error(state -> connection))
    {
        FFATAL("FAILED to connect to X Server via XCB");
        return FALSE;
    }
    //Get data from X Server
    const struct xcb_setup_t *setup = xcb_get_setup(state -> connection);
    //Loop through screens using iterator
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for(i32 s = screen_p; s > 0; s--)
    {
        xcb_screen_next(&it);
    }
    //Assigning screen after screens've been lopped through
    state -> screen = it.data;
    //Allocate a XID for the window 
    state -> window = xcb_generate_id(state -> connection);
    //Register event types
    //XCB_CW_BACK_PIXEL = filling then window bg with a single color
    //XCB_CW_EVENT_MASK is required
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    //Listen for keyboard and mouse buttons
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    //Values to be sent over XCB(bg color, events)
    u32 value_list[] = {state -> screen -> black_pixel, event_values};
    //Create window
    xcb_void_cookie_t cookie = xcb_create_window(state -> connection,/*depth*/ XCB_COPY_FROM_PARENT, state -> window,/*parent*/ state -> screen -> root, x, y, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, state -> screen -> root_visual, event_mask, value_list);
    //Change the title
    xcb_change_property(state -> connection, XCB_PROP_MODE_REPLACE, state -> window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,/*data should be viewed 8 bits at the time*/ 8, strlen(application_name), application_name);
    //Tell the X Server to notify when the window menager attempts to destroy the window
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state -> connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state -> connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *wm_delete_reply = xcb_intern_atom_reply(state -> connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t *wm_protocols_reply = xcb_intern_atom_reply(state -> connection, wm_protocols_cookie, NULL);
    state -> wm_delete_win = wm_delete_reply -> atom;
    state -> wm_protocols = wm_protocols_reply -> atom;
    xcb_change_property(state -> connection, XCB_PROP_MODE_REPLACE, state -> window, wm_protocols_reply -> atom, 4, 32, 1, &wm_delete_reply -> atom);
    //Map the window to the screen
    xcb_map_window(state -> connection, state -> window);
    //Flush the stream
    i32 stream_result = xcb_flush(state -> connection);
    if(stream_result <= 0)
    {
        FFATAL("An ERROR occured when flushing the stream: %d", stream_result);
        return FALSE;
    }
    return TRUE;
}

void platform_shutdown(platform_state* plat_state)
{
    //Cold-cast to the known type
    internal_state *state = (internal_state *)plat_state -> internal_state;
    //Turn key repeats back on
    XAutoRepeatOn(state -> display);
    xcb_destroy_window(state -> connection, state -> window);
}

b8 platform_pump_messages(platform_state* plat_state)
{
    //Cold-cast to known type
    internal_state *state = (internal_state *)plat_state -> internal_state;
    xcb_generic_event_t *event;
    xcb_client_message_event_t *cm;
    b8 quit_flagged = FALSE;
    //Poll for events until null is returned
    while(event != 0)
    {
        event = xcb_poll_for_event(state -> connection);
        if(event == 0)
        {
            break;
        }
        //Input events
        switch(event -> response_type & ~0x80)
        {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE:
            {
                //Presses and releases of key
            }break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE:
            {
                //Presses and releases of mouse button
            }
            case XCB_MOTION_NOTIFY:
                //Mouse movement
            break;
            case XCB_CONFIGURE_NOTIFY:
            {
                //Resizing
            }
            case XCB_CLIENT_MESSAGE:
            {
                cm = (xcb_client_message_event_t *)event;
                //Window close
                if(cm -> data.data32[0] == state -> wm_delete_win)
                {
                    quit_flagged = TRUE;
                }
            }break;
            default:
                //Something else
                break;
        }
        free(event);
    }
    return !quit_flagged;
}

void* platform_allocate(u64 size, b8 aligned)
{
    return malloc(size);
}

void platform_free(void* block, b8 aligned)
{
    free(block);
}

void* platform_zero_memory(void* block, u64 size)
{
    return memset(block, 0, size);
}

void* platform_copy_memory(void* dest, const void* source, u64 size)
{
    return memcpy(dest, source, size);
}

void* platform_set_memory(void* dest, i32 value, u64 size)
{
    return memset(dest, value, size);
}

void platform_console_write(const char* message, u8 color)
{
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"}; //Fatal, Error, Warn, Info, Debug, Trace
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

void platform_console_write_error(const char* message, u8 color)
{
    const char* color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"}; //Fatal, Error, Warn, Info, Debug, Trace
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

f64 platform_get_absolute_time()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms)
{
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if(ms >= 1000)
    {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

#endif