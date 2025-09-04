#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_modules.h>
#include <vlc_actions.h>
#include <vlc_playlist.h>
#include <vlc_input_item.h>
#include <vlc_url.h>
#include <vlc_atomic.h>

#include <trashcan.h>

typedef enum vlcrm_command_t {
    VLCRM_NOOP,
    VLCRM_REMOVE,
    VLCRM_DELETE,
    VLCRM_TRASH
} vlcrm_command_t;

typedef struct intf_sys_t {
    vlcrm_command_t command;
    uint_fast32_t key_remove;
    uint_fast32_t key_delete;
    uint_fast32_t key_trash;
} intf_sys_t;

vlcrm_command_t get_command_by_key(intf_sys_t* sys, uint_fast32_t key) {
    if (key == sys->key_remove) {
        return VLCRM_REMOVE;
    } else if (key == sys->key_delete) {
        return VLCRM_DELETE;
    } else if (key == sys->key_trash) {
        return VLCRM_TRASH;
    } else {
        return VLCRM_NOOP;
    }
}

void stop_and_run_command(intf_thread_t *intf, vlcrm_command_t command) {
    playlist_t* p_playlist = pl_Get(intf);
    PL_LOCK;
    playlist_item_t* item = playlist_CurrentPlayingItem(p_playlist);
    if (item != NULL && item->p_input != NULL && item->p_input->i_type == ITEM_TYPE_FILE) {
        intf->p_sys->command = command;
        playlist_Control(p_playlist, PLAYLIST_STOP, true);
    }
    PL_UNLOCK;
}

uint_fast32_t parse_key(intf_thread_t* intf, const char* var_name) {
    uint_fast32_t key = KEY_UNSET;
    char* remove_str = var_InheritString(intf, var_name);
    if (remove_str != NULL) {
        key = vlc_str2keycode(remove_str);
        free(remove_str);
    }
    return key;
}

int on_key_press(vlc_object_t * p_this, char const * name, vlc_value_t oldval, vlc_value_t newval, void *p_data) {
    VLC_UNUSED(p_this);
    VLC_UNUSED(name);
    VLC_UNUSED(oldval);
    
    intf_thread_t *intf = (intf_thread_t*) p_data;
    vlcrm_command_t command = get_command_by_key(intf->p_sys, (uint_fast32_t) newval.i_int);
    if (command != VLCRM_NOOP) {
        stop_and_run_command(intf, command);
    }
    return VLC_SUCCESS;
}

void trash_path(vlc_object_t* p_this, const char* path) {
    int status = trashcan_soft_delete(path);
    if (status == 0) {
        msg_Info(p_this, "Playlist item has been moved to Recycle Bin, path=%s", path);
    } else {
        const char* status_text = trashcan_status_msg(status);
        msg_Err(p_this, "Unable to move playlist item to Recycle Bin, path=%s, error=%s", path, status_text);
    }
}

void delete_path(vlc_object_t* p_this, const char* path) {
    if (remove(path) != -1) {
        msg_Info(p_this, "Playlist item has been deleted, path=%s", path);
    } else {
        msg_Err(p_this, "Unable to delete playlist item by path=%s, error=%s", path, strerror(errno));
    }
}

void run_command(vlc_object_t* p_this, vlcrm_command_t command, const char* path) {
    switch (command) {
    case VLCRM_TRASH:
        trash_path(p_this, path);
        break;
    case VLCRM_DELETE:
        delete_path(p_this, path);
        break;
    default:
        break;
    }
}

void pick_and_play_next(playlist_t* p_playlist) {
    if (playlist_IsEmpty(p_playlist)) {
        return;
    }
    int i_next = var_GetBool( p_playlist, "random" ) ? 0 : p_playlist->i_current_index;
    if (i_next < 0 || i_next >= p_playlist->current.i_size) {
        if (!var_GetBool( p_playlist, "loop" )) {
            return;
        }
        i_next = 0;
    }
    playlist_Control(p_playlist, PLAYLIST_VIEWPLAY, true, NULL, p_playlist->current.p_elems[i_next]);
}

int on_playlist_input_current(vlc_object_t *p_this, char const * name, vlc_value_t oldval, vlc_value_t newval, void *p_data) {
    VLC_UNUSED(name);
    VLC_UNUSED(oldval);
    VLC_UNUSED(newval);

    intf_thread_t *intf = (intf_thread_t *) p_data;
    vlcrm_command_t command = intf->p_sys->command;
    if (command == VLCRM_NOOP || newval.p_address != NULL) {
        return VLC_SUCCESS;
    }
    intf->p_sys->command = VLCRM_NOOP;

    playlist_t* p_playlist = (playlist_t*) p_this;
    
    PL_LOCK;
    playlist_item_t* p_current = playlist_CurrentPlayingItem(p_playlist);
    if (p_current != NULL) {
        char* input_path = NULL;
        if (command != VLCRM_REMOVE) {
            input_path = vlc_uri2path(p_current->p_input->psz_uri);
        }
        playlist_NodeDelete(p_playlist, p_current);
        if (input_path != NULL) {
            run_command(VLC_OBJECT(intf), command, input_path);
            free(input_path);
        }
        pick_and_play_next(p_playlist);
    }
    PL_UNLOCK;
    return VLC_SUCCESS;
}

static int plugin_open(vlc_object_t *obj) {
    intf_thread_t *intf = (intf_thread_t *) obj;
    intf->p_sys = malloc(sizeof(intf_sys_t));
    if (intf->p_sys == NULL) {
        return VLC_ENOMEM;
    }
    intf->p_sys->command = VLCRM_NOOP;
    intf->p_sys->key_remove = parse_key(intf, "key-remove-current");
    intf->p_sys->key_delete = parse_key(intf, "key-delete-current");
    intf->p_sys->key_trash = parse_key(intf, "key-trash-current");
    var_AddCallback(intf->obj.libvlc, "key-pressed", on_key_press, intf);
    var_AddCallback(pl_Get(intf), "input-current", on_playlist_input_current, intf);
    return VLC_SUCCESS;
}

static void plugin_close(vlc_object_t *obj) {
    intf_thread_t *intf = (intf_thread_t *) obj;
    var_DelCallback(pl_Get(intf), "input-current", on_playlist_input_current, intf);
    var_DelCallback(intf->obj.libvlc, "key-pressed", on_key_press, intf);
    if (intf->p_sys != NULL) {
        free(intf->p_sys);
        intf->p_sys = NULL;
    }
}

vlc_module_begin()
    set_text_domain(MODULE_STRING)
    set_shortname(MODULE_STRING)
    set_description("Plugin for deleting a currently playing file")
    set_capability("interface", 0)
    set_callbacks(&plugin_open, &plugin_close)
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_HOTKEYS)
    set_section("Hotkeys (VLC must be restarted for changes to take effect)", NULL)
    add_string(
        "key-remove-current", 
#ifdef __APPLE__
        "Command+Shift+n", 
#else
        "Ctrl+Shift+n",
#endif
        "Remove playing item from playlist", 
        "Removes currently playing item from playlist", 
        false
    )
    add_string(
        "key-delete-current", 
#ifdef __APPLE__
        "Command+Shift+d", 
#else
        "Ctrl+Shift+d",
#endif
        "Delete playing file", 
        "Permanently deletes currently playing file", 
        false
    ) 
    add_string(
        "key-trash-current", 
#ifdef __APPLE__
        "Command+Shift+t", 
#else
        "Ctrl+Shift+t",
#endif
        "Move playing file to Recycle Bin", 
        "Moves the currently playing file to the Recycle Bin", 
        false
    ) 
vlc_module_end()
