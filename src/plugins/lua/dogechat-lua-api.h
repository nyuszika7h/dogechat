/*
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2016 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of DogeChat, the extensible chat client.
 *
 * DogeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * DogeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DogeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOGECHAT_LUA_API_H
#define DOGECHAT_LUA_API_H 1

extern struct luaL_Reg dogechat_lua_api_funcs[];
extern struct t_lua_const dogechat_lua_api_consts[];

extern int dogechat_lua_api_buffer_input_data_cb (void *data,
                                                 struct t_gui_buffer *buffer,
                                                 const char *input_data);
extern int dogechat_lua_api_buffer_close_cb (void *data,
                                            struct t_gui_buffer *buffer);

#endif /* DOGECHAT_LUA_API_H */
