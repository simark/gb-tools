#include <stdio.h>
#include <zmq.h>

#include <gb/comm.h>

const char gb_sessiond_path[] = "ipc:///tmp/gb_zmq_socket";

struct gb_client {
	void *zmq_ctx;
	void *zmq_sock;
};

int connect_sessiond(struct gb_client *client)
{
	int ret = 1;

	if (client->zmq_ctx && client->zmq_sock) {
		goto out;
	}

	client->zmq_ctx = zmq_ctx_new();

	if (!client->zmq_ctx) {
		fprintf(stderr, "ZMQ context creation failed.\n");
		goto out;
	}

	client->zmq_sock = zmq_socket(client->zmq_ctx, ZMQ_REQ);

	if (!client->zmq_sock) {
		fprintf(stderr, "ZMQ socket creation failed.\n");
		goto out_free_ctx;
	}

	if (zmq_connect(client->zmq_sock, gb_sessiond_path) == -1) {
		fprintf(stderr, "ZMQ connect failed.\n");
		goto out_free_socket;
	}

out:
	return ret;

out_free_socket:
	zmq_close(client->zmq_sock);
	client->zmq_sock = NULL;
out_free_ctx:
	zmq_ctx_destroy(client->zmq_ctx);
	client->zmq_ctx = NULL;
	ret = 0;
	goto out;
}

int put_breakpoint(struct gb_client *client)
{
	struct gb_client_msg msg;
	struct gb_client_res res;
	int ret = 1;

	msg.type = MSG_PUT_BREAKPOINT;
	msg.put_breakpoint.address = &put_breakpoint;

	/* TODO: try to use zmq_msg_{send,recv} */
	if (zmq_send(client->zmq_sock, &msg, sizeof(msg), 0) == -1) {
		goto err;
	}

	if (zmq_recv(client->zmq_sock, &res, sizeof(res), 0) == -1) {
		goto err;
	}

	ret = res.status;
out:
	return ret;
err:
	ret = 0;
	goto out;
}

int main()
{
	struct gb_client client;
	int ret;
	ret = connect_sessiond(&client);


	if (ret) {
		put_breakpoint(&client);
	} else {
		printf("FAL\n");
	}



	return 0;
}
