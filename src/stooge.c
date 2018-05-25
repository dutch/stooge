/*
 * Copyright (C) 2018 Chris Lamberson <clamberson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <microhttpd.h>

struct handler_data
{
  char *cmds[32];
  pid_t pids[32];
  int ncmds;
};

static volatile sig_atomic_t done = 0;

static struct option longopts[] = {
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "port", required_argument, NULL, 'p' },
  { "dir", required_argument, NULL, 'C' },
  { "cmd", required_argument, NULL, 'e' },
  { NULL, 0, NULL, 0 }
};

static void
do_help(void)
{
  printf("usage: stooge [options]\n");
  printf("\n");
  printf("Listen for and respond to GitHub webhooks.\n");
  printf("\n");
  printf("options:\n");
  printf("  -h, --help              print this info and exit\n");
  printf("  -v, --version           print version and exit\n");
  printf("  -p PORT, --port=PORT    listen on PORT\n");
  printf("  -C DIR, --dir=DIR       change to DIR before doing anything\n");
  printf("  -e CMD, --cmd=CMD       run CMD as a single-line script\n");
  exit(0);
}

static void
do_version(void)
{
  printf("%s\n", PACKAGE_STRING);
  exit(0);
}

static int
request_cb(void *cls, struct MHD_Connection *conn, const char *url,
           const char *method, const char *version, const char *upload_data,
           size_t *upload_data_size, void **con_cls)
{
  struct MHD_Response *res;
  struct handler_data *hd;
  int i;

  if (strcmp(method, "POST") != 0)
    return MHD_NO;

  hd = cls;

  if (hd->ncmds == 0)
    goto respond;

  for (i = 0; i < hd->ncmds; ++i) {
    if ((hd->pids[i] = fork()) == -1)
      perror("fork");

    if (hd->pids[i] == 0) {
      if (execl("/bin/sh", "sh", "-c", hd->cmds[i], NULL) == -1)
        perror("exec");
    }
  }

respond:
  res = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
  MHD_queue_response(conn, MHD_HTTP_OK, res);
  MHD_destroy_response(res);

  return MHD_YES;
}

static void
signal_cb(int sig)
{
  done = 1;
}

int
main(int argc, char **argv)
{
  int res, fd, i, maxfd, port;
  char ch, *dir;
  struct MHD_Daemon *d;
  fd_set readfds, writefds, errorfds;
  sigset_t mask, oldmask;
  struct sigaction action;
  struct handler_data hd;

  res = EXIT_FAILURE;
  hd.ncmds = 0;
  maxfd = 0;
  port = 5000;
  dir = NULL;

  while ((ch = getopt_long(argc, argv, "hvp:C:e:", longopts, NULL)) != -1) {
    switch (ch) {
    case 'h':
      do_help();
      break;

    case 'v':
      do_version();
      break;

    case 'p':
      port = atoi(optarg);
      break;

    case 'C':
      dir = strdup(optarg);
      break;

    case 'e':
      hd.cmds[hd.ncmds++] = strdup(optarg);
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (dir) {
    if ((fd = open(".", O_RDONLY)) == -1) {
      perror("open");
      goto done;
    }

    if (chdir(dir) == -1) {
      perror("chdir");
      goto done;
    }
  }

  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = signal_cb;

  if (sigaction(SIGINT, &action, 0) == -1) {
    perror("sigaction");
    goto popd;
  }

  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);

  if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1) {
    perror("sigprocmask");
    goto popd;
  }

  d = MHD_start_daemon(0, port, NULL, NULL, request_cb, &hd, MHD_OPTION_END);

  if (d == NULL) {
    fprintf(stderr, "MHD_start_daemon: unknown failure\n");
    goto popd;
  }

  while (!done) {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&errorfds);
    MHD_get_fdset(d, &readfds, &writefds, &errorfds, &maxfd);

    if (pselect(maxfd + 1, &readfds, &writefds, &errorfds, NULL, &oldmask) == -1) {
      if (errno != EINTR)
        perror("pselect");
      break;
    }

    MHD_run_from_select(d, &readfds, &writefds, &errorfds);
  }

  fprintf(stderr, "\b\bCaught SIGINT, shutting down...\n");

  MHD_stop_daemon(d);

  res = EXIT_SUCCESS;

popd:
  if (dir) {
    if (fchdir(fd) == -1) {
      perror("fchdir");
      goto done;
    }
  }

done:
  return res;
}
