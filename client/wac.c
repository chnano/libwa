#include <stdio.h>
#include <unistd.h>

#include "wa.h"

char *jid = NULL;
wa_t *wa = NULL;
int client_run = 1;

int
cb_priv_msg(void *ptr, priv_msg_t *msg)
{
	printf("%s\n", msg->text);
	fflush(stdout);
	return 0;
}

int
cb_update_user(void *ptr, user_t *u)
{
	//printf("New user: %s (%s)\n", u->name, u->jid);
	return 0;
}

void *
input_worker(void *ptr)
{
	char *line;
	size_t len;

	while(1)
	{
		line = NULL;
		len = 0;

		if(getline(&line, &len, stdin) < 0)
			break;

		/* Remove new line */
		line[strlen(line) - 1] = '\0';

		if(strlen(line) > 0)
			wa_send_priv_msg(wa, jid, line);

		free(line);
	}

	/* No mutex needed, only read from main thread */
	client_run = 0;

	return NULL;
}

int
main(int argc, char *argv[])
{
	pthread_t th;
	char session_file[PATH_MAX];

	jid = argv[1];

	if(!jid)
	{
		fprintf(stderr, "Usage: %s <recipient>\n", argv[0]);
		return 1;
	}

	cb_t cb =
	{
		.ptr = NULL,
		.priv_msg = cb_priv_msg,
		.update_user = cb_update_user,
	};

	wa = wa_init(&cb);

	getcwd(session_file, sizeof(session_file));

	strcat(session_file, "/session.json");

	wa_login(wa, session_file);

	/* Wait until we receive the contact list */
	while(wa->run && (wa->state != WA_STATE_READY))
	{
		wa_dispatch(wa, 50);
	}

	pthread_create(&th, NULL, input_worker, NULL);

	while(wa->run && client_run)
	{
		wa_dispatch(wa, 50);
	}

	wa_free(wa);

	pthread_join(th, NULL);

	return 0;
}

