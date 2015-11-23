#include "broadcast.h"

// useful doc at http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

// ping packets consist solely of their TCP server's port number,
// which is a uint16, followed by PING_CODE, which is
// a uint32, so we're never going to be interested in any
// message longer than 2 + 4 = 6 bytes
#define PACKET_LEN 6

// a port can never be longer than 5 chars, +1 to NULL terminate
#define PORT_STRING_LEN 6

// FIXME: should we reset the socket every now and then?
// in particular test with the computer going to sleep mode, and such
void broadcast_listen(logger_t* logger)
{
  int sock_fd, socket_fds[FD_SETSIZE], socket_count = 0, highest_sock_fd = -1;
  int i, error_code, bytes_received_count;
  fd_set socket_fds_set;
  char buffer[PACKET_LEN + 1], port_string[PORT_STRING_LEN], host_info[NI_MAXHOST + NI_MAXSERV + 2], host[NI_MAXHOST], service[NI_MAXSERV];
  struct addrinfo hints, *address_list_head, *listen_address;
  struct sockaddr_storage their_address;
  socklen_t my_addr_len, their_address_len;
  uint16_t their_tcp_port;
  uint32_t ping_code_verif;
  size_t host_len;

  memset(&hints, 0, sizeof(struct addrinfo));
  // FIXME: actually test with ipv6
  hints.ai_family = AF_UNSPEC; // ipv4 or 6
  hints.ai_socktype = SOCK_DGRAM; // datagram socket
  hints.ai_flags = AI_PASSIVE; // wildcard IP address

  snprintf(port_string, 6, "%d", BROADCAST_PORT);
  error_code = getaddrinfo(NULL, port_string, &hints, &address_list_head);
  if (error_code != 0) {
    fatal(logger, "listen: getaddrinfo: %s\n", gai_strerror(error_code));
    exit(1);
  }

  // loop through all the results and bind to all we can
  for(listen_address = address_list_head; listen_address != NULL; listen_address = listen_address->ai_next) {
    my_addr_len = sizeof(struct addrinfo);
    error_code = getnameinfo(listen_address->ai_addr, my_addr_len,
                             host, NI_MAXHOST,
                             service, NI_MAXSERV, NI_NUMERICSERV);
    if (error_code == 0) {
      host_len = strlen(host);
      memcpy(host_info, host, host_len);
      memcpy(host_info + host_len, "/", 1);
      memcpy(host_info + host_len + 1, service, strlen(service) + 1);
    } else {
      warning(logger, "listen: failed to get human-readable host info");
      host_info[0] = '\0';
    }

    sock_fd = socket(listen_address->ai_family, listen_address->ai_socktype, listen_address->ai_protocol);
    
    if (sock_fd == -1) {
      warning(logger, "listen: failed to create socket fd for %s", host_info);
      continue;
    }

    error_code = bind(sock_fd, listen_address->ai_addr, listen_address->ai_addrlen);
    if (error_code != 0) {
      warning(logger, "listen: failed to bind to %s: %d", host_info, error_code);
      close(sock_fd);
      continue;
    }

    // success!
    info(logger, "listen: successfully bound to %s", host_info);
    // add the socket FD to the set of FDs we'll watch
    socket_fds[socket_count] = sock_fd;
    socket_count++;
    if (sock_fd > highest_sock_fd) {
      highest_sock_fd = sock_fd;
    }
  }

  // no longer needed
  freeaddrinfo(address_list_head);

  if (socket_count == 0) {
    fatal(logger, "listen: failed to bind any socket");
    exit(1);
  }

  // now we can listen for incoming pings
  their_address_len = sizeof(struct sockaddr_storage);
  while(1) {
    // select modifies the fd_set in place, so we need to rebuild it every time
    FD_ZERO(&socket_fds_set);
    for (i = 0; i < socket_count; i++) {
      FD_SET(socket_fds[i], &socket_fds_set);
    }

    // then wait for someone to ping us!
    error_code = select(highest_sock_fd + 1, &socket_fds_set, NULL, NULL, NULL);
    if (error_code == -1) {
      warning(logger, "listen: error while waiting for a ping: %d", errno);
    } else {
      debug(logger, "listen: select returned with %d", error_code);

      for (i = 0; i < socket_count; i++) {
        sock_fd = socket_fds[i];
        if (FD_ISSET(sock_fd, &socket_fds_set)) {
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
              warning(logger, "listen: ignoring failed broadcast request");
              break;
            default:
              debug(logger, "listen: received an unexpected broadcast packet of length %d", bytes_received_count);
          }
        }
      }
    }
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
