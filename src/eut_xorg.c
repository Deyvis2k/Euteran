

#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>

xcb_connection_t *conn;
xcb_window_t window_id;

void *capture_key(void *data){
    xcb_key_press_event_t *key_event;
    xcb_keycode_t keycode = 9;
    xcb_grab_key(conn, 1, window_id, XCB_MOD_MASK_ANY, keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_flush(conn);

    while(1) {
        xcb_generic_event_t *event = xcb_wait_for_event(conn);
        if(!event) continue;
        switch(event->response_type & ~0x80) {
            case XCB_KEY_PRESS:
                key_event = (xcb_key_press_event_t *)event;
                if(key_event->detail == keycode) {
                    // play_audio(data, NULL, NULL);
                }
                break;
            default:
                break;
        }
        free(event);
    }
    return NULL;
}


void main_f(){
    conn = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(conn)) {
        fprintf(stderr, "Cannot connect to X server\n");
        return;
    }

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    window_id = screen->root;

    u_int32_t event_mask = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    xcb_change_window_attributes(conn, window_id, XCB_CW_EVENT_MASK, &event_mask);
    xcb_flush(conn);

    xcb_disconnect(conn);

}
