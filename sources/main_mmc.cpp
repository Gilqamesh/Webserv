#include "MultiClientChat.hpp"

int main(int argc, char *argv[])
{
	MultiClientChat mmc("0.0.0.0", "8080");

	// 1. - 3. see init()
	mmc.m_socket = mmc.init();

	// 4. Create an empty kqueue
	// 5. creae an eventSet for READs on the socket
	// 6. add evSet to the kqueue
	int kq = kqueue();
	struct kevent evSet;
	EV_SET(&evSet, mmc.m_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
	kevent(kq, &evSet, 1, NULL, 0, NULL); /* adding/modifying N (here N = 1) events to the queue */

	// 7. enter run() loop where incoming connections and sen/reseive messages are handled
	mmc.run(kq, mmc.m_socket);
	return EXIT_SUCCESS;
}