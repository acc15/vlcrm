#include <stdlib.h>
#include <stdio.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_modules.h>
#include <vlc_actions.h>
#include <vlc_playlist.h>
#include <vlc_input_item.h>
#include <vlc_url.h>
#include <vlc_atomic.h>

#define DELETE_ID_BIT 0x40000000

struct intf_sys_t {
    atomic_int pending_removal_id;
    uint_fast32_t key_remove;
    uint_fast32_t key_delete;
};

void delete_item(intf_thread_t* intf, playlist_t* playlist, playlist_item_t* item) {
    char* local_path = vlc_uri2path(item->p_input->psz_uri);
    playlist_NodeDelete(playlist, item);
    if (remove(local_path) == -1) {
        msg_Err(intf, "Unable to delete playlist item, id=%d, path=%s", item->i_id, local_path);
    } else {
        msg_Info(intf, "Playlist item has been deleted, id=%d, path=%s", item->i_id, local_path);
    }
    free(local_path);
}

void remove_item(intf_thread_t* intf, playlist_t* playlist, playlist_item_t* item) {
    playlist_NodeDelete(playlist, item);
    msg_Info(intf, "Playlist item has been removed, id=%d", item->i_id);
}

void mark_item(intf_thread_t *intf, bool delete) {
    playlist_t* playlist = pl_Get(intf);
    playlist_Lock(playlist);
    playlist_item_t* item = playlist_CurrentPlayingItem(playlist);
    if (item != NULL && item->p_input != NULL && item->p_input->i_type == ITEM_TYPE_FILE) {
        atomic_store(&intf->p_sys->pending_removal_id, (delete ? DELETE_ID_BIT : 0) | item->i_id);
        if (playlist_CurrentSize(playlist) == 1) {
            playlist_Control(playlist, PLAYLIST_STOP, true);
        } else {
            playlist_Control(playlist, PLAYLIST_SKIP, true, 1);
        }
    }
    playlist_Unlock(playlist);
}

uint_fast32_t get_key(intf_thread_t* intf, const char* var_name) {
    char* remove_str = var_InheritString(intf, var_name);
    if (remove_str == NULL) {
        return 0;
    }
    uint_fast32_t key = vlc_str2keycode(remove_str);
    free(remove_str);
    return key;
}

static int on_key_press(vlc_object_t * p_this, char const * name, vlc_value_t oldval, vlc_value_t newval, void *p_data) {
    VLC_UNUSED(p_this);
    VLC_UNUSED(name);
    VLC_UNUSED(oldval);
    
    intf_thread_t *intf = (intf_thread_t*) p_data;
    if (intf->p_sys->key_remove == newval.i_int) {
        mark_item(intf, false);
    } else if (intf->p_sys->key_delete == newval.i_int) {
        mark_item(intf, true);
    }
    return VLC_SUCCESS;
}

static int on_playlist_item_changed(vlc_object_t *p_this, char const * name, vlc_value_t oldval, vlc_value_t newval, void *p_data) {
    VLC_UNUSED(name);
    VLC_UNUSED(oldval);
    VLC_UNUSED(newval);

    intf_thread_t *intf = (intf_thread_t *) p_data;
    int rm_id = atomic_load(&intf->p_sys->pending_removal_id);
    if (rm_id == 0) {
        return VLC_SUCCESS;
    }

    playlist_t* playlist = (playlist_t*) p_this;
    playlist_Lock(playlist);
    
    playlist_item_t* rm_item = playlist_ItemGetById(playlist, rm_id & ~DELETE_ID_BIT);
    if (rm_item == NULL) { 
        // pending item no more in playlist
        atomic_store(&intf->p_sys->pending_removal_id, 0);
    } else {
        playlist_item_t* cur_item = playlist_CurrentPlayingItem(playlist);
        if (playlist_CurrentSize(playlist) == 1 || cur_item == NULL || cur_item->i_id != rm_item->i_id) { 
            if (atomic_exchange(&intf->p_sys->pending_removal_id, 0) == rm_id) {
                if (rm_id & DELETE_ID_BIT) {
                    delete_item(intf, playlist, rm_item);
                } else {
                    remove_item(intf, playlist, rm_item);
                }
            }
        }
    }
    playlist_Unlock(playlist);
    return VLC_SUCCESS;
}

static int plugin_open(vlc_object_t *obj) {
    intf_thread_t *intf = (intf_thread_t *) obj;
    intf->p_sys = malloc(sizeof(intf_sys_t));
    if (intf->p_sys == NULL) {
        return VLC_ENOMEM;
    }
    msg_Dbg(intf, "Opening");
    atomic_init(&intf->p_sys->pending_removal_id, 0);
    intf->p_sys->key_remove = get_key(intf, "key-remove-current");
    intf->p_sys->key_delete = get_key(intf, "key-delete-current");

    var_AddCallback(intf->obj.libvlc, "key-pressed", on_key_press, intf);
    var_AddCallback(pl_Get(intf), "item-change", on_playlist_item_changed, intf);
    return VLC_SUCCESS;
}

static void plugin_close(vlc_object_t *obj) {
    intf_thread_t *intf = (intf_thread_t *) obj;
    var_DelCallback(pl_Get(intf), "item-change", on_playlist_item_changed, intf);
    var_DelCallback(intf->obj.libvlc, "key-pressed", on_key_press, intf );
    free(intf->p_sys);
    msg_Dbg(intf, "Closing");
}

#ifdef __APPLE__
#   define DEFAULT_REMOVE_HOTKEY "Shift+Command+r"
#   define DEFAULT_DELETE_HOTKEY "Shift+Command+d"
#else
#   define DEFAULT_REMOVE_HOTKEY "Shift+Ctrl+r"
#   define DEFAULT_DELETE_HOTKEY "Shift+Ctrl+d"
#endif

vlc_module_begin()
    set_text_domain(MODULE_STRING)
    set_shortname(MODULE_STRING)
    set_description("Plugin for deleting currently playing file")
    set_capability("interface", 0)
    set_callbacks(plugin_open, plugin_close)
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_HOTKEYS)
    set_section("Hotkeys (VLC must be restarted for changes to take effect)", NULL)
    add_string(
        "key-remove-current", 
        DEFAULT_REMOVE_HOTKEY, 
        "Remove playing item from playlist", 
        "Removes currently playing item from playlist", false)
    add_string(
        "key-delete-current", 
        DEFAULT_DELETE_HOTKEY, 
        "Delete playing file", 
        "Permanently deletes currently playing file", false) 
vlc_module_end()
