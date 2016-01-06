/*
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef DOGECHAT_RELAY_DOGECHAT_MSG_H
#define DOGECHAT_RELAY_DOGECHAT_MSG_H 1

struct t_relay_dogechat_nicklist;

#define RELAY_DOGECHAT_MSG_INITIAL_ALLOC 4096

/* object ids in binary messages */
#define RELAY_DOGECHAT_MSG_OBJ_CHAR      "chr"
#define RELAY_DOGECHAT_MSG_OBJ_INT       "int"
#define RELAY_DOGECHAT_MSG_OBJ_LONG      "lon"
#define RELAY_DOGECHAT_MSG_OBJ_STRING    "str"
#define RELAY_DOGECHAT_MSG_OBJ_BUFFER    "buf"
#define RELAY_DOGECHAT_MSG_OBJ_POINTER   "ptr"
#define RELAY_DOGECHAT_MSG_OBJ_TIME      "tim"
#define RELAY_DOGECHAT_MSG_OBJ_HASHTABLE "htb"
#define RELAY_DOGECHAT_MSG_OBJ_HDATA     "hda"
#define RELAY_DOGECHAT_MSG_OBJ_INFO      "inf"
#define RELAY_DOGECHAT_MSG_OBJ_INFOLIST  "inl"
#define RELAY_DOGECHAT_MSG_OBJ_ARRAY     "arr"

struct t_relay_dogechat_msg
{
    char *id;                          /* message id                        */
    char *data;                        /* binary buffer                     */
    int data_alloc;                    /* currently allocated size          */
    int data_size;                     /* current size of buffer            */
};

extern struct t_relay_dogechat_msg *relay_dogechat_msg_new (const char *id);
extern void relay_dogechat_msg_add_bytes (struct t_relay_dogechat_msg *msg,
                                         const void *buffer, int size);
extern void relay_dogechat_msg_set_bytes (struct t_relay_dogechat_msg *msg,
                                         int position, const void *buffer,
                                         int size);
extern void relay_dogechat_msg_add_type (struct t_relay_dogechat_msg *msg,
                                        const char *string);
extern void relay_dogechat_msg_add_char (struct t_relay_dogechat_msg *msg,
                                        char c);
extern void relay_dogechat_msg_add_int (struct t_relay_dogechat_msg *msg,
                                       int value);
extern void relay_dogechat_msg_add_long (struct t_relay_dogechat_msg *msg,
                                        long value);
extern void relay_dogechat_msg_add_string (struct t_relay_dogechat_msg *msg,
                                          const char *string);
extern void relay_dogechat_msg_add_buffer (struct t_relay_dogechat_msg *msg,
                                          void *data, int length);
extern void relay_dogechat_msg_add_pointer (struct t_relay_dogechat_msg *msg,
                                           void *pointer);
extern void relay_dogechat_msg_add_time (struct t_relay_dogechat_msg *msg,
                                        time_t time);
extern int relay_dogechat_msg_add_hdata (struct t_relay_dogechat_msg *msg,
                                        const char *path, const char *keys);
extern void relay_dogechat_msg_add_infolist (struct t_relay_dogechat_msg *msg,
                                            const char *name,
                                            void *pointer,
                                            const char *arguments);
extern void relay_dogechat_msg_add_nicklist (struct t_relay_dogechat_msg *msg,
                                            struct t_gui_buffer *buffer,
                                            struct t_relay_dogechat_nicklist *nicklist);
extern void relay_dogechat_msg_send (struct t_relay_client *client,
                                    struct t_relay_dogechat_msg *msg);
extern void relay_dogechat_msg_free (struct t_relay_dogechat_msg *msg);

#endif /* DOGECHAT_RELAY_DOGECHAT_MSG_H */
