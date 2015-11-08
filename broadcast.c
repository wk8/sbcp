#include "broadcast.h"

// useful doc at http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

// ping packets consist solely of their TCP server's port number,
// which is a uint16, followed by PING_CODE, which is
// a uint32, so we're never going to be interested in any
// message longer than 2 + 4 = 6 bytes
#define PACKET_LEN 6

// FIXME: should we reset the socket every now and then?
// in particular test with the computer going to sleep mode, and such
void broadcast_listen(logger_t* logger)
{
  int sock_fd, error_code, bytes_received_count;
  char buffer[PACKET_LEN + 1], port_string[6];
  struct addrinfo hints, *bcast_address;
  struct sockaddr_storage their_address;
  struct timespec wait_interval;
  socklen_t their_address_len;
  uint16_t their_tcp_port;
  uint32_t ping_code_verif;

  memset(&hints, 0, sizeof(struct addrinfo));
  // FIXME: actually test with ipv6
  hints.ai_family = AF_UNSPEC; // ipv4 or 6
  hints.ai_socktype = SOCK_DGRAM; // datagram socket
  hints.ai_flags = AI_PASSIVE; // wildcard IP address

  snprintf(port_string, 6, "%d", BROADCAST_PORT);
  error_code = getaddrinfo(NULL, port_string, &hints, &bcast_address);
  if (error_code != 0) {
    fatal(logger, "listen: getaddrinfo: %s\n", gai_strerror(error_code));
    exit(1);
  }

  // loop through all the results and bind to the first we can
  // TODO wkpo maybe we should broacast on all ifaces?
  for(; bcast_address != NULL; bcast_address = bcast_address->ai_next) {
    sock_fd = socket(bcast_address->ai_family, bcast_address->ai_socktype, bcast_address->ai_protocol);
    if (sock_fd == -1) {
      debug(logger, "listen: failed to create socket fd");
      continue;
    }

    error_code = bind(sock_fd, bcast_address->ai_addr, bcast_address->ai_addrlen);
    if (error_code != 0) {
      debug(logger, "listen: failed to bind: %d", error_code);
      close(sock_fd);
      continue;
    }

    // success!
    info(logger, "listen: successfully bound");
    break;
  }

  if (bcast_address == NULL) {
    fatal(logger, "listen: failed to bind socket");
    exit(1);
  }

  // no longer needed
  // TODO wkpo pas safe ca. goto exit, properly,
  // in all error cases above, and close socket too
  freeaddrinfo(bcast_address);

  // now we can listen for incoming pings
  their_address_len = sizeof(struct sockaddr_storage);
  wait_interval.tv_sec = 0;
  wait_interval.tv_nsec = 100000000; // 0.1 sec
  while (1) {
    // FIXME wkpo: should really be a select...!! not this ugly polling
    bytes_received_count = recvfrom(sock_fd, buffer, PACKET_LEN, 0,
                                    (struct sockaddr*) &their_address, &their_address_len);

    switch(bytes_received_count) {
      case PACKET_LEN:
        // TODO wkpo get the IP
        memcpy(&their_tcp_port, buffer, 2);
        memcpy(&ping_code_verif, buffer + 2, 4);
        info(logger, "listen: received a ping from TODO wkpo with tcp_port: %d and code verif %d", their_tcp_port, ping_code_verif);
        break;
      case -1:
        debug(logger, "listen: ignoring failed broadcast request");
        break;
      default:
        debug(logger, "listen: received an unexpected broadcast packet of length");
    }
    // TODO wkpo nanosleep(&wait_interval, NULL);
  }
}

// broadcasts the TCP port on the subnet
void broadcast_emit(logger_t* logger, uint16_t tcp_port)
{
  int sock_fd, bytes_sent_count, sock_optval, error_code;
  struct sockaddr_in addr_in;
  // TODO wkpo not thread safe, and deprecated; replace with
  // getaddrinfo (and don't forget to freeaddrinfo afterwards)
  struct hostent* hostent;
  char buffer[PACKET_LEN];
  uint32_t ping_code_verif = 2093727718;

  // retrieve host info for broadcast packets
  hostent = gethostbyname("255.255.255.255");
  if (hostent == NULL) {
    fatal(logger, "emit: could not get host info");
    exit(1);
  }

  // create the socket
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd == -1) {
    fatal(logger, "emit: failed to create socket fd");
    exit(1);
  }

  // set the right sock options to send broadcast packets
  error_code = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &sock_optval,
                          sizeof(int));
  if (error_code == -1) {
    fatal(logger, "emit: setsockopt (SO_BROADCAST)");
    exit(1);
  }

  // set address options
  memset(&addr_in, 0, sizeof(struct sockaddr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(BROADCAST_PORT);
  addr_in.sin_addr = *((struct in_addr*) hostent->h_addr);

  // prepare the packet to be sent
  memcpy(&tcp_port, buffer, 2);
  memcpy(&ping_code_verif, buffer + 2, 4);

  // and send it!
  bytes_sent_count = sendto(sock_fd, &buffer, PACKET_LEN, 0,
                            (struct sockaddr*) &addr_in, sizeof(struct sockaddr_in));
  if (bytes_sent_count != PACKET_LEN) {
    fatal(logger, "emit: unable to send full packet, sent %d bytes", bytes_sent_count);
    exit(1);
  }
  debug(logger, "emit: sent broadcast packet with TCP port %d", tcp_port);

  close(sock_fd);
}
