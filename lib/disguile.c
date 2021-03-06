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
disguile_connect (SCM rest)
{
  SCM smob, s_type = NULL, s_host = NULL, s_port = NULL;
  disguile_client_t *scm_client;
  riemann_client_t *client;
  riemann_client_type_t real_type;
  int port;
  long i;
  char *type, *host;

  if (scm_ilength (rest) > 3)
    scm_wrong_num_args (scm_from_utf8_symbol ("disguile/connect"));

  for (i = 0; i < scm_ilength (rest); i++)
    {
      SCM current = scm_list_ref (rest, scm_from_int64 (i));

      if (scm_is_keyword (current))
        s_type = current;
      else if (scm_is_string (current))
        s_host = current;
      else if (scm_is_number (current))
        s_port = current;
    }

  if (!s_type)
    s_type = scm_from_utf8_keyword ("tcp");
  if (!s_host)
    s_host = scm_from_utf8_string ("127.0.0.1");
  if (!s_port)
    s_port = scm_from_int32 (5555);

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

static SCM
_disguile_event_to_alist (riemann_event_t *event)
{
  SCM alist;
  size_t i;

  alist = scm_list_n (SCM_UNDEFINED);

  for (i = 0; i < event->n_attributes; i++)
    {
      const char *key, *value;

      key = event->attributes[i]->key;
      value = event->attributes[i]->value;

      alist = scm_acons (scm_from_utf8_symbol (key),
                         scm_from_utf8_string (value),
                         alist);
    }

  if (event->n_tags > 0)
    {
      SCM tags = scm_list_n (SCM_UNDEFINED);

      for (i = 0; i < event->n_tags; i++)
        {
          tags = scm_append_x
            (scm_list_2 (tags,
                         scm_list_1 (scm_from_utf8_string (event->tags[i]))));
        }

      alist = scm_acons (scm_from_utf8_symbol ("tags"),
                         tags,
                         alist);
    }

  if (event->has_ttl)
    alist = scm_acons (scm_from_utf8_symbol ("ttl"),
                       scm_from_double (event->ttl),
                       alist);

  if (event->has_metric_d)
    alist = scm_acons (scm_from_utf8_symbol ("metric"),
                       scm_from_double (event->metric_d),
                       alist);
  else if (event->has_metric_f)
    alist = scm_acons (scm_from_utf8_symbol ("metric"),
                       scm_from_double ((double) event->metric_f),
                       alist);
  else if (event->has_metric_sint64)
  alist = scm_acons (scm_from_utf8_symbol ("metric"),
                       scm_from_int64 (event->metric_sint64),
                       alist);

  if (event->description)
    alist = scm_acons (scm_from_utf8_symbol ("description"),
                       scm_from_utf8_string (event->description),
                       alist);

  if (event->host)
    alist = scm_acons (scm_from_utf8_symbol ("host"),
                       scm_from_utf8_string (event->host),
                       alist);

  if (event->service)
    alist = scm_acons (scm_from_utf8_symbol ("service"),
                       scm_from_utf8_string (event->service),
                       alist);

  if (event->state)
    alist = scm_acons (scm_from_utf8_symbol ("state"),
                       scm_from_utf8_string (event->state),
                       alist);

  return scm_list_1 (alist);
}

static SCM
disguile_query (SCM client_smob, SCM query_string)
{
  disguile_client_t *client;
  riemann_message_t *response;
  char *query;
  int r;
  size_t i;
  SCM results;

  scm_assert_smob_type (disguile_client_tag, client_smob);

  client = (disguile_client_t *) SCM_SMOB_DATA (client_smob);
  query = scm_to_locale_string (query_string);

  r = riemann_client_send_message_oneshot
    (client->client,
     riemann_message_create_with_query (riemann_query_new (query)));
  if (r != 0)
    {
      errno = -r;
      scm_syserror ("disguile/query");
    }

  response = riemann_client_recv_message (client->client);
  if (!response)
    {
      scm_syserror ("disguile/query");
    }

  if (response->ok != 1)
    {
      SCM err;

      err = scm_list_1 (scm_from_utf8_string (response->error));

      riemann_message_free (response);

      scm_misc_error ("disguile/query", "~S", err);
    }

  results = scm_list_n (SCM_UNDEFINED);

  for (i = 0; i < response->n_events; i++)
    {
      results = scm_append_x
        (scm_list_2 (results,
                     _disguile_event_to_alist (response->events [i])));
    }

  return results;
}

void
init_disguile ()
{
  disguile_client_tag = scm_make_smob_type ("disguile-client",
                                            sizeof (disguile_client_t));
  scm_set_smob_mark (disguile_client_tag, _disguile_client_mark);
  scm_set_smob_free (disguile_client_tag, _disguile_client_free);
  scm_set_smob_print (disguile_client_tag, _disguile_client_print);

  scm_c_define_gsubr ("disguile/connect", 0, 0, 1, disguile_connect);
  scm_c_define_gsubr ("disguile/send", 1, 0, 1, disguile_send);
  scm_c_define_gsubr ("disguile/query", 2, 0, 0, disguile_query);
}
