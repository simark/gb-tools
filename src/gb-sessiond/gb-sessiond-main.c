#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

#include <gb/comm.h>

const char gb_sessiond_path[] = "ipc:///tmp/gb_zmq_socket";
static volatile int quit_flag = 0;

struct gb_sessiond {
	void *zmq_ctx;
	void *zmq_sock;
};

struct gb_sessiond *sessiond_new(void)
{
	struct gb_sessiond *sessiond;

	sessiond = malloc(sizeof(*sessiond));

	if (!sessiond)
		return NULL;

	memset(sessiond, 0, sizeof(*sessiond));

	return sessiond;
}

void sessiond_free(struct gb_sessiond *sessiond)
{
	if (sessiond->zmq_sock) {
		zmq_close(sessiond->zmq_sock);
	}

	if (sessiond->zmq_ctx) {
		zmq_ctx_destroy(sessiond->zmq_ctx);
	}

	free(sessiond);
}

int listen_sessiond(struct gb_sessiond *sessiond)
{
	int ret = 1;

	if (sessiond->zmq_ctx && sessiond->zmq_sock) {
		goto out;
	}

	sessiond->zmq_ctx = zmq_ctx_new();

	if (!sessiond->zmq_ctx) {
		fprintf(stderr, "ZMQ context creation failed.\n");
		goto out;
	}

	sessiond->zmq_sock = zmq_socket(sessiond->zmq_ctx, ZMQ_REP);

	if (!sessiond->zmq_sock) {
		fprintf(stderr, "ZMQ socket creation failed.\n");
		goto out_free_ctx;
	}

	if (zmq_bind(sessiond->zmq_sock, gb_sessiond_path) == -1) {
		fprintf(stderr, "ZMQ bin failed.\n");
		goto out_free_socket;
	}

out:
	return ret;

out_free_socket:
	zmq_close(sessiond->zmq_sock);
	sessiond->zmq_sock = NULL;
out_free_ctx:
	zmq_ctx_destroy(sessiond->zmq_ctx);
	sessiond->zmq_ctx = NULL;
	ret = 0;
	goto out;
}

void process_client_message(struct gb_sessiond *sessiond)
{
	struct gb_client_msg msg;
	struct gb_client_res res;

	res.status = 1;

	if (zmq_recv(sessiond->zmq_sock, &msg, sizeof(msg), 0) == -1) {
		fprintf(stderr, "zmq_recv error.\n");
	}

	switch (msg.type) {
	case MSG_PUT_BREAKPOINT:
		printf("Address is %p\n", msg.put_breakpoint.address);
		break;
	default:
		printf("Unknown message\n.");
		res.status = 0;
		break;
	}

	if (zmq_send(sessiond->zmq_sock, &res, sizeof(res), 0) == -1) {
		fprintf(stderr, "zmq_send error.\n");
	}
}

static void sigint_handler(int sig, siginfo_t *si, void *v)
{
	quit_flag = 1;
}

int main()
{
	struct gb_sessiond *sessiond;
	int ret = 0, num;
	zmq_pollitem_t poll_items[2];
	struct sigaction sa;

	sa.sa_sigaction = sigint_handler;
	if (sigaction(SIGINT, &sa, NULL) < 0) {
		perror("sigaction");
		ret = 1;
		goto out;
	}

	if ((sessiond = sessiond_new() == NULL)) {
		ret = 1;
		goto out;
	}

	if (!listen_sessiond(sessiond)) {
		ret = 1;
		goto out_free_sessiond;
	}

	printf("Listening...\n");

	poll_items[0].socket = sessiond->zmq_sock;
	poll_items[0].events = ZMQ_POLLIN;

	for (;;) {
		num = zmq_poll(poll_items, 1, -1);

		if (num == -1) {
			if (errno == EINTR) {
				if (quit_flag) {
					printf("Received SIGINT, quitting.\n");
					break;
				}
			} else {
				perror("zmq_poll");
				break;
			}
		} else if (num > 0) {
			if (poll_items[0].events & ZMQ_POLLIN) {
				process_client_message(sessiond);
			} else {
				fprintf(stderr, "Unexpected event came out of zmq_poll.\n");
			}
		}
	}

out_free_sessiond:
	sessiond_free(sessiond);
out:
	return 0;
}
