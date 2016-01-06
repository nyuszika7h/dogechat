/*
 * relay-network.c - network functions for relay plugin
 *
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

#include <stdlib.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif /* HAVE_GNUTLS */

#include "../dogechat-plugin.h"
#include "relay.h"
#include "relay-network.h"
#include "relay-config.h"


int relay_network_init_ok = 0;
int relay_network_init_ssl_cert_key_ok = 0;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials_t relay_gnutls_x509_cred;
gnutls_priority_t *relay_gnutls_priority_cache = NULL;
gnutls_dh_params_t *relay_gnutls_dh_params = NULL;
#endif /* HAVE_GNUTLS */


/*
 * Sets SSL certificate/key file.
 *
 * If verbose == 1, a message is displayed if successful, otherwise a warning
 * (if no cert/key found in file).
 */

void
relay_network_set_ssl_cert_key (int verbose)
{
#ifdef HAVE_GNUTLS
    char *certkey_path, *certkey_path2;
    int ret;

    gnutls_certificate_free_credentials (relay_gnutls_x509_cred);
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);

    relay_network_init_ssl_cert_key_ok = 0;

    certkey_path = dogechat_string_expand_home (dogechat_config_string (relay_config_network_ssl_cert_key));
    if (certkey_path)
    {
        certkey_path2 = dogechat_string_replace (certkey_path, "%h",
                                                dogechat_info_get ("dogechat_dir",
                                                                  NULL));
        if (certkey_path2)
        {
            ret = gnutls_certificate_set_x509_key_file (relay_gnutls_x509_cred,
                                                        certkey_path2,
                                                        certkey_path2,
                                                        GNUTLS_X509_FMT_PEM);
            if (ret >= 0)
            {
                relay_network_init_ssl_cert_key_ok = 1;
                if (verbose)
                {
                    dogechat_printf (NULL,
                                    _("%s: SSL certificate and key have been "
                                      "set"),
                                    RELAY_PLUGIN_NAME);
                }
            }
            else
            {
                if (verbose)
                {
                    dogechat_printf (NULL,
                                    _("%s%s: warning: no SSL certificate/key "
                                      "found (option relay.network.ssl_cert_key)"),
                                    dogechat_prefix ("error"), RELAY_PLUGIN_NAME);
                }
            }
            free (certkey_path2);
        }
        free (certkey_path);
    }
#else
    /* make C compiler happy */
    (void) verbose;
#endif /* HAVE_GNUTLS */
}

/*
 * Sets gnutls priority cache.
 */

void
relay_network_set_priority ()
{
#ifdef HAVE_GNUTLS
    if (gnutls_priority_init (relay_gnutls_priority_cache,
                              dogechat_config_string (
                                  relay_config_network_ssl_priorities),
                              NULL) != GNUTLS_E_SUCCESS)
    {
        dogechat_printf (NULL,
                        _("%s%s: unable to initialize priority for SSL"),
                        dogechat_prefix ("error"), RELAY_PLUGIN_NAME);
        free (relay_gnutls_priority_cache);
        relay_gnutls_priority_cache = NULL;
    }
#endif /* HAVE_GNUTLS */
}

/*
 * Initializes network for relay.
 */

void
relay_network_init ()
{
#ifdef HAVE_GNUTLS

    /* credentials */
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);
    relay_network_set_ssl_cert_key (0);

    /* priority */
    relay_gnutls_priority_cache = malloc (sizeof (*relay_gnutls_priority_cache));
    if (relay_gnutls_priority_cache)
        relay_network_set_priority ();
#endif /* HAVE_GNUTLS */
    relay_network_init_ok = 1;
}

/*
 * Ends network for relay.
 */

void
relay_network_end ()
{
    if (relay_network_init_ok)
    {
#ifdef HAVE_GNUTLS
        if (relay_gnutls_priority_cache)
        {
            gnutls_priority_deinit (*relay_gnutls_priority_cache);
            free (relay_gnutls_priority_cache);
            relay_gnutls_priority_cache = NULL;
        }
        if (relay_gnutls_dh_params)
        {
            gnutls_dh_params_deinit (*relay_gnutls_dh_params);
            free (relay_gnutls_dh_params);
            relay_gnutls_dh_params = NULL;
        }
        gnutls_certificate_free_credentials (relay_gnutls_x509_cred);
#endif /* HAVE_GNUTLS */
        relay_network_init_ok = 0;
    }
}
