//
// 04/2017 Pirmin Schmid
//
// network handling follows this guide (code in Public Domain) with modifications:
// http://beej.us/guide/bgnet/output/html/multipage/index.html
//
// DVL message specifications as defined in server/common_listeners
//
// note: no specific security is implemented in this messaging system
// (similar to entire DVL server).
//
// However, a HelloMessage handshake must successfully be completed (similar to clients),
// which assures some minimal properties (server is active and can handle HelloMessage)
//
// additional options can be added:
// - PID of server needed additionally
// - stop_dvl actually waits confirmation signal that DVL has got the message
//   and started shutdown procedure
// - ...


#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "DVBasicTypes.h"
#include "toolbox/Version.h"
#include "toolbox/StringHelper.h"

using namespace std;
using namespace dv;
using namespace toolbox;

constexpr int kHelloMsgReplyAppIdIndex = 2;
constexpr int kHelloMsgReplyNeededVectorSize = 3;

constexpr char kMsgStopServer[] = "X";

constexpr int kMaxBufferLen = 4096;

const string name = "Stop DV";
const int version_major = 0;
const int version_minor = 9;
const int version_patch = 16;
const string kReleaseDate = "2017-09-24";

void error_exit(const string &name, const string &additional_text) {
	cout << "Usage: " << name << " <IP address> <port>" << endl;
	cout << endl;
	cout << "IP address and port of a running DVL server" << endl;
	cout << endl;
	cout << additional_text << endl;
	cout << endl;
	exit(1);
}

bool sendAllToSocket(int socket, const string &message) {
	const char *ptr = message.c_str();
	size_t len = message.size();
	while (len > 0) {
		ssize_t sent = send(socket, ptr, len, 0);
		if (sent < 1) {
			return false;
		}
		ptr += sent;
		len -= sent;
	}
	return true;
}

string connectAndSend(const string &address, const string &port, const string &message, bool wait_for_reply) {
	int sock = 0;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // current use as before: use provided IP address in address

	int status = getaddrinfo(address.c_str(), port.c_str(), &hints, &servinfo);
	if (status != 0) {
		cerr << "connectAndSend(): getaddrinfo error: " << gai_strerror(status) << endl;
		exit(1);
	}


	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1) {
			continue;
		}

		int c_status = connect(sock, p->ai_addr, p->ai_addrlen);
		if (c_status == -1) {
			close(sock);
			continue;
		}

		break;
	}

	if (p == NULL) {
		cerr << "connectAndSend(): Could not connect to " + address + ":" + port << endl;
		exit(1);
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (!sendAllToSocket(sock, message)) {
		cerr << "connectAndSend(): Could not send the complete message" << endl;
	}

	string reply;
	if (wait_for_reply) {
		char buf[kMaxBufferLen];
		ssize_t received_bytes = recv(sock, buf, kMaxBufferLen - 1, 0);

		if (received_bytes == -1) {
			cerr << "connectAndSend(): Error while receiving reply" << endl;
			exit(1);
		}

		buf[received_bytes] = '\0';
		reply = string(buf);
	}

	close(sock);
	return reply;
}

template <typename T>
T getInt(const string &name, const string &s, T min, T max, const string &error_text) {
	T r = 0;
	try {
		r = static_cast<T>(stoll(s));
	}   catch (const std::invalid_argument& ia) {
		error_exit(name, error_text);
	}
	if (r < min || max < r) {
		error_exit(name, error_text);
	}
	return r;
}

int main(int argc, char *argv[]) {
	cout << endl;
	Version version(name, version_major, version_minor, version_patch, kReleaseDate);
	version.print(&cout);

	string program_name = argv[0];
	if (argc < 3) {
		error_exit(program_name, "Wrong number of arguments.");
	}

	string address = argv[1];
	string port = argv[2];

	string reply = connectAndSend(address, port, "0:0:0", true);
	vector<string> parameters;
	StringHelper::splitStr(&parameters, reply, ":");
	if (parameters.size() < kHelloMsgReplyNeededVectorSize) {
		error_exit(program_name, "Invalid reply on Hello message.");
	}

	string appid_string = parameters[kHelloMsgReplyAppIdIndex];
	dv::id_type appid = getInt<dv::id_type>(program_name, appid_string,
											  0, numeric_limits<dv::id_type>::max(),
											  "Invalid appid (must be an integer >= 0)");

	cout << "DV server replied on Hello message with appid " << appid << endl;

	string stop_message = string(kMsgStopServer) + ":" + appid_string + ":stop";
	connectAndSend(address, port, stop_message , false);
	cout << "stop server message sent to DV server @ " << address << ":" << port << endl;
}
