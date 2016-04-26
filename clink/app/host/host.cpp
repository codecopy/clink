// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "host.h"
#include "rl/rl_backend.h"
#include "rl/rl_history.h"

#include <core/os.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_compare.h>
#include <lib/match_generator.h>
#include <lib/line_editor.h>
// MODE4 #include <lua/lua_script_loader.h>
#include <lua/lua_state.h>
#include <lua/lua_match_generator.h>
#include <terminal/win_terminal.h>

/* MODE4
extern "C" {
#include <lua.h>
}
MODE4 */

//------------------------------------------------------------------------------
static setting_bool g_case_sensitive(
    "match.case_sensitive",
    "Case sensitive matching.",
    "", // MODE4
    false);

//------------------------------------------------------------------------------
static setting_bool g_case_relaxed(
    "match.case_relaxed",
    "Also consider -/_ equal when case insensitive.",
    "", // MODE4
    true);



//------------------------------------------------------------------------------
host::host(const char* name)
: m_name(name)
/* MODE4
: m_lua(lua)
, m_line_editor(editor)
MODE4 */
{
/* MODE4
    lua_load_script(lua, dll, dir);
    lua_load_script(lua, dll, env);
    lua_load_script(lua, dll, exec);
    lua_load_script(lua, dll, git);
    lua_load_script(lua, dll, go);
    lua_load_script(lua, dll, hg);
    lua_load_script(lua, dll, p4);
    lua_load_script(lua, dll, prompt);
    lua_load_script(lua, dll, self);
    lua_load_script(lua, dll, svn);
MODE4 */
}

//------------------------------------------------------------------------------
host::~host()
{
}

//------------------------------------------------------------------------------
bool host::edit_line(const char* prompt, str_base& out)
{
    struct cwd_restorer
    {
        cwd_restorer()  { os::get_current_dir(m_path); }
        ~cwd_restorer() { os::set_current_dir(m_path.c_str()); }
        str<288>        m_path;
    } cwd;

#if MODE4
    str<128> filtered_prompt;
    filter_prompt(prompt, filtered_prompt);
#endif

    int cmp_mode = str_compare_scope::exact;
    if (!g_case_sensitive.get())
        cmp_mode = g_case_relaxed.get() ? str_compare_scope::relaxed : str_compare_scope::caseless;

    str_compare_scope compare(cmp_mode);

    win_terminal terminal;
    rl_backend backend(m_name);
    rl_history history;
    editor_backend* ui = classic_match_ui_create();

    lua_state lua;
    lua_match_generator lua_generator(lua);

    line_editor::desc desc = {};
    desc.prompt = prompt;
    desc.quote_pair = "\"";
    desc.word_delims = " \t<>=;";
    desc.partial_delims = "\\/:";
    desc.auto_quote_chars = " %=;&^";
    desc.terminal = &terminal;
    desc.backend = &backend;
    desc.buffer = &backend;

    line_editor* editor = line_editor_create(desc);
    editor->add_backend(*ui);
    editor->add_generator(lua_generator);
    editor->add_generator(file_match_generator());

    bool ret = editor->edit(out.data(), out.size());

    if (ret)
        history.add(out.c_str());

    line_editor_destroy(editor);
    classic_match_ui_destroy(ui);
    return ret;
}

//------------------------------------------------------------------------------
void host::filter_prompt(const char* in, str_base& out)
{
#if MODE4
    // Call Lua to filter prompt
    lua_getglobal(m_lua, "clink");
    lua_pushliteral(m_lua, "filter_prompt");
    lua_rawget(m_lua, -2);

    lua_pushstring(m_lua, in);
    if (lua_pcall(m_lua, 1, 1, 0) != 0)
    {
        puts(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 2);
        return;
    }

    // Collect the filtered prompt.
    const char* prompt = lua_tostring(m_lua, -1);
    out = prompt;

    lua_pop(m_lua, 2);
#endif // MODE4
}
