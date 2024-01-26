#include <stdio.h>
#include <unistd.h>

#include "wa.h"
#include "l1.h" /* TODO: remove this */

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

	fprintf(stderr, "Sending presence subscription\n");
	l1_presence_subscribe(wa, jid);

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
	pthread_t th; //pthread_t 是有關linux的data type (沒有定義)
	char config_dir[PATH_MAX];

	jid = argv[1];

	if(!jid)
	{
		fprintf(stderr, "Usage: %s <recipient>\n", argv[0]);
		return 1;
	}

	cb_t *cb; 
	//cb_t 是存放void, 2個callbacks function
	//*cb 是cb_t的instance，但用了pointer，由下面 calloc 去分配空間

	cb = calloc(sizeof(cb_t), 1);

	//以下兩個雖然是逗號，但其實也一樣的
	cb->priv_msg = cb_priv_msg, //把function給cb->priv_msg
	cb->update_user = cb_update_user, //把function給cb->update_user

	getcwd(config_dir, PATH_MAX); 
	//getcwd 是取得linux的當前目錄
	//然後再把當前目錄放在 config_dir 中

	strcat(config_dir, "/config"); 
	//strcat 是合併字串
	//所以config_dir應該會是 "/home/peter/libwa/config"

	wa = wa_init(cb, config_dir); 
	//wa 是在main外面定義的，它應該是最大的一個struct，成員包恬了 cb_t
	//現在要初始化它

	wa_login(wa);

	/* Wait until we receive the contact list */
	while(wa->run && (wa->state != WA_STATE_READY))
	{
		wa_dispatch(wa, 50);
	}

	fprintf(stderr, "Client ready\n");
	pthread_create(&th, NULL, input_worker, NULL);

	while(wa->run && client_run)
	{
		wa_dispatch(wa, 50);
	}

	wa_free(wa);

	pthread_join(th, NULL);

	return 0;
}

