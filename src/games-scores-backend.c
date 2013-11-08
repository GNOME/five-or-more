/* games-scores-backend.c 
 *
 * Copyright (C) 2005 Callum McKenzie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib.h>
#include <glib-object.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "games-score.h"
#include "games-scores.h"
#include "games-scores-backend.h"

struct GamesScoresBackendPrivate {
  GList *scores_list;
  GamesScoreStyle style;
  time_t timestamp;
  gchar *filename;
  gint fd;
};

G_DEFINE_TYPE (GamesScoresBackend, games_scores_backend, G_TYPE_OBJECT);

void
games_scores_backend_startup (void)
{
  /* Retained for compatibility */
}

static void
games_scores_backend_finalize (GObject *object)
{
  GamesScoresBackend *backend = GAMES_SCORES_BACKEND (object);
  GamesScoresBackendPrivate *priv = backend->priv;

  g_free (priv->filename);
  /* FIXME: more to do? */

  G_OBJECT_CLASS (games_scores_backend_parent_class)->finalize (object);
}

static void
games_scores_backend_class_init (GamesScoresBackendClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GamesScoresBackendPrivate));
  oclass->finalize = games_scores_backend_finalize;
}

static void
games_scores_backend_init (GamesScoresBackend * backend)
{
  backend->priv = G_TYPE_INSTANCE_GET_PRIVATE (backend,
                                               GAMES_TYPE_SCORES_BACKEND,
                                               GamesScoresBackendPrivate);
}

GamesScoresBackend *
games_scores_backend_new (GamesScoreStyle style,
                          char *base_name,
                          char *name)
{
  GamesScoresBackend *backend;
  char *pkguserdatadir;

  backend = GAMES_SCORES_BACKEND (g_object_new (GAMES_TYPE_SCORES_BACKEND,
                                                NULL));

  backend->priv->timestamp = 0;
  backend->priv->style = style;
  backend->priv->scores_list = NULL;
  pkguserdatadir = g_build_filename (g_get_user_data_dir (), base_name, NULL);
  backend->priv->filename = g_build_filename (pkguserdatadir, name, NULL);

  if (access (pkguserdatadir, O_RDWR) == -1) {
    /* Don't return NULL because games-scores.c does not
     * expect it, and can't do anything about it anyway. */
    mkdir (pkguserdatadir, 0775);
  }
  backend->priv->fd = -1;

  return backend;
}


/* Get a lock on the scores file. Block until it is available. 
 * This also supplies the file descriptor we need. The return value
 * is whether we were succesful or not. */
static gboolean
games_scores_backend_get_lock (GamesScoresBackend * self)
{
  gint error;
  struct flock lock;

  if (self->priv->fd != -1) {
    /* Assume we already have the lock and rewind the file to
     * the beginning. */
    lseek (self->priv->fd, 0, SEEK_SET);
    return TRUE;                /* Assume we already have the lock. */
  }

  self->priv->fd = open (self->priv->filename, O_RDWR | O_CREAT, 0755);
  if (self->priv->fd == -1) {
    return FALSE;
  }

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  error = fcntl (self->priv->fd, F_SETLKW, &lock);

  if (error == -1) {
    close (self->priv->fd);
    self->priv->fd = -1;
    return FALSE;
  }

  return TRUE;
}

/* Release the lock on the scores file and dispose of the fd. */
/* We ignore errors, there is nothing we can do about them. */
static void
games_scores_backend_release_lock (GamesScoresBackend * self)
{
  struct flock lock;

  /* We don't have a lock, ignore this call. */
  if (self->priv->fd == -1)
    return;

  lock.l_type = F_UNLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  fcntl (self->priv->fd, F_SETLKW, &lock);

  close (self->priv->fd);

  self->priv->fd = -1;
}

/**
 * games_scores_backend_get_scores:
 * @self: the backend to get the scores from
 * 
 * You can alter the list returned by this function, but you must
 * make sure you set it again with the _set_scores method or discard it
 * with with the _discard_scores method. Otherwise deadlocks will ensue.
 *
 * Return value: (transfer none) (allow-none) (element-type GnomeGamesSupport.Score): The list of scores
 */
GList *
games_scores_backend_get_scores (GamesScoresBackend * self)
{
  gchar *buffer;
  gchar *eol;
  gchar *scorestr;
  gchar *timestr;
  GamesScore *newscore;
  struct stat info;
  int error;
  ssize_t length, target;
  GList *t;
  
  /* Check for a change in the scores file and update if necessary. */
  error = stat (self->priv->filename, &info);

  /* If an error occurs then we give up on the file and return NULL. */
  if (error != 0) {
    return NULL;
  }

  if ((info.st_mtime > self->priv->timestamp) || (self->priv->scores_list == NULL)) {
    self->priv->timestamp = info.st_mtime;

    /* Dump the old list of scores. */
    t = self->priv->scores_list;
    while (t != NULL) {
      g_object_unref (t->data);
      t = g_list_next (t);
    }
    g_list_free (self->priv->scores_list);
    self->priv->scores_list = NULL;

    /* Lock the file and get the list. */
    if (!games_scores_backend_get_lock (self))
      return NULL;

    buffer = g_malloc (info.st_size + 1);
    if (buffer == NULL) {
      games_scores_backend_release_lock (self);
      return NULL;
    }

    target = info.st_size;
    length = 0;
    do {
      target -= length;
      length = read (self->priv->fd, buffer, info.st_size);
      if (length == -1) {
        games_scores_backend_release_lock (self);
        g_free (buffer);
        return NULL;
      }
    } while (length < target);

    buffer[info.st_size] = '\0';

    /* FIXME: These details should be in a sub-class. */

    /* Parse the list. We start by breaking it into lines. */
    /* Since the buffer is null-terminated 
     * we can do the string stuff reasonably safely. */
    eol = strchr (buffer, '\n');
    scorestr = buffer;
    while (eol != NULL) {
      *eol++ = '\0';
      timestr = strchr (scorestr, ' ');
      if (timestr == NULL)
        break;
      *timestr++ = '\0';
      /* At this point we have two strings, both null terminated. All
       * part of the original buffer. */
      switch (self->priv->style) {
      case GAMES_SCORES_STYLE_PLAIN_DESCENDING:
      case GAMES_SCORES_STYLE_PLAIN_ASCENDING:
        newscore = games_score_new_plain (g_ascii_strtod (scorestr, NULL));
        break;
      case GAMES_SCORES_STYLE_TIME_DESCENDING:
      case GAMES_SCORES_STYLE_TIME_ASCENDING:
        newscore = games_score_new_time (g_ascii_strtod (scorestr, NULL));
        break;
      default:
        g_assert_not_reached ();
      }
      games_score_set_time (newscore, g_ascii_strtoull (timestr, NULL, 10));
      self->priv->scores_list = g_list_append (self->priv->scores_list, newscore);
      /* Setup again for the next time around. */
      scorestr = eol;
      eol = strchr (eol, '\n');
    }

    g_free (buffer);
  }

  /* FIXME: Sort the scores! We shouldn't rely on the file being sorted. */

  return self->priv->scores_list;
}

gboolean
games_scores_backend_set_scores (GamesScoresBackend * self, GList * list)
{
  GList *s;
  GamesScore *d;
  gchar *buffer;
  gint output_length = 0;
  gchar dtostrbuf[G_ASCII_DTOSTR_BUF_SIZE];

  if (!games_scores_backend_get_lock (self))
    return FALSE;

  self->priv->scores_list = list;

  s = list;
  while (s != NULL) {
    gdouble rscore;
    guint64 rtime;

    d = (GamesScore *) s->data;
    rscore = 0.0;
    switch (self->priv->style) {
    case GAMES_SCORES_STYLE_PLAIN_DESCENDING:
    case GAMES_SCORES_STYLE_PLAIN_ASCENDING:
      rscore = games_score_get_value_as_plain (d);
      break;
    case GAMES_SCORES_STYLE_TIME_DESCENDING:
    case GAMES_SCORES_STYLE_TIME_ASCENDING:
      rscore = games_score_get_value_as_time(d);
      break;
    default:
      g_assert_not_reached ();
    }
    rtime = games_score_get_time (d);

    buffer = g_strdup_printf ("%s %"G_GUINT64_FORMAT"\n",
                              g_ascii_dtostr (dtostrbuf, sizeof (dtostrbuf),
                                              rscore), rtime);
    write (self->priv->fd, buffer, strlen (buffer));
    output_length += strlen (buffer);
    /* Ignore any errors and blunder on. */
    g_free (buffer);

    s = g_list_next (s);
  }

  /* Remove any content in the file that hasn't yet been overwritten. */
  ftruncate (self->priv->fd, output_length--);

  /* Update the timestamp so we don't reread the scores unnecessarily. */
  self->priv->timestamp = time (NULL);

  games_scores_backend_release_lock (self);

  return TRUE;
}

void
games_scores_backend_discard_scores (GamesScoresBackend * self)
{
  games_scores_backend_release_lock (self);
}
