/*
 * create a fake tty for `ldr` to load into and
 * read this data back out to make sure the ldr
 * loaded and the data sent are the same.
 */

#include "headers.h"

pid_t child;
int status = -1;

void child_exited(int sig)
{
	/* see if the child bombed */
	waitpid(child, &status, 0);
	status = WEXITSTATUS(status);
	if (status) {
		fprintf(stderr, "ERROR: child exited with %i\n", status);
		_exit(status);
	}
}

int main(int argc, char *argv[])
{
	enum { LOAD_TCP, LOAD_UDP, LOAD_TTY } load;
	char *buf, *target;
	int in_fd, ret, i;
	FILE *out_fp;
	struct sockaddr_in addr;
	struct sockaddr *saddr = (struct sockaddr *)&addr;
	socklen_t slen;

	if (argc == 1) {
 usage:
		fprintf(stderr,
			"USAGE: fake-loadee [load method] <ldr invocation>\n"
			"\n"
			"Load methods (default: --tty):\n"
			"  --tty\n"
			"  --network (same as --tcp)\n"
			"  --tcp\n"
			"  --udp\n"
		);
		return 1;
	}

	if (!strcmp(argv[1], "--tty"))
		load = LOAD_TTY;
	else if (!strcmp(argv[1], "--network"))
		load =  LOAD_TCP;
	else if (!strcmp(argv[1], "--udp"))
		load =  LOAD_UDP;
	else if (!strcmp(argv[1], "--tcp"))
		load =  LOAD_TCP;
	else {
		--argv;
		++argc;
		load =  LOAD_TTY;
	}
	++argv;
	--argc;

	switch (load) {
		case LOAD_TTY: {
			int slave;

			/* the fake tty to load into */
			ret = openpty(&in_fd, &slave, NULL, NULL, NULL);
			assert(ret == 0);

			/* load into the slave pty */
			target = malloc(10);
			sprintf(target, "#%i", slave);
			break;
		}

		case LOAD_UDP:
		case LOAD_TCP: {
			uint16_t random_local_port = 55192;

			addr.sin_family = AF_INET;
			addr.sin_port = htons(random_local_port);
			addr.sin_addr.s_addr = inet_addr("0.0.0.0");

			target = malloc(30);
			if (load == LOAD_TCP) {
				in_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
				sprintf(target, "tcp:localhost:%i", random_local_port);
			} else {
				in_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
				sprintf(target, "udp:localhost:%i", random_local_port);
			}
			assert(in_fd != -1);

			slen = sizeof(*saddr);
			ret = bind(in_fd, saddr, slen);
			assert(ret == 0);

			if (load == LOAD_TCP) {
				ret = listen(in_fd, 1);
				assert(ret == 0);
			}
			break;
		}
	}

	/* output file representing the data loaded into the tty */
	out_fp = fopen("load.ldr", "w+");
	assert(out_fp != NULL);

	/* recreate args to run `ldr` */
	if (argc == 1)
		goto usage;
	for (i = 1; i < argc; ++i)
		argv[i-1] = argv[i];
	argv[argc-1] = target;

	/* spawn the ldr prog and catch it exiting */
	signal(SIGCHLD, child_exited);

	child = fork();
	if (!child) {
		sleep(1);
		ret = execvp(argv[0], argv);
		fprintf(stderr, "ERROR: failed to execv(\"%s\"): %s\n", argv[0], strerror(errno));
		exit(ret);
	} else {
		if (load == LOAD_TCP) {
			slen = sizeof(*saddr);
			in_fd = accept(in_fd, saddr, &slen);
		}
	}

	/* wait for the autobaud char */
	buf = malloc(4);
	if (load == LOAD_UDP) {
		ret = recvfrom(in_fd, buf, 1, MSG_WAITALL, saddr, &slen);
	} else
		ret = read(in_fd, buf, 1);
	if (ret <= 0) {
		perror("fake-loadee: autobaud read failed");
		return 1;
	}
	if (buf[0] != '@') {
		perror("fake-loadee: autobaud != @");
		return 2;
	}

	/* send autobaud reply */
	buf[0] = 0xBF;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	if (load == LOAD_UDP)
		ret = sendto(in_fd, buf, 4, MSG_CONFIRM, saddr, slen);
	else
		ret = write(in_fd, buf, 4);

	/* dont block */
	ret = fcntl(in_fd, F_GETFL);
	fcntl(in_fd, F_SETFL, ret | O_NONBLOCK);

	/* read the LDR in random chunks */
	i = 0x8000;
	buf = realloc(buf, i);
	while (1) {
		ret = read(in_fd, buf, i);
		if (ret == -1 && errno == EAGAIN) {
			if (status)
				continue;
			ret = 0;
			break;
		} else if (ret < 0)
			break;
		else if (!ret)
			break;
		if (fwrite(buf, 1, ret, out_fp) != ret)
			perror("fake-loadee: fwrite was short");
	}
	fclose(out_fp);

	return ret;
}
