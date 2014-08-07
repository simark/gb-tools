enum gb_client_msg_type {
	MSG_PUT_BREAKPOINT,
	MSG_DEL_BREAKPOINT,
	MSG_GET_BREAKPOINTS,
	MSG_GET_WAITING_PIDS,
};

struct gb_client_msg {
	enum gb_client_msg_type type;
	union {
		struct {
			void *address;
		} put_breakpoint;
		struct {
		} del_breakpoint;
		struct {
		} get_breakpoints;
		struct {
		} get_waiting_pids;
	};
};


struct gb_client_res {
	int status;
};
