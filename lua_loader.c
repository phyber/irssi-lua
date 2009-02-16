#include <lua_loader.h>
#include <lua_irssi.h>
#include <lua_api.h>

static GHashTable *lua_scripts = NULL;
static lua_State *current_interpreter = NULL;

lua_State *get_current_interpreter()
{
    return current_interpreter;
}

GHashTable *get_currently_loaded_scripts()
{
    return lua_scripts;
}

static void free_lua_script_hash_table_entry(gpointer key, gpointer interpreter, gpointer user_data)
{
    g_free(key);
    lua_close(interpreter);
}

gboolean script_loaded(const char *script_name)
{
    return g_hash_table_lookup(lua_scripts, script_name) != NULL;
}

void lua_load_script(const char *script_name)
{
    struct stat stat_buf;
    lua_State *interpreter;

    if (script_loaded(script_name))
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Script \"%s\" already loaded.", script_name);
        return;
    }

    if (0 != stat(script_name, &stat_buf))
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Script \"%s\" not found.", script_name);
        return;
    }

    interpreter = luaL_newstate();

    if (interpreter == NULL)
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Unable to create lua interpreter");
        return;
    }

    luaL_openlibs(interpreter);
    register_lua_api(interpreter);

    if (luaL_loadfile(interpreter, script_name) != 0)
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Unable to load script: %s", lua_tostring(interpreter, -1));
        lua_close(interpreter);
        return;
    }

    if (lua_pcall(interpreter, 0, 0, 0) != 0)
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Unable to execute the script.");
        lua_close(interpreter);
        return;
    }

    g_hash_table_insert(lua_scripts, g_strdup(script_name), interpreter);
    current_interpreter = interpreter;

    printtext(NULL, NULL, MSGLEVEL_CLIENTCRAP, "Script \"%s\" loaded.", script_name);
}

void lua_unload_script(const char *script_name)
{
    void *interpreter;
    void *key;

    if (! script_loaded(script_name))
    {
        printtext(NULL, NULL, MSGLEVEL_CLIENTERROR, "Script \"%s\" not loaded.", script_name);
        return;
    }

    if (g_hash_table_lookup_extended(lua_scripts, script_name, &key, &interpreter))
    {
        g_hash_table_remove(lua_scripts, script_name);

        g_free(key);
        lua_close(interpreter);
    }

    printtext(NULL, NULL, MSGLEVEL_CLIENTCRAP, "Script \"%s\" unloaded.", script_name);
}

void lua_loader_init()
{
    lua_scripts = g_hash_table_new(g_str_hash, g_str_equal);
}

void lua_loader_deinit()
{
    g_hash_table_foreach(lua_scripts, free_lua_script_hash_table_entry, NULL);
    g_hash_table_destroy(lua_scripts);
}
