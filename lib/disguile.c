/* riemann-disguile -- Guile bindings for riemann-c-client
 * Copyright (C) 2015  Gergely Nagy <algernon@madhouse-project.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <riemann/riemann-client.h>
#include <libguile.h>

static scm_t_bits disguile_client_tag;

typedef struct
{
  riemann_client_t *client;

  SCM type, host, port;

  SCM update_func;
} disguile_client_t;

static SCM
disguile_connect (SCM s_type, SCM s_host, SCM s_port)
{
  SCM smob;
  disguile_client_t *scm_client;
  riemann_client_t *client;
  riemann_client_type_t real_type;
  int port;
  char *type, *host;

  type = scm_to_locale_string
    (scm_symbol_to_string (scm_keyword_to_symbol (s_type)));
  host = scm_to_locale_string (s_host);
  port = scm_to_int (s_port);

  if (strcmp (type, "tcp") == 0)
    real_type = RIEMANN_CLIENT_TCP;
  else if (strcmp (type, "udp") == 0)
    real_type = RIEMANN_CLIENT_UDP;
  else
    {
      free (type);
      free (host);

      scm_wrong_type_arg ("disguile/connect", 1, s_type);
    }

  client = riemann_client_create (real_type, host, port);
  free (type);
  free (host);

  if (!client)
    {
      scm_syserror ("disguile/connect");
    }

  scm_client = (disguile_client_t *)
    scm_gc_malloc (sizeof (disguile_client_t), "disguile-client");

  scm_client->client = client;
  scm_client->update_func = SCM_BOOL_F;

  smob = scm_new_smob (disguile_client_tag, (scm_t_bits)scm_client);

  scm_client->type = s_type;
  scm_client->host = s_host;
  scm_client->port = s_port;

  return smob;
}

static SCM
_disguile_client_mark (SCM client_smob)
{
  disguile_client_t *client = (disguile_client_t *) SCM_SMOB_DATA (client_smob);

  scm_gc_mark (client->type);
  scm_gc_mark (client->host);
  scm_gc_mark (client->port);

  return client->update_func;
}

static size_t
_disguile_client_free (SCM client_smob)
{
  disguile_client_t *client = (disguile_client_t *) SCM_SMOB_DATA (client_smob);

  scm_gc_free (client, sizeof (disguile_client_t), "disguile-client");

  return 0;
}

static int
_disguile_client_print (SCM client_smob, SCM port,
                        scm_print_state __attribute__((unused)) *pstate)
{
  disguile_client_t *client = (disguile_client_t *) SCM_SMOB_DATA (client_smob);

  scm_puts ("#<disguile-client ", port);
  scm_display (client->type, port);
  scm_puts (" ", port);
  scm_display (client->host, port);
  scm_puts (":", port);
  scm_display (client->port, port);
  scm_puts (">", port);

  return 1;
}

#define _disguile_event_set_one_string(event, property, current)        \
  {                                                                     \
    char *value = scm_to_locale_string (scm_cdr (current));             \
    riemann_event_set_one (event, property, value);                     \
    free (value);                                                       \
  }

static riemann_event_t *
_disguile_alist_to_event (SCM alist)
{
  riemann_event_t *event;
  long i;

  if (scm_ilength (alist) <= 0)
    return NULL;

  event = riemann_event_new ();

  for (i = 0; i < scm_ilength (alist); i++)
    {
      SCM current = scm_list_ref (alist, scm_from_int64 (i));
      char *key;

      key = scm_to_locale_string (scm_symbol_to_string (scm_car (current)));

      if (strcmp (key, "time") == 0)
        {
          int64_t value = scm_to_int64 (scm_cdr (current));

          riemann_event_set_one (event, TIME, value);
        }
      else if (strcmp (key, "state") == 0)
        {
          _disguile_event_set_one_string (event, STATE, current);
        }
      else if (strcmp (key, "service") == 0)
        {
          _disguile_event_set_one_string (event, SERVICE, current);
        }
      else if (strcmp (key, "host") == 0)
        {
          _disguile_event_set_one_string (event, HOST, current);
        }
      else if (strcmp (key, "description") == 0)
        {
          _disguile_event_set_one_string (event, DESCRIPTION, current);
        }
      else if (strcmp (key, "tags") == 0)
        {
          SCM taglist = scm_cdr (current);
          long n;

          for (n = 0; n < scm_ilength (taglist); n++)
            {
              SCM scm_tag = scm_list_ref (taglist, scm_from_int64 (n));
              char *tag;

              tag = scm_to_locale_string (scm_tag);

              riemann_event_tag_add (event, tag);
              free (tag);
            }
        }
      else if (strcmp (key, "ttl") == 0)
        {
          double value = scm_to_double (scm_cdr (current));

          riemann_event_set_one (event, TTL, value);
        }
      else if (strcmp (key, "metric") == 0)
        {
          double value = scm_to_double (scm_cdr (current));

          riemann_event_set_one (event, METRIC_D, value);
        }
      else
        {
          char *value = scm_to_locale_string (scm_cdr (current));

          riemann_event_attribute_add (event,
                                       riemann_attribute_create (key, value));
          free (value);
        }

      free (key);
    }

  return event;
}

static SCM
disguile_send (SCM client_smob, SCM events)
{
  disguile_client_t *client;
  long i;
  int r, n_events = 0;
  riemann_message_t *message;

  scm_assert_smob_type (disguile_client_tag, client_smob);

  client = (disguile_client_t *) SCM_SMOB_DATA (client_smob);

  message = riemann_message_new ();

  for (i = 0; i < scm_ilength (events); i++)
    {
      SCM current = scm_list_ref (events, scm_from_int64 (i));
      riemann_event_t *event;

      event = _disguile_alist_to_event (current);
      if (event)
        {
          riemann_message_append_events (message, event, NULL);
          n_events++;
        }
    }

  if (n_events)
    {
      r = riemann_client_send_message_oneshot (client->client, message);

      if (r != 0)
        {
          errno = -r;

          scm_syserror ("disguile/send");
        }
    }
  else
    {
      riemann_message_free (message);

      return SCM_BOOL_F;
    }

  return SCM_BOOL_T;
}

void
init_disguile ()
{
  disguile_client_tag = scm_make_smob_type ("disguile-client",
                                            sizeof (disguile_client_t));
  scm_set_smob_mark (disguile_client_tag, _disguile_client_mark);
  scm_set_smob_free (disguile_client_tag, _disguile_client_free);
  scm_set_smob_print (disguile_client_tag, _disguile_client_print);

  scm_c_define_gsubr ("disguile/connect", 3, 0, 0, disguile_connect);
  scm_c_define_gsubr ("disguile/send", 1, 0, 1, disguile_send);
}
